# Firmware: `ds18b20.c`, czujnik temperatury

[← Poprzedni](21-firmware-demo.md) · [Spis treści](README.md) · [Następny →](30-serwer-start.md)

**Po co ten plik:** na płytce ATB jest cyfrowy czujnik temperatury DS18B20. Nie daje on napięcia do zmierzenia, tylko sam mierzy temperaturę i przesyła wynik jako liczbę po jednym przewodzie. Ten sposób komunikacji nazywa się **1-Wire** („jeden przewód”). Ten moduł co sekundę prosi czujnik o pomiar i odbiera wynik.

**Jak działa 1-Wire:** przewód danych jest podciągnięty rezystorem do +5 V, więc „sam z siebie” ma stan wysoki. Zarówno mikrokontroler, jak i czujnik mogą go tylko ściągnąć do masy (0 V) albo puścić. Każdy bit to krótka „szczelina czasowa” około 60 µs:

- mikrokontroler ściąga linię na chwilę, a potem ją puszcza – to bit 1 albo prośba o odczyt bitu,
- mikrokontroler trzyma linię nisko przez całą szczelinę – to bit 0,
- przy odczycie czujnik w odpowiedzi trzyma linię nisko (bit 0) albo jej nie rusza (bit 1), a mikrokontroler sprawdza stan najpóźniej 15 µs po początku szczeliny.

Przed każdą rozmową mikrokontroler wysyła **reset**: trzyma linię nisko przez 480 µs. Czujnik odpowiada wtedy **impulsem obecności**, czyli sam ściąga linię na chwilę.

**Pomiar krok po kroku:** reset → „pomiń adres” (`0xCC`) → „zmierz” (`0x44`) → czekanie 750 ms → reset → „pomiń adres” → „odczytaj pamięć” (`0xBE`) → 9 bajtów odpowiedzi. Każdy krok zajmuje najwyżej 1 ms, więc moduł wykonuje jeden krok na takt i reszta programu działa normalnie.

**Dlaczego czasy są tak ciasne:** przy zegarze 1 MHz jedna instrukcja trwa 1–2 µs, a czujnik wymaga odczytu w ciągu 15 µs. Dlatego na czas jednej szczeliny wyłączamy przerwania – przerwanie mogłoby opóźnić odczyt. Symulator simavr sprawdza te czasy w najmniej korzystnym wariancie z karty katalogowej.

## `firmware/src/ds18b20.c`

[Otwórz ten plik w repozytorium](../../firmware/src/ds18b20.c)

```c
  1  // Czujnik DS18B20 na 1-Wire: pomiar co 1 s, w jednym takcie 1 ms co najwyżej reset albo jeden bajt
  2  #include <avr/io.h>
  3  #include <util/atomic.h>
  4  #include <util/crc16.h>
  5  #include <util/delay.h>
  6  #include "pinout.h"
  7  #include "ds18b20.h"
  8
  9  #define CONVERT_MS 750
 10  #define PERIOD_MS  1000
 11  // Bit trzeba odczytać najpóźniej 15 µs po zboczu; przy 1 MHz jedna instrukcja to 1–2 µs
 12  #define SAMPLE_US  (F_CPU > 4000000UL ? 9 : 2)
 13
 14  #define CMD_SKIP_ROM  0xCC
 15  #define CMD_CONVERT   0x44
 16  #define CMD_READ      0xBE
 17
 18  // Linia z pull-upem na płytce: stan niski = pin jako wyjście (PORT = 0), zwolnienie = wejście
 19  #define bus_low()     (OW_DDR |= _BV(OW_BIT))
 20  #define bus_release() (OW_DDR &= ~_BV(OW_BIT))
 21  #define bus_high()    ((OW_PIN & _BV(OW_BIT)) != 0)
 22
 23  static int16_t tenths = TEMP_NONE;
 24  static uint16_t wait_ms;
 25  static uint8_t step;
 26  static uint8_t scratch[9];
```

