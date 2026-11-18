# Firmware: `adc.c`, pomiary napięć

[← Poprzedni](16-firmware-pwm.md) · [Spis treści](README.md) · [Następny →](18-firmware-sensors.md)

**Po co ten plik:** odczytuje dwa napięcia: z potencjometru (PA0) i fotorezystora (PA1). Temperaturę mierzy cyfrowy czujnik DS18B20 (osobny plik `ds18b20.c`). Pomiar trwa około 100 µs, więc program go nie „czeka”: zleca go, a przy kolejnym obrocie pętli sprawdza, czy jest gotowy.

**Dlaczego 16 próbek:** pojedynczy pomiar lekko „szumi”. Suma 16 kolejnych pomiarów daje stabilniejszy wynik. Największa możliwa suma to 16 × 1023 = 16 368, co mieści się w 16 bitach.

**Dlaczego odrzucamy pierwszy pomiar:** przetwornik ma w środku mały kondensator. Po zmianie kanału musi się on naładować do nowego napięcia, więc pierwszy pomiar bywa niedokładny.

## `firmware/src/adc.c`

[Otwórz ten plik w repozytorium](../../firmware/src/adc.c)

```c
  1  // Pomiary ADC bez blokowania: kanały po kolei, suma 16 próbek na kanał
  2  #include <avr/io.h>
  3  #include "adc.h"
  4
  5  // Zegar ADC 50–200 kHz: preskaler 8 przy 1 MHz, 64 przy kwarcu 11,0592 MHz
  6  #if F_CPU <= 1600000UL
  7  #define ADC_PRESCALER (_BV(ADPS1) | _BV(ADPS0))
  8  #elif F_CPU <= 12800000UL
  9  #define ADC_PRESCALER (_BV(ADPS2) | _BV(ADPS1))
 10  #else
 11  #error "Nieobsługiwane F_CPU (adc.c)"
 12  #endif
 13
 14  static uint16_t sums[ADC_CHANNELS];
 15  static uint16_t acc;
 16  static uint8_t channel;
 17  static uint8_t samples;
 18  static uint8_t skip;
 19  static uint8_t ready;
```

- **Linia 1:** Komentarz: pomiary bez blokowania, kanały po kolei, suma 16 próbek na kanał.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Nasz nagłówek, m.in. stałe `ADC_CHANNELS` i `ADC_SAMPLES`.
- **Linia 5:** Komentarz: przetwornik potrzebuje zegara od 50 do 200 kHz, więc preskaler zależy od zegara układu.
- **Linia 6:** Warunek sprawdzany przez kompilator: jeśli zegar to najwyżej 1,6 MHz (u nas 1 MHz)...
- **Linia 7:** ...bity `ADPS1` i `ADPS0` wybierają preskaler 8: 1 000 000 / 8 = 125 kHz.
- **Linia 8:** W przeciwnym razie, jeśli zegar to najwyżej 12,8 MHz (kwarc 11,0592 MHz)...
- **Linia 9:** ...bity `ADPS2` i `ADPS1` wybierają preskaler 64: 11 059 200 / 64 ≈ 173 kHz.
- **Linia 10:** Dla innych zegarów...
- **Linia 11:** ...kompilacja kończy się błędem.
- **Linia 12:** Koniec warunku.
- **Linia 14:** Tablica gotowych wyników: po jednej sumie na każdy z trzech kanałów.
- **Linia 15:** Suma zbierana dla bieżącego kanału.
- **Linia 16:** Numer kanału mierzonego w tej chwili.
- **Linia 17:** Ile próbek bieżącego kanału już zebrano.
- **Linia 18:** Znacznik: następny wynik należy odrzucić.
- **Linia 19:** Znacznik: zebrano pełny komplet wszystkich kanałów.

```c
 21  static void start(void)
 22  {
 23      ADCSRA |= _BV(ADSC);
 24  }
```

- **Linia 21:** Pomocnicza funkcja rozpoczynająca jeden pomiar.
- **Linia 22:** Początek funkcji.
- **Linia 23:** Ustawienie bitu `ADSC` w rejestrze `ADCSRA` uruchamia pomiar. Sprzęt sam skasuje ten bit po zakończeniu.
- **Linia 24:** Koniec funkcji.

### Przygotowanie przetwornika

```c
 26  void adc_init(void)
 27  {
 28      // Odniesienie AVCC (AREF tylko z kondensatorem)
 29      ADMUX = _BV(REFS0);
 30      ADCSRA = _BV(ADEN) | ADC_PRESCALER;
 31      skip = 1;
 32      start();
 33  }
```

