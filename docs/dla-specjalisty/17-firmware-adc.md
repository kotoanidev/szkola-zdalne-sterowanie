# Firmware: `adc.c`, pomiary napięć

[← Poprzedni](16-firmware-pwm.md) · [Spis treści](README.md) · [Następny →](18-firmware-sensors.md)

**Po co ten plik:** odczytuje trzy napięcia: z potencjometru, fotorezystora i czujnika temperatury. Pomiar trwa około 100 µs, więc program go nie „czeka”: zleca go, a przy kolejnym obrocie pętli sprawdza, czy jest gotowy.

**Dlaczego 16 próbek:** pojedynczy pomiar lekko „szumi”. Suma 16 kolejnych pomiarów daje stabilniejszy wynik. Największa możliwa suma to 16 × 1023 = 16 368, co mieści się w 16 bitach.

**Dlaczego odrzucamy pierwszy pomiar:** przetwornik ma w środku mały kondensator. Po zmianie kanału musi się on naładować do nowego napięcia, więc pierwszy pomiar bywa niedokładny.

## `firmware/src/adc.c`

[Otwórz ten plik w repozytorium](../../firmware/src/adc.c)

```c
  1  // Pomiary ADC bez blokowania: kanały po kolei, suma 16 próbek na kanał
  2  #include <avr/io.h>
  3  #include "adc.h"
  4
  5  static uint16_t sums[ADC_CHANNELS];
  6  static uint16_t acc;
  7  static uint8_t channel;
  8  static uint8_t samples;
  9  static uint8_t skip;
 10  static uint8_t ready;
```

- **Linia 1:** Komentarz: pomiary bez blokowania, kanały po kolei, suma 16 próbek na kanał.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Nasz nagłówek, m.in. stałe `ADC_CHANNELS` i `ADC_SAMPLES`.
- **Linia 5:** Tablica gotowych wyników: po jednej sumie na każdy z trzech kanałów.
- **Linia 6:** Suma zbierana dla bieżącego kanału.
- **Linia 7:** Numer kanału mierzonego w tej chwili.
- **Linia 8:** Ile próbek bieżącego kanału już zebrano.
- **Linia 9:** Znacznik: następny wynik należy odrzucić.
- **Linia 10:** Znacznik: zebrano pełny komplet wszystkich kanałów.

```c
 12  static void start(void)
 13  {
 14      ADCSRA |= _BV(ADSC);
 15  }
```

- **Linia 12:** Pomocnicza funkcja rozpoczynająca jeden pomiar.
- **Linia 13:** Początek funkcji.
- **Linia 14:** Ustawienie bitu `ADSC` w rejestrze `ADCSRA` uruchamia pomiar. Sprzęt sam skasuje ten bit po zakończeniu.
- **Linia 15:** Koniec funkcji.

### Przygotowanie przetwornika

```c
 17  void adc_init(void)
 18  {
 19      // Odniesienie AVCC, preskaler 8: 1 MHz / 8 = 125 kHz (zakres 50–200 kHz)
 20      ADMUX = _BV(REFS0);
 21      ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);
 22      skip = 1;
 23      start();
 24  }
```

- **Linia 17:** Początek funkcji `adc_init`.
- **Linia 18:** Początek treści funkcji.
- **Linia 19:** Komentarz: napięcie odniesienia to AVCC, a preskaler 8 daje 125 kHz. Przetwornik wymaga 50–200 kHz.
- **Linia 20:** Rejestr `ADMUX` wybiera odniesienie i kanał. Bit `REFS0` wybiera napięcie AVCC, czyli 5 V, a kanał to 0, bo pozostałe bity są zerami.
- **Linia 21:** Rejestr `ADCSRA`: `ADEN` włącza przetwornik, a `ADPS1` i `ADPS0` razem wybierają preskaler 8.
- **Linia 22:** Pierwszy wynik ma zostać odrzucony.
- **Linia 23:** Uruchom pierwszy pomiar.
- **Linia 24:** Koniec funkcji.

### Zbieranie wyników

