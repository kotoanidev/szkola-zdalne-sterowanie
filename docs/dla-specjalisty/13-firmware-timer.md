# Firmware: `timer.c`, zegar programu

[← Poprzedni](12-firmware-main.md) · [Spis treści](README.md) · [Następny →](14-firmware-uart.md)

**Po co ten plik:** program musi wiedzieć, ile minęło czasu. Timer2 to sprzętowy licznik, który co 1 ms zgłasza przerwanie, a to przerwanie zwiększa zmienną `ms`. Dzięki temu cały firmware ma wspólny „zegarek”.

**Rachunek:** przy zegarze 1 MHz układ wykonuje 1 000 000 taktów na sekundę. Preskaler 8 daje 125 000 zliczeń na sekundę. Przerwanie co 125 zliczeń daje dokładnie 1000 przerwań na sekundę, czyli jedno na milisekundę. Przy kwarcu 11,0592 MHz preskaler 8 dałby za dużą liczbę zliczeń (1382 na milisekundę, a rejestr `OCR2` mieści najwyżej 255), dlatego wtedy kod sam wybiera preskaler 64.

## `firmware/src/timer.c`

[Otwórz ten plik w repozytorium](../../firmware/src/timer.c)

```c
  1  // Takt systemowy 1 ms z Timer2 (CTC)
  2  #include <avr/io.h>
  3  #include <avr/interrupt.h>
  4  #include <util/atomic.h>
  5  #include "timer.h"
  6
  7  // OCR2 musi się zmieścić w 8 bitach: preskaler 8 przy 1 MHz, 64 przy kwarcu 11,0592 MHz
  8  #if F_CPU / 8 / 1000 <= 256
  9  #define T2_DIV 8
 10  #define T2_CS  _BV(CS21)
 11  #elif F_CPU / 64 / 1000 <= 256
 12  #define T2_DIV 64
 13  #define T2_CS  _BV(CS22)
 14  #else
 15  #error "Nieobsługiwane F_CPU (timer.c)"
 16  #endif
 17
 18  static volatile uint32_t ms;
```

- **Linia 1:** Komentarz: takt co 1 ms pochodzi z Timer2 w trybie CTC. CTC oznacza „zeruj licznik po osiągnięciu zadanej wartości”.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Biblioteka przerwań, dostarcza zapis `ISR`.
- **Linia 4:** Biblioteka z `ATOMIC_BLOCK`, czyli blokiem bez przerwań.
- **Linia 5:** Nasz nagłówek z zapowiedziami funkcji.
- **Linia 7:** Komentarz: wartość `OCR2` musi zmieścić się w 8 bitach, więc preskaler zależy od zegara.
- **Linia 8:** `#if` to warunek sprawdzany przez kompilator, nie przez układ. Jeśli przy preskalerze 8 na jedną milisekundę wypada najwyżej 256 zliczeń (tak jest przy 1 MHz)...
- **Linia 9:** ...to dzielnik wynosi 8...
- **Linia 10:** ...a w rejestrze sterującym ustawiamy bit `CS21`, który wybiera preskaler 8.
- **Linia 11:** `#elif` znaczy „w przeciwnym razie, jeśli”. Gdy przy preskalerze 64 wypada najwyżej 256 zliczeń (tak jest przy 11,0592 MHz)...
- **Linia 12:** ...dzielnik wynosi 64...
- **Linia 13:** ...a bit `CS22` wybiera preskaler 64.
- **Linia 14:** W pozostałych przypadkach...
- **Linia 15:** ...kompilacja kończy się błędem z tym komunikatem. Lepiej, żeby program się nie skompilował, niż żeby zegar chodził źle.
- **Linia 16:** Koniec warunku.
- **Linia 18:** Licznik milisekund od startu. `static`: widoczny tylko w tym pliku. `volatile`: zmienia się w przerwaniu, więc kompilator ma zawsze czytać go z pamięci. `uint32_t`: 32 bity, wystarczą na około 49 dni.

### Przerwanie

```c
 20  ISR(TIMER2_COMP_vect)
 21  {
 22      ms++;
 23  }
```