- **Linia 26:** Początek funkcji `adc_init`.
- **Linia 27:** Początek treści funkcji.
- **Linia 28:** Komentarz: napięcie odniesienia to AVCC. Na płytce ATB do wyprowadzenia AREF podłączony jest tylko kondensator, nie żadne napięcie.
- **Linia 29:** Rejestr `ADMUX` wybiera odniesienie i kanał. Bit `REFS0` wybiera napięcie AVCC, czyli 5 V, a kanał to 0, bo pozostałe bity są zerami.
- **Linia 30:** Rejestr `ADCSRA`: `ADEN` włącza przetwornik, a `ADC_PRESCALER` wybiera preskaler ustalony na początku pliku.
- **Linia 31:** Pierwszy wynik ma zostać odrzucony.
- **Linia 32:** Uruchom pierwszy pomiar.
- **Linia 33:** Koniec funkcji.

### Zbieranie wyników

```c
 35  void adc_poll(void)
 36  {
 37      if (ADCSRA & _BV(ADSC))
 38          return;
 39
 40      // Pierwszy pomiar po zmianie kanału odrzucamy (ładowanie kondensatora S/H)
 41      if (skip) {
 42          skip = 0;
 43      } else {
 44          acc += ADC;
 45          if (++samples == ADC_SAMPLES) {
 46              sums[channel] = acc;
 47              acc = 0;
 48              samples = 0;
 49              if (++channel == ADC_CHANNELS) {
 50                  channel = 0;
 51                  ready = 1;
 52              }
 53              ADMUX = _BV(REFS0) | channel;
 54              skip = 1;
 55          }
 56      }
 57      start();
 58  }
```

- **Linia 35:** Funkcja wywoływana ciągle z pętli głównej.
- **Linia 36:** Początek funkcji.
- **Linia 37:** Sprawdź, czy bit `ADSC` jest jeszcze ustawiony, czyli czy pomiar nadal trwa. Jeśli tak...
- **Linia 38:** ...wyjdź natychmiast, bez czekania. Sprawdzimy znowu przy kolejnym obrocie pętli.
- **Linia 40:** Komentarz: pierwszy pomiar po zmianie kanału odrzucamy, bo ładuje się kondensator wewnętrzny (*sample and hold*, S/H).
- **Linia 41:** Jeśli ten wynik ma być odrzucony...
- **Linia 42:** ...skasuj znacznik i pomiń wynik.
- **Linia 43:** W przeciwnym razie, czyli wynik jest ważny:
- **Linia 44:** Dodaj wynik do sumy. `ADC` to rejestr z wynikiem pomiaru.
- **Linia 45:** Zwiększ licznik próbek. Jeśli osiągnął 16...
- **Linia 46:** ...zapisz sumę jako gotowy wynik kanału...
- **Linia 47:** ...wyzeruj sumę...
- **Linia 48:** ...wyzeruj licznik próbek.
- **Linia 49:** Przejdź do następnego kanału. Jeśli to już koniec listy...
- **Linia 50:** ...wróć do kanału 0...
- **Linia 51:** ...i oznacz, że mamy komplet pomiarów.
- **Linia 52:** Koniec warunku.
- **Linia 53:** Wybierz nowy kanał. Bit `REFS0` zostaje, a numer kanału trafia na dolne bity przez operator `|`.
- **Linia 54:** Następny wynik odrzuć, bo kanał się zmienił.
- **Linia 55:** Koniec warunku z linii 36.
- **Linia 56:** Koniec warunku z linii 32.
- **Linia 57:** Rozpocznij kolejny pomiar.
- **Linia 58:** Koniec funkcji.

### Odczyt wyników przez inne moduły

```c
 60  uint8_t adc_ready(void)
 61  {
 62      return ready;
 63  }
 64
 65  uint16_t adc_sum(uint8_t ch)
 66  {
 67      return sums[ch];
 68  }
 69
 70  uint16_t adc_value(uint8_t ch)
 71  {
 72      return (sums[ch] + ADC_SAMPLES / 2) / ADC_SAMPLES;
 73  }
```

- **Linia 60:** Funkcja mówi, czy jest już pełny komplet pomiarów.
- **Linia 61:** Początek funkcji.
- **Linia 62:** Zwróć znacznik.
- **Linia 63:** Koniec funkcji.
- **Linia 65:** Funkcja zwraca sumę 16 próbek danego kanału. Suma jest dokładniejsza niż średnia, przydaje się do temperatury.
- **Linia 66:** Początek funkcji.
- **Linia 67:** Zwróć sumę.
- **Linia 68:** Koniec funkcji.
- **Linia 70:** Funkcja zwraca średnią z 16 próbek, w zakresie 0–1023.
- **Linia 71:** Początek funkcji.
- **Linia 72:** Podziel sumę przez 16. Dodanie połowy dzielnika (8) sprawia, że wynik jest zaokrąglany, a nie obcinany.
- **Linia 73:** Koniec funkcji.

---

[← Poprzedni](16-firmware-pwm.md) · [Spis treści](README.md) · [Następny →](18-firmware-sensors.md)