```c
 26  void adc_poll(void)
 27  {
 28      if (ADCSRA & _BV(ADSC))
 29          return;
 30
 31      // Pierwszy pomiar po zmianie kanału odrzucamy (ładowanie kondensatora S/H)
 32      if (skip) {
 33          skip = 0;
 34      } else {
 35          acc += ADC;
 36          if (++samples == ADC_SAMPLES) {
 37              sums[channel] = acc;
 38              acc = 0;
 39              samples = 0;
 40              if (++channel == ADC_CHANNELS) {
 41                  channel = 0;
 42                  ready = 1;
 43              }
 44              ADMUX = _BV(REFS0) | channel;
 45              skip = 1;
 46          }
 47      }
 48      start();
 49  }
```

- **Linia 26:** Funkcja wywoływana ciągle z pętli głównej.
- **Linia 27:** Początek funkcji.
- **Linia 28:** Sprawdź, czy bit `ADSC` jest jeszcze ustawiony, czyli czy pomiar nadal trwa. Jeśli tak...
- **Linia 29:** ...wyjdź natychmiast, bez czekania. Sprawdzimy znowu przy kolejnym obrocie pętli.
- **Linia 31:** Komentarz: pierwszy pomiar po zmianie kanału odrzucamy, bo ładuje się kondensator wewnętrzny (*sample and hold*, S/H).
- **Linia 32:** Jeśli ten wynik ma być odrzucony...
- **Linia 33:** ...skasuj znacznik i pomiń wynik.
- **Linia 34:** W przeciwnym razie, czyli wynik jest ważny:
- **Linia 35:** Dodaj wynik do sumy. `ADC` to rejestr z wynikiem pomiaru.
- **Linia 36:** Zwiększ licznik próbek. Jeśli osiągnął 16...
- **Linia 37:** ...zapisz sumę jako gotowy wynik kanału...
- **Linia 38:** ...wyzeruj sumę...
- **Linia 39:** ...wyzeruj licznik próbek.
- **Linia 40:** Przejdź do następnego kanału. Jeśli to już koniec listy...
- **Linia 41:** ...wróć do kanału 0...
- **Linia 42:** ...i oznacz, że mamy komplet pomiarów.
- **Linia 43:** Koniec warunku.
- **Linia 44:** Wybierz nowy kanał. Bit `REFS0` zostaje, a numer kanału trafia na dolne bity przez operator `|`.
- **Linia 45:** Następny wynik odrzuć, bo kanał się zmienił.
- **Linia 46:** Koniec warunku z linii 36.
- **Linia 47:** Koniec warunku z linii 32.
- **Linia 48:** Rozpocznij kolejny pomiar.
- **Linia 49:** Koniec funkcji.

### Odczyt wyników przez inne moduły

```c
 51  uint8_t adc_ready(void)
 52  {
 53      return ready;
 54  }
 55
 56  uint16_t adc_sum(uint8_t ch)
 57  {
 58      return sums[ch];
 59  }
 60
 61  uint16_t adc_value(uint8_t ch)
 62  {
 63      return (sums[ch] + ADC_SAMPLES / 2) / ADC_SAMPLES;
 64  }
```

- **Linia 51:** Funkcja mówi, czy jest już pełny komplet pomiarów.
- **Linia 52:** Początek funkcji.
- **Linia 53:** Zwróć znacznik.
- **Linia 54:** Koniec funkcji.
- **Linia 56:** Funkcja zwraca sumę 16 próbek danego kanału. Suma jest dokładniejsza niż średnia, przydaje się do temperatury.
- **Linia 57:** Początek funkcji.
- **Linia 58:** Zwróć sumę.
- **Linia 59:** Koniec funkcji.
- **Linia 61:** Funkcja zwraca średnią z 16 próbek, w zakresie 0–1023.
- **Linia 62:** Początek funkcji.
- **Linia 63:** Podziel sumę przez 16. Dodanie połowy dzielnika (8) sprawia, że wynik jest zaokrąglany, a nie obcinany.
- **Linia 64:** Koniec funkcji.

---

[← Poprzedni](16-firmware-pwm.md) · [Spis treści](README.md) · [Następny →](18-firmware-sensors.md)
