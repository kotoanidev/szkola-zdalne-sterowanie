# Firmware: `pwm.c`, jasność diody

[← Poprzedni](15-firmware-protocol.md) · [Spis treści](README.md) · [Następny →](17-firmware-adc.md)

**Po co ten plik:** dioda ma świecić z zadaną jasnością, a przejścia mają być płynne. Sprzętowy Timer1 sam generuje sygnał PWM na pinie `PB1`, więc procesor nie musi niczego przełączać ręcznie. Ten moduł ustala tylko wartość wypełnienia.

**Dwie wartości:**

- `target`: jasność, do której dążymy,
- `level`: jasność w tej chwili. Co 1 ms zbliża się o mały krok do `target`, dając płynne przejście.

**Korekcja gamma:** oko odbiera jasność nieliniowo. Wypełnienie 50 % wygląda znacznie jaśniej niż „połowa”. Dlatego wypełnienie to kwadrat jasności: przy jasności 50 % wypełnienie wynosi 25 %. Suwak działa dzięki temu równomiernie dla oka.

## `firmware/src/pwm.c`

[Otwórz ten plik w repozytorium](../../firmware/src/pwm.c)

```c
  1  // Jasność diody głównej: Timer1, fast PWM 10-bit na OC1A, płynne przejście i korekcja gamma
  2  #include <avr/io.h>
  3  #include "config.h"
  4  #include "pinout.h"
  5  #include "pwm.h"
  6
  7  #define PWM_TOP 1023
  8
  9  static uint16_t level;
 10  static uint16_t target;
```

- **Linia 1:** Komentarz: jasność diody głównej, Timer1, tryb fast PWM 10-bitowy na wyjściu `OC1A`, płynne przejście i korekcja gamma.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Nasze ustawienia, m.in. `PWM_FADE_STEP`.
- **Linia 4:** Nasze przypisanie pinów.
- **Linia 5:** Nasz nagłówek.
- **Linia 7:** Górna granica licznika PWM: 1023, bo 10 bitów daje wartości od 0 do 1023.
- **Linia 9:** Obecna jasność w skali 0–1023.
- **Linia 10:** Docelowa jasność w tej samej skali.

### Zastosowanie jasności

```c
 12  static void apply(void)
 13  {
 14      // Wypełnienie = (jasność)^2, żeby zmiana jasności była równomierna dla oka
 15      uint16_t duty = (uint16_t)(((uint32_t)level * level + PWM_TOP) >> 10);
 16
 17      if (duty == 0) {
 18          // OCR1A = 0 daje w fast PWM krótkie impulsy, więc odłączamy wyjście
 19          TCCR1A &= ~_BV(COM1A1);
 20          LED_PORT &= ~_BV(LED_BIT);
 21      } else {
 22          OCR1A = duty;
 23          TCCR1A |= _BV(COM1A1);
 24      }
 25  }
```

- **Linia 12:** Funkcja zapisująca bieżącą jasność do sprzętu. `static`: tylko dla tego pliku.
- **Linia 13:** Początek funkcji.
- **Linia 14:** Komentarz: wypełnienie to kwadrat jasności.
- **Linia 15:** Oblicza wypełnienie. `level * level` może sięgnąć około miliona, dlatego `(uint32_t)` każe liczyć na 32 bitach. `>> 10` dzieli przez 1024, wracając do skali 0–1023. Dodanie `PWM_TOP` zaokrągla w górę, więc najmniejsza niezerowa jasność daje niezerowe wypełnienie.
- **Linia 17:** Jeśli wypełnienie wynosi zero...
- **Linia 18:** Komentarz: wpisanie zera w trybie fast PWM wciąż daje bardzo krótkie impulsy, dlatego trzeba całkiem odłączyć wyjście.
- **Linia 19:** Skasuj bit `COM1A1`. Timer przestaje sterować pinem.
- **Linia 20:** Ustaw pin na 0 V. Dioda całkowicie gaśnie.
- **Linia 21:** W przeciwnym razie:
- **Linia 22:** Wpisz wypełnienie do rejestru `OCR1A`. Timer porównuje z nim swój licznik.
- **Linia 23:** Ustaw bit `COM1A1`, czyli podłącz timer do pinu. Pin ma 5 V od początku cyklu do chwili zrównania licznika z `OCR1A`.
- **Linia 24:** Koniec warunku.
- **Linia 25:** Koniec funkcji.

