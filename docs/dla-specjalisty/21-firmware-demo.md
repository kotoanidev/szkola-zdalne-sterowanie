# Firmware: `demo/etap1.c`, program pokazowy

[← Poprzedni](20-firmware-state.md) · [Spis treści](README.md) · [Następny →](22-firmware-ds18b20.md)

**Po co ten plik:** to mały, osobny program z etapu 1. Pokazuje samą podstawę sterowania układem: miganie diodą i odczyt przycisku za pomocą rejestrów `DDRx`, `PORTx` i `PINx`. **Jeśli dopiero zaczynasz, przeczytaj go jako pierwszy kod w C**, bo jest najprostszy.

## `firmware/demo/etap1.c`

[Otwórz ten plik w repozytorium](../../firmware/demo/etap1.c)

```c
  1  // Demonstracja etapu 1: miganie diody i przełączanie przyciskiem, tylko DDRx/PORTx/PINx.
  2  // make demo && make flash-demo
  3  #include <avr/io.h>
  4  #include <util/delay.h>
  5  #include "pinout.h"
```

- **Linia 1:** Komentarz: program migający diodą i przełączający ją przyciskiem, tylko na rejestrach `DDRx`, `PORTx`, `PINx`.
- **Linia 2:** Komentarz: jak go zbudować i wgrać.
- **Linia 3:** Biblioteka z nazwami rejestrów układu.
- **Linia 4:** Biblioteka z opóźnieniami, tu funkcja `_delay_ms`.
- **Linia 5:** Nasz plik z przypisaniem pinów.

```c
  7  int main(void)
  8  {
  9      uint8_t on = 0;
 10      uint8_t prev = 1;
 11      uint8_t i;
```

- **Linia 7:** Funkcja główna. Od niej zaczyna się program.
- **Linia 8:** Początek funkcji.
- **Linia 9:** Czy dioda ma świecić. Na początku 0, czyli nie.
- **Linia 10:** Poprzedni odczyt przycisku. Na początku 1, czyli niewciśnięty.
- **Linia 11:** Licznik do pętli.

```c
 13      LED_DDR |= _BV(LED_BIT);
 14      STATUS_DDR |= _BV(STATUS_BIT);
 15      BTN_DDR &= ~_BV(BTN_BIT);
 16      BTN_PORT |= _BV(BTN_BIT);
```

- **Linia 13:** Pin diody głównej ustaw jako wyjście: włącz jego bit w rejestrze kierunku.
- **Linia 14:** Pin diody statusu ustaw jako wyjście.
- **Linia 15:** Pin przycisku ustaw jako wejście: skasuj jego bit.
- **Linia 16:** Włącz pull-up na pinie przycisku: bez naciśnięcia pin ma stan wysoki.

```c
 18      // Na start trzy mignięcia diody głównej
 19      for (i = 0; i < 6; i++) {
 20          LED_PORT ^= _BV(LED_BIT);
 21          _delay_ms(250);
 22      }
```

- **Linia 18:** Komentarz: na początek trzy mignięcia diodą główną.
- **Linia 19:** Pętla wykona się 6 razy, dla `i` od 0 do 5. Jedno mignięcie to zapalenie i zgaszenie, czyli 2 kroki.
- **Linia 20:** Odwróć stan diody operatorem XOR: zgaszona się zapala, a zapalona gaśnie.
- **Linia 21:** Poczekaj 250 ms. Nic innego w tym czasie się nie dzieje.
- **Linia 22:** Koniec pętli.

```c
 24      for (i = 0;; i++) {
 25          uint8_t now = (BTN_PIN & _BV(BTN_BIT)) ? 1 : 0;
 26
 27          if (prev && !now) {
 28              on = !on;
 29              if (on)
 30                  LED_PORT |= _BV(LED_BIT);
 31              else
 32                  LED_PORT &= ~_BV(LED_BIT);
 33          }
 34          prev = now;
 35
 36          // Dioda statusu miga co 0,5 s, pętla co 10 ms (opóźnienie tłumi drgania styków)
 37          if (i == 50) {
 38              i = 0;
 39              STATUS_PORT ^= _BV(STATUS_BIT);
 40          }
 41          _delay_ms(10);
 42      }
 43  }
```

- **Linia 24:** Pętla bez końca, bo brakuje warunku. Licznik `i` zaczyna od 0 i rośnie z każdym obrotem.
- **Linia 25:** Odczytaj przycisk: 1 lub 0. Operator `? :` zamienia dowolną wartość niezerową na 1.
- **Linia 27:** Jeśli poprzednio był niewciśnięty, a teraz jest wciśnięty, to mamy chwilę naciśnięcia...
- **Linia 28:** ...odwróć stan.
- **Linia 29:** Jeśli ma świecić...
- **Linia 30:** ...podaj 5 V na pin diody.
- **Linia 31:** W przeciwnym razie:
- **Linia 32:** Podaj 0 V na pin diody.
- **Linia 33:** Koniec reakcji na naciśnięcie.
- **Linia 34:** Zapamiętaj bieżący odczyt na następny obrót.
- **Linia 36:** Komentarz: dioda statusu miga co 0,5 s, pętla obraca się co 10 ms, a to opóźnienie przy okazji tłumi drgania styków.
- **Linia 37:** Jeśli licznik doszedł do 50, czyli minęło 50 × 10 ms = 0,5 s...
- **Linia 38:** ...wyzeruj go...
- **Linia 39:** ...i odwróć stan diody statusu.
- **Linia 40:** Koniec warunku.
- **Linia 41:** Poczekaj 10 ms.
- **Linia 42:** Koniec pętli.
- **Linia 43:** Koniec funkcji.

---

[← Poprzedni](20-firmware-state.md) · [Spis treści](README.md) · [Następny →](22-firmware-ds18b20.md)
