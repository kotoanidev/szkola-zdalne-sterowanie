# Firmware: `timer.c`, zegar programu

[← Poprzedni](12-firmware-main.md) · [Spis treści](README.md) · [Następny →](14-firmware-uart.md)

**Po co ten plik:** program musi wiedzieć, ile minęło czasu. Timer2 to sprzętowy licznik, który co 1 ms zgłasza przerwanie, a to przerwanie zwiększa zmienną `ms`. Dzięki temu cały firmware ma wspólny „zegarek”.

**Rachunek:** zegar układu to 1 000 000 taktów na sekundę. Preskaler 8 daje 125 000 zliczeń na sekundę. Przerwanie co 125 zliczeń daje dokładnie 1000 przerwań na sekundę, czyli jedno na milisekundę.

## `firmware/src/timer.c`

[Otwórz ten plik w repozytorium](../../firmware/src/timer.c)

```c
  1  // Takt systemowy 1 ms z Timer2 (CTC)
  2  #include <avr/io.h>
  3  #include <avr/interrupt.h>
  4  #include <util/atomic.h>
  5  #include "timer.h"
  6
  7  static volatile uint32_t ms;
```

- **Linia 1:** Komentarz: takt co 1 ms pochodzi z Timer2 w trybie CTC. CTC oznacza „zeruj licznik po osiągnięciu zadanej wartości”.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Biblioteka przerwań, dostarcza zapis `ISR`.
- **Linia 4:** Biblioteka z `ATOMIC_BLOCK`, czyli blokiem bez przerwań.
- **Linia 5:** Nasz nagłówek z zapowiedziami funkcji.
- **Linia 7:** Licznik milisekund od startu. `static`: widoczny tylko w tym pliku. `volatile`: zmienia się w przerwaniu, więc kompilator ma zawsze czytać go z pamięci. `uint32_t`: 32 bity, wystarczą na około 49 dni.

### Przerwanie

```c
  9  ISR(TIMER2_COMP_vect)
 10  {
 11      ms++;
 12  }
```

- **Linia 9:** Funkcja obsługi przerwania `TIMER2_COMP_vect`, czyli „Timer2 doliczył do zadanej wartości”. Sprzęt uruchamia ją sam co 1 ms.
- **Linia 10:** Początek funkcji.
- **Linia 11:** Zwiększ licznik o jeden.
- **Linia 12:** Koniec funkcji. Procesor wraca do przerwanej pracy.

### Uruchomienie zegara

```c
 14  void timer_init(void)
 15  {
 16      TCCR2 = _BV(WGM21) | _BV(CS21);
 17      OCR2 = (uint8_t)(F_CPU / 8 / 1000 - 1);
 18      TIMSK |= _BV(OCIE2);
 19  }
```

- **Linia 14:** Początek funkcji `timer_init`.
- **Linia 15:** Początek treści funkcji.
- **Linia 16:** `TCCR2` to rejestr sterujący Timer2. `WGM21` wybiera tryb CTC, a `CS21` preskaler 8. Od teraz licznik pracuje z tempem 125 000 zliczeń na sekundę.
- **Linia 17:** `OCR2` to wartość, po której licznik się zeruje. Liczy się ją tak: 1 000 000 / 8 / 1000 - 1 = 124. Liczenie od 0 do 124 to 125 kroków. Odejmujemy 1, bo zero też jest krokiem.
- **Linia 18:** `TIMSK` to rejestr włączania przerwań timerów. Bit `OCIE2` włącza przerwanie po osiągnięciu `OCR2`. `|=` nie rusza pozostałych bitów.
- **Linia 19:** Koniec funkcji.

### Odczyt czasu

Procesor ATmega8 jest 8-bitowy i czyta liczbę 32-bitową w czterech krokach. Gdyby w połowie tych kroków przyszło przerwanie zmieniające `ms`, wynik byłby mieszanką starej i nowej wartości. Dlatego odczyt jest chroniony.

```c
 21  uint32_t timer_ms(void)
 22  {
 23      uint32_t t;
 24      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 25          t = ms;
 26      }
 27      return t;
 28  }
```

- **Linia 21:** Funkcja `timer_ms` zwraca czas w milisekundach jako liczbę 32-bitową.
- **Linia 22:** Początek funkcji.
- **Linia 23:** Zmienna na kopię licznika.
- **Linia 24:** `ATOMIC_BLOCK`: na czas wykonania bloku wyłącz przerwania. `ATOMIC_RESTORESTATE` mówi, że potem ich stan ma wrócić do poprzedniego.
- **Linia 25:** Skopiuj licznik. Tu żadne przerwanie nie może przeszkodzić.
- **Linia 26:** Koniec bloku. Przerwania działają jak wcześniej.
- **Linia 27:** Zwróć kopię.
- **Linia 28:** Koniec funkcji.

```c
 30  uint16_t timer_ms16(void)
 31  {
 32      uint16_t t;
 33      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
 34          t = (uint16_t)ms;
 35      }
 36      return t;
 37  }
```

- **Linia 30:** Funkcja `timer_ms16` zwraca tylko dolne 16 bitów czasu. Takie liczby są dla układu tańsze w obliczeniach. Przekręcają się co 65,5 sekundy, ale do mierzenia krótkich odstępów to nie przeszkadza.
- **Linia 31:** Początek funkcji.
- **Linia 32:** Zmienna na kopię.
- **Linia 33:** Blok bez przerwań, jak wyżej.
- **Linia 34:** Skopiuj licznik. `(uint16_t)` obcina go do dolnych 16 bitów.
- **Linia 35:** Koniec bloku.
- **Linia 36:** Zwróć kopię.
- **Linia 37:** Koniec funkcji.

---

[← Poprzedni](12-firmware-main.md) · [Spis treści](README.md) · [Następny →](14-firmware-uart.md)