- **Linia 1:** Komentarz: czujnik DS18B20 na 1-Wire, pomiar co sekundę, w jednej milisekundzie najwyżej reset albo jeden bajt.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Biblioteka z `ATOMIC_BLOCK`, czyli blokiem bez przerwań.
- **Linia 4:** Biblioteka z funkcją `_crc_ibutton_update`, która liczy sumę kontrolną CRC używaną przez czujniki 1-Wire.
- **Linia 5:** Biblioteka z dokładnymi opóźnieniami `_delay_us`. Długość opóźnienia oblicza się z `F_CPU`.
- **Linia 6:** Nasze przypisanie pinów, m.in. `OW_DDR` i `OW_BIT`.
- **Linia 7:** Nasz nagłówek, m.in. `TEMP_NONE`.
- **Linia 9:** Czujnik potrzebuje do 750 ms na pomiar w najwyższej rozdzielczości.
- **Linia 10:** Cały cykl pomiaru trwa 1000 ms.
- **Linia 11:** Komentarz: bit trzeba odczytać najpóźniej 15 µs po zboczu, a przy 1 MHz jedna instrukcja trwa 1–2 µs.
- **Linia 12:** Odstęp od zwolnienia linii do odczytu: 9 µs przy szybkim zegarze (kwarc 11,0592 MHz), a przy 1 MHz tylko 2 µs, bo same instrukcje zajmują już kilka mikrosekund.
- **Linia 14:** Kod komendy „pomiń adres” (SKIP ROM). Na linii jest jeden czujnik, więc nie trzeba go wskazywać.
- **Linia 15:** Kod komendy „zmierz temperaturę” (CONVERT T).
- **Linia 16:** Kod komendy „odczytaj pamięć roboczą” (READ SCRATCHPAD).
- **Linia 18:** Komentarz: linia ma rezystor podciągający na płytce. Stan niski uzyskujemy, ustawiając pin jako wyjście (w rejestrze wyjściowym ten bit jest zawsze 0), a zwolnienie linii to ustawienie pinu jako wejścia.
- **Linia 19:** `bus_low()` ustawia pin jako wyjście, czyli ściąga linię do masy.
- **Linia 20:** `bus_release()` ustawia pin jako wejście, czyli puszcza linię; rezystor podciąga ją do +5 V, chyba że trzyma ją czujnik.
- **Linia 21:** `bus_high()` odczytuje stan linii: 1, gdy jest wysoki.
- **Linia 23:** Ostatnia temperatura w dziesiątych częściach stopnia. Na początku `TEMP_NONE`, bo pomiaru jeszcze nie było.
- **Linia 24:** Ile milisekund zostało do następnego kroku.
- **Linia 25:** Numer bieżącego kroku pomiaru.
- **Linia 26:** Miejsce na 9 bajtów odczytanych z czujnika.

### Reset i impuls obecności

```c
 28  static uint8_t bus_reset(void)
 29  {
 30      uint8_t presence;
 31
 32      bus_low();
 33      _delay_us(480);
 34      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 35          bus_release();
 36          _delay_us(70);
 37          presence = !bus_high();
 38      }
 39      _delay_us(410);
 40      // Linia musi wrócić do stanu wysokiego, inaczej jest zwarta do masy
 41      return presence && bus_high();
 42  }
```

- **Linia 28:** Funkcja resetu linii. Zwraca 1, gdy czujnik odpowiedział.
- **Linia 29:** Początek funkcji.
- **Linia 30:** Zmienna na wynik sprawdzenia obecności.
- **Linia 32:** Ściągnij linię do masy.
- **Linia 33:** Trzymaj ją tak przez 480 µs. Przerwania mogą ten czas wydłużyć, co nie szkodzi – reset może być dłuższy.
- **Linia 34:** Od tej chwili bez przerwań, bo moment odczytu musi być dokładny.
- **Linia 35:** Puść linię.
- **Linia 36:** Odczekaj 70 µs. Czujnik w tym czasie ściąga linię, sygnalizując obecność.
- **Linia 37:** Jeśli linia jest niska, czujnik jest obecny. `!` odwraca wynik `bus_high()`.
- **Linia 38:** Koniec bloku bez przerwań.
- **Linia 39:** Odczekaj do końca okna obecności (razem z poprzednim czasem ok. 480 µs).
- **Linia 40:** Komentarz: linia musi wrócić do stanu wysokiego, inaczej jest zwarta do masy.
- **Linia 41:** Wynik: czujnik był obecny i linia jest teraz wolna. Bez drugiego warunku zwarta linia udawałaby obecny czujnik.
- **Linia 42:** Koniec funkcji.

### Jeden bit

```c
 44  // Szczelina czasowa: zapis bitu; zapis jedynki jest jednocześnie odczytem
 45  static uint8_t bus_bit(uint8_t bit)
 46  {
 47      uint8_t r;
 48
 49      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 50          bus_low();
 51          _delay_us(1);
 52          if (bit)
 53              bus_release();
 54          _delay_us(SAMPLE_US);
 55          r = bus_high();
 56          _delay_us(55);
 57          bus_release();
 58      }
 59      _delay_us(2);
 60      return r;
 61  }
```