### Uruchomienie PWM

```c
 27  void pwm_init(void)
 28  {
 29      LED_PORT &= ~_BV(LED_BIT);
 30      LED_DDR |= _BV(LED_BIT);
 31      // Tryb 7 (fast PWM 10-bit), bez preskalera: 1 MHz / 1024 ≈ 977 Hz
 32      TCCR1A = _BV(WGM11) | _BV(WGM10);
 33      TCCR1B = _BV(WGM12) | _BV(CS10);
 34      apply();
 35  }
```

- **Linia 27:** Początek funkcji `pwm_init`.
- **Linia 28:** Początek treści funkcji.
- **Linia 29:** Najpierw ustaw 0 V w rejestrze wyjściowym, by dioda nie błysnęła przy starcie.
- **Linia 30:** Ustaw pin jako wyjście.
- **Linia 31:** Komentarz: tryb 7 (fast PWM 10-bitowy) bez preskalera, co daje 1 000 000 / 1024, czyli około 977 cykli na sekundę.
- **Linia 32:** Pierwsza część wyboru trybu: bity `WGM11` i `WGM10` w rejestrze `TCCR1A`.
- **Linia 33:** Druga część wyboru trybu: bit `WGM12` w `TCCR1B`. Bit `CS10` uruchamia timer bez preskalera.
- **Linia 34:** Zastosuj bieżącą jasność, na początku zero.
- **Linia 35:** Koniec funkcji.

### Ustawienie celu

```c
 37  void pwm_set_percent(uint8_t percent)
 38  {
 39      if (percent > 100)
 40          percent = 100;
 41      target = (uint16_t)(((uint32_t)percent * PWM_TOP + 50) / 100);
 42  }
```

- **Linia 37:** Funkcja dostaje jasność w procentach.
- **Linia 38:** Początek funkcji.
- **Linia 39:** Jeśli podano więcej niż 100...
- **Linia 40:** ...przytnij do 100, dla bezpieczeństwa.
- **Linia 41:** Przelicz procenty na skalę 0–1023. Dodanie 50 przed dzieleniem przez 100 daje zaokrąglenie, a nie obcięcie. Mnożenie na 32 bitach zapobiega przepełnieniu.
- **Linia 42:** Koniec funkcji. Sama zmiana celu niczego jeszcze nie zapala, resztę zrobi `pwm_tick`.

### Krok płynnej zmiany

```c
 44  void pwm_tick(void)
 45  {
 46      if (level == target)
 47          return;
 48      if (level < target)
 49          level = (target - level > PWM_FADE_STEP) ? level + PWM_FADE_STEP : target;
 50      else
 51          level = (level - target > PWM_FADE_STEP) ? level - PWM_FADE_STEP : target;
 52      apply();
 53  }
```

- **Linia 44:** Funkcja wywoływana co 1 ms.
- **Linia 45:** Początek funkcji.
- **Linia 46:** Jeśli jasność już jest równa celowi...
- **Linia 47:** ...nie ma nic do zrobienia.
- **Linia 48:** Jeśli jasność jest poniżej celu...
- **Linia 49:** ...zwiększ ją o krok, a jeśli do celu zostało mniej niż krok, ustaw od razu cel. Dzięki temu nie przeskoczymy celu.
- **Linia 50:** W przeciwnym razie, czyli jasność jest powyżej celu:
- **Linia 51:** Zmniejsz ją analogicznie.
- **Linia 52:** Zapisz nową jasność do sprzętu.
- **Linia 53:** Koniec funkcji.

---

[← Poprzedni](15-firmware-protocol.md) · [Spis treści](README.md) · [Następny →](17-firmware-adc.md)