- **Linia 20:** Funkcja obsługi przerwania `TIMER2_COMP_vect`, czyli „Timer2 doliczył do zadanej wartości”. Sprzęt uruchamia ją sam co 1 ms.
- **Linia 21:** Początek funkcji.
- **Linia 22:** Zwiększ licznik o jeden.
- **Linia 23:** Koniec funkcji. Procesor wraca do przerwanej pracy.

### Uruchomienie zegara

```c
 25  void timer_init(void)
 26  {
 27      TCCR2 = _BV(WGM21) | T2_CS;
 28      OCR2 = (uint8_t)((F_CPU / T2_DIV + 500) / 1000 - 1);
 29      TIMSK |= _BV(OCIE2);
 30  }
```

- **Linia 25:** Początek funkcji `timer_init`.
- **Linia 26:** Początek treści funkcji.
- **Linia 27:** `TCCR2` to rejestr sterujący Timer2. `WGM21` wybiera tryb CTC, a `T2_CS` preskaler wybrany wyżej. Przy 1 MHz licznik pracuje z tempem 125 000 zliczeń na sekundę.
- **Linia 28:** `OCR2` to wartość, po której licznik się zeruje. Przy 1 MHz: (1 000 000 / 8 + 500) / 1000 - 1 = 124. Liczenie od 0 do 124 to 125 kroków. Odejmujemy 1, bo zero też jest krokiem, a dodanie 500 zaokrągla wynik dzielenia. Przy 11,0592 MHz wychodzi 172, czyli takt co 1,001 ms.
- **Linia 29:** `TIMSK` to rejestr włączania przerwań timerów. Bit `OCIE2` włącza przerwanie po osiągnięciu `OCR2`. `|=` nie rusza pozostałych bitów.
- **Linia 30:** Koniec funkcji.

### Odczyt czasu

Procesor ATmega32A jest 8-bitowy i czyta liczbę 32-bitową w czterech krokach. Gdyby w połowie tych kroków przyszło przerwanie zmieniające `ms`, wynik byłby mieszanką starej i nowej wartości. Dlatego odczyt jest chroniony.

```c
 32  uint32_t timer_ms(void)
 33  {
 34      uint32_t t;
 35      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 36          t = ms;
 37      }
 38      return t;
 39  }
```

- **Linia 32:** Funkcja `timer_ms` zwraca czas w milisekundach jako liczbę 32-bitową.
- **Linia 33:** Początek funkcji.
- **Linia 34:** Zmienna na kopię licznika.
- **Linia 35:** `ATOMIC_BLOCK`: na czas wykonania bloku wyłącz przerwania. `ATOMIC_RESTORESTATE` mówi, że potem ich stan ma wrócić do poprzedniego.
- **Linia 36:** Skopiuj licznik. Tu żadne przerwanie nie może przeszkodzić.
- **Linia 37:** Koniec bloku. Przerwania działają jak wcześniej.
- **Linia 38:** Zwróć kopię.
- **Linia 39:** Koniec funkcji.

```c
 41  uint16_t timer_ms16(void)
 42  {
 43      uint16_t t;
 44      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 45          t = (uint16_t)ms;
 46      }
 47      return t;
 48  }
```

- **Linia 41:** Funkcja `timer_ms16` zwraca tylko dolne 16 bitów czasu. Takie liczby są dla układu tańsze w obliczeniach. Przekręcają się co 65,5 sekundy, ale do mierzenia krótkich odstępów to nie przeszkadza.
- **Linia 42:** Początek funkcji.
- **Linia 43:** Zmienna na kopię.
- **Linia 44:** Blok bez przerwań, jak wyżej.
- **Linia 45:** Skopiuj licznik. `(uint16_t)` obcina go do dolnych 16 bitów.
- **Linia 46:** Koniec bloku.
- **Linia 47:** Zwróć kopię.
- **Linia 48:** Koniec funkcji.

---

[← Poprzedni](12-firmware-main.md) · [Spis treści](README.md) · [Następny →](14-firmware-uart.md)