- **Linia 44:** Komentarz: szczelina czasowa przesyła jeden bit. Wysłanie jedynki jest jednocześnie odczytem, bo czujnik może wtedy ściągnąć linię.
- **Linia 45:** Funkcja wysyłająca bit `bit` i zwracająca stan linii odczytany w szczelinie.
- **Linia 46:** Początek funkcji.
- **Linia 47:** Zmienna na odczytany bit.
- **Linia 49:** Cała szczelina bez przerwań, bo trwa tylko ok. 60 µs, a czasy są ciasne.
- **Linia 50:** Ściągnij linię – początek szczeliny.
- **Linia 51:** Krótka chwila (co najmniej 1 µs), żeby czujnik zauważył początek szczeliny.
- **Linia 52:** Jeśli wysyłamy jedynkę (albo czytamy)...
- **Linia 53:** ...od razu puść linię.
- **Linia 54:** Odczekaj do chwili odczytu, wciąż przed upływem 15 µs od początku szczeliny.
- **Linia 55:** Odczytaj stan linii. Przy wysyłaniu zera wynik nie ma znaczenia.
- **Linia 56:** Odczekaj do końca szczeliny (razem ok. 60 µs). Przy wysyłaniu zera linia jest przez ten czas cały czas niska.
- **Linia 57:** Puść linię (przy zerze dopiero teraz).
- **Linia 58:** Koniec bloku bez przerwań.
- **Linia 59:** Krótka przerwa między szczelinami, której wymaga czujnik.
- **Linia 60:** Zwróć odczytany bit.
- **Linia 61:** Koniec funkcji.

### Jeden bajt

```c
 63  // Wysyła bajt (LSB pierwszy) i zwraca odczytane bity; 0xFF oznacza sam odczyt
 64  static uint8_t bus_byte(uint8_t b)
 65  {
 66      uint8_t i;
 67
 68      for (i = 0; i < 8; i++)
 69          b = (uint8_t)((b >> 1) | (bus_bit(b & 1) ? 0x80 : 0));
 70      return b;
 71  }
```

- **Linia 63:** Komentarz: funkcja wysyła bajt od najmłodszego bitu i jednocześnie zbiera odczytane bity. Wysłanie `0xFF` (samych jedynek) oznacza czysty odczyt.
- **Linia 64:** Funkcja `bus_byte`.
- **Linia 65:** Początek funkcji.
- **Linia 66:** Licznik bitów.
- **Linia 68:** Powtórz 8 razy.
- **Linia 69:** Wyślij najmłodszy bit (`b & 1`), przesuń bajt w prawo i na najstarsze miejsce wstaw odczytany bit. Po 8 obrotach w `b` są same odczytane bity.
- **Linia 70:** Zwróć odczytany bajt.
- **Linia 71:** Koniec funkcji.

### Sprawdzenie i przeliczenie wyniku

```c
 73  static int16_t decode(void)
 74  {
 75      uint8_t crc = 0;
 76      uint8_t i;
 77      int16_t raw;
 78
 79      // CRC liczone razem z ostatnim bajtem (CRC) daje 0
 80      for (i = 0; i < sizeof(scratch); i++)
 81          crc = _crc_ibutton_update(crc, scratch[i]);
 82      if (crc != 0)
 83          return TEMP_NONE;
 84      // 1/16 °C -> 0,1 °C z zaokrągleniem
 85      raw = (int16_t)((scratch[1] << 8) | scratch[0]);
 86      return (int16_t)((raw * 10 + (raw < 0 ? -8 : 8)) / 16);
 87  }
```

- **Linia 73:** Funkcja zamieniająca 9 bajtów z czujnika na temperaturę.
- **Linia 74:** Początek funkcji.
- **Linia 75:** Suma kontrolna, zaczynamy od zera.
- **Linia 76:** Licznik bajtów.
- **Linia 77:** Surowy wynik z czujnika.
- **Linia 79:** Komentarz: suma CRC policzona ze wszystkich 9 bajtów, łącznie z bajtem sumy, wychodzi 0, jeśli dane są poprawne.
- **Linia 80:** Dla każdego z 9 bajtów...
- **Linia 81:** ...dolicz go do sumy kontrolnej.
- **Linia 82:** Jeśli suma nie jest zerem, dane zostały przekłamane...
- **Linia 83:** ...więc zwróć „brak odczytu”.
- **Linia 84:** Komentarz: czujnik podaje temperaturę w szesnastych częściach stopnia, a my potrzebujemy dziesiątych.
- **Linia 85:** Złóż wynik z dwóch pierwszych bajtów: bajt 1 to starsza część, bajt 0 młodsza. `(int16_t)` sprawia, że temperatury ujemne wychodzą ujemne.
- **Linia 86:** Przelicz: razy 10, dzielone przez 16. Dodanie 8 (albo odjęcie dla ujemnych) zaokrągla do najbliższej dziesiątej części stopnia. Przykład: 22,0 °C to 352 szesnastych, (352 · 10 + 8) / 16 = 220, czyli 22,0 °C.
- **Linia 87:** Koniec funkcji.

