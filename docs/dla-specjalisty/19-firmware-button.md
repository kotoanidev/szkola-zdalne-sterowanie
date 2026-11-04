# Firmware: `button.c`, przycisk

[← Poprzedni](18-firmware-sensors.md) · [Spis treści](README.md) · [Następny →](20-firmware-state.md)

**Po co ten plik:** rozpoznaje krótkie naciśnięcie (przełącz wyjście) i długie przytrzymanie (zmień tryb), a jednocześnie ignoruje drgania styków.

**Jak działa filtr drgań (tzw. integrator):** mamy licznik od 0 do 20. Co 1 ms, gdy przycisk jest wciśnięty, licznik rośnie, a gdy puszczony, maleje. Stan zmieniamy dopiero, gdy licznik dojdzie do końca skali. Krótkie drgania tylko kołyszą nim w środku zakresu i niczego nie przełączają.

## `firmware/src/button.c`

[Otwórz ten plik w repozytorium](../../firmware/src/button.c)

```c
  1  // Przycisk z programowym debouncingiem (integrator próbkowany co 1 ms)
  2  #include <avr/io.h>
  3  #include "config.h"
  4  #include "pinout.h"
  5  #include "button.h"
  6
  7  static uint8_t integrator;
  8  static uint8_t pressed;
  9  static uint8_t long_sent;
 10  static uint16_t held_ms;
```

- **Linia 1:** Komentarz: przycisk z programowym usuwaniem drgań, próbkowany co 1 ms.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Nasze ustawienia, m.in. `BTN_DEBOUNCE_MS`.
- **Linia 4:** Nasze przypisanie pinów.
- **Linia 5:** Nasz nagłówek, m.in. nazwy wyników `BTN_SHORT`, `BTN_LONG`.
- **Linia 7:** Licznik filtru drgań.
- **Linia 8:** Czy przycisk uznajemy za wciśnięty.
- **Linia 9:** Czy zgłoszono już długie naciśnięcie, by nie zgłosić go wiele razy.
- **Linia 10:** Jak długo przycisk jest trzymany, w milisekundach.

```c
 12  void button_init(void)
 13  {
 14      BTN_DDR &= ~_BV(BTN_BIT);
 15      BTN_PORT |= _BV(BTN_BIT);
 16  }
```

- **Linia 12:** Początek funkcji `button_init`.
- **Linia 13:** Początek treści funkcji.
- **Linia 14:** Ustaw pin jako wejście, kasując jego bit w rejestrze kierunku.
- **Linia 15:** Włącz wbudowany rezystor pull-up. Dla pinu wejściowego robi to jedynka w rejestrze `PORT`.
- **Linia 16:** Koniec funkcji.

```c
 18  uint8_t button_tick(void)
 19  {
 20      if (!(BTN_PIN & _BV(BTN_BIT))) {
 21          if (integrator < BTN_DEBOUNCE_MS)
 22              integrator++;
 23      } else if (integrator > 0) {
 24          integrator--;
 25      }
```

- **Linia 18:** Funkcja wywoływana co 1 ms. Zwraca wynik: nic, krótkie lub długie naciśnięcie.
- **Linia 19:** Początek funkcji.
- **Linia 20:** Odczytaj pin. Przycisk łączy go z masą, więc bit równy 0 oznacza wciśnięcie. Wykrzyknik odwraca warunek.
- **Linia 21:** Jeśli licznik nie doszedł do końca skali...
- **Linia 22:** ...zwiększ go.
- **Linia 23:** W przeciwnym razie, gdy puszczony: jeśli licznik jest powyżej zera...
- **Linia 24:** ...zmniejsz go.
- **Linia 25:** Koniec warunku.

```c
 27      if (!pressed && integrator == BTN_DEBOUNCE_MS) {
 28          pressed = 1;
 29          long_sent = 0;
 30          held_ms = 0;
 31      } else if (pressed && integrator == 0) {
 32          pressed = 0;
 33          // Krótkie naciśnięcie zgłaszamy przy puszczeniu, żeby odróżnić je od długiego
 34          if (!long_sent)
 35              return BTN_SHORT;
 36      }
```

- **Linia 27:** Jeśli przycisk nie był wciśnięty, a licznik doszedł do końca skali, czyli stan jest stabilny...
- **Linia 28:** ...uznaj go za wciśnięty...
- **Linia 29:** ...skasuj znacznik długiego naciśnięcia...
- **Linia 30:** ...i zacznij mierzyć czas trzymania od zera.
- **Linia 31:** Jeśli przycisk był wciśnięty, a licznik zszedł do zera...
- **Linia 32:** ...uznaj go za puszczony.
- **Linia 33:** Komentarz: krótkie naciśnięcie zgłaszamy dopiero przy puszczeniu, bo wcześniej nie wiadomo, czy nie będzie długie.
- **Linia 34:** Jeśli nie zgłoszono długiego naciśnięcia...
- **Linia 35:** ...zwróć krótkie.
- **Linia 36:** Koniec warunku.

```c
 38      if (pressed && !long_sent && ++held_ms >= BTN_LONG_MS) {
 39          long_sent = 1;
 40          return BTN_LONG;
 41      }
 42      return BTN_NONE;
 43  }
```

- **Linia 38:** Trzy warunki naraz: przycisk wciśnięty, długiego jeszcze nie było, a czas trzymania po zwiększeniu o 1 ms sięgnął 1000 ms...
- **Linia 39:** ...zaznacz, że już je zgłoszono...
- **Linia 40:** ...i zwróć długie naciśnięcie.
- **Linia 41:** Koniec warunku.
- **Linia 42:** Nic się nie stało, zwróć `BTN_NONE`.
- **Linia 43:** Koniec funkcji.

---

[← Poprzedni](18-firmware-sensors.md) · [Spis treści](README.md) · [Następny →](20-firmware-state.md)