### Kolejne kroki pomiaru

```c
 89  void ds18b20_tick(void)
 90  {
 91      uint8_t s;
 92
 93      if (wait_ms > 0) {
 94          wait_ms--;
 95          return;
 96      }
 97      s = step++;
 98      switch (s) {
 99      case 0:
100      case 3:
101          if (!bus_reset()) {
102              tenths = TEMP_NONE;
103              step = 0;
104              wait_ms = PERIOD_MS;
105          }
106          break;
107      case 1:
108      case 4:
109          // Na linii jest jeden czujnik, więc bez adresu
110          bus_byte(CMD_SKIP_ROM);
111          break;
112      case 2:
113          bus_byte(CMD_CONVERT);
114          wait_ms = CONVERT_MS;
115          break;
116      case 5:
117          bus_byte(CMD_READ);
118          break;
119      default:
120          s -= 6;
121          scratch[s] = bus_byte(0xFF);
122          if (s == sizeof(scratch) - 1) {
123              tenths = decode();
124              step = 0;
125              wait_ms = PERIOD_MS - CONVERT_MS;
126          }
127          break;
128      }
129  }
```

- **Linia 89:** Funkcja wywoływana co 1 ms z pętli głównej.
- **Linia 90:** Początek funkcji.
- **Linia 91:** Zmienna na numer bieżącego kroku.
- **Linia 93:** Jeśli trzeba jeszcze czekać...
- **Linia 94:** ...zmniejsz czas oczekiwania o 1 ms...
- **Linia 95:** ...i nic więcej nie rób.
- **Linia 96:** Koniec warunku.
- **Linia 97:** Zapamiętaj numer kroku i od razu przygotuj następny.
- **Linia 98:** Wybierz działanie według numeru kroku.
- **Linia 99:** Krok 0 (początek pomiaru)...
- **Linia 100:** ...i krok 3 (początek odczytu) to reset linii.
- **Linia 101:** Jeśli czujnik nie odpowiedział...
- **Linia 102:** ...zapisz „brak odczytu” (protokół wyśle `temp=ERR`)...
- **Linia 103:** ...zacznij od początku...
- **Linia 104:** ...za sekundę.
- **Linia 105:** Koniec warunku.
- **Linia 106:** Koniec tego kroku.
- **Linia 107:** Krok 1...
- **Linia 108:** ...i krok 4:
- **Linia 109:** Komentarz: na linii jest jeden czujnik, więc nie podajemy jego adresu.
- **Linia 110:** Wyślij komendę „pomiń adres”.
- **Linia 111:** Koniec kroku.
- **Linia 112:** Krok 2:
- **Linia 113:** Wyślij komendę „zmierz temperaturę”.
- **Linia 114:** Kolejny krok dopiero za 750 ms, gdy pomiar będzie gotowy.
- **Linia 115:** Koniec kroku.
- **Linia 116:** Krok 5:
- **Linia 117:** Wyślij komendę „odczytaj pamięć”.
- **Linia 118:** Koniec kroku.
- **Linia 119:** Kroki 6–14:
- **Linia 120:** Zamień numer kroku na numer bajtu: krok 6 to bajt 0, krok 14 to bajt 8.
- **Linia 121:** Odczytaj jeden bajt.
- **Linia 122:** Jeśli to był ostatni, dziewiąty bajt...
- **Linia 123:** ...sprawdź sumę kontrolną i przelicz temperaturę...
- **Linia 124:** ...wróć do kroku 0...
- **Linia 125:** ...po odczekaniu reszty sekundy, by pomiary szły co 1 s.
- **Linia 126:** Koniec warunku.
- **Linia 127:** Koniec kroku.
- **Linia 128:** Koniec wyboru.
- **Linia 129:** Koniec funkcji.

### Odczyt wyniku

```c
131  int16_t ds18b20_tenths(void)
132  {
133      return tenths;
134  }
```

- **Linia 131:** Funkcja zwracająca ostatni wynik w dziesiątych częściach stopnia albo `TEMP_NONE`.
- **Linia 132:** Początek funkcji.
- **Linia 133:** Zwróć zapamiętaną temperaturę.
- **Linia 134:** Koniec funkcji.

---

[← Poprzedni](21-firmware-demo.md) · [Spis treści](README.md) · [Następny →](30-serwer-start.md)
