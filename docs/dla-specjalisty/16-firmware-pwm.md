# Firmware: `pwm.c`, jasność diody

[← Poprzedni](15-firmware-protocol.md) · [Spis treści](README.md) · [Następny →](17-firmware-adc.md)

**Po co ten plik:** dioda ma świecić z zadaną jasnością, a przejścia mają być płynne. Sprzętowy Timer1 sam generuje sygnał PWM na pinie `PD5` (wyjście `OC1A` w ATmega32A), więc procesor nie musi niczego przełączać ręcznie. Ten moduł ustala tylko wartość wypełnienia.

**Dioda świecąca stanem niskim:** na płytce ATB diody mają najpewniej anodę podłączoną do +5 V, a katodę przez zworkę do pinu (do potwierdzenia testem z `docs/sprzet.md`, rozdz. 6). Dioda świeci wtedy, gdy pin ma 0 V. Timer pracuje wtedy w trybie odwracającym: im większe wypełnienie, tym dłużej pin ma 0 V. Wybiera to stała `LED_ACTIVE_LOW` w `pinout.h`.

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
  9  #if LED_ACTIVE_LOW
 10  // Tryb odwracający: dioda świeci, gdy wyjście jest w stanie niskim
 11  #define PWM_COM   (_BV(COM1A1) | _BV(COM1A0))
 12  #define led_off() (LED_PORT |= _BV(LED_BIT))
 13  #else
 14  #define PWM_COM   _BV(COM1A1)
 15  #define led_off() (LED_PORT &= ~_BV(LED_BIT))
 16  #endif
 17
 18  static uint16_t level;
 19  static uint16_t target;
```

- **Linia 1:** Komentarz: jasność diody głównej, Timer1, tryb fast PWM 10-bitowy na wyjściu `OC1A`, płynne przejście i korekcja gamma.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Nasze ustawienia, m.in. `PWM_FADE_STEP`.
- **Linia 4:** Nasze przypisanie pinów.
- **Linia 5:** Nasz nagłówek.
- **Linia 7:** Górna granica licznika PWM: 1023, bo 10 bitów daje wartości od 0 do 1023.
- **Linia 9:** Warunek sprawdzany przez kompilator: jeśli dioda świeci stanem niskim (tak jest na płytce ATB)...
- **Linia 10:** Komentarz: tryb odwracający, dioda świeci, gdy wyjście ma stan niski.
- **Linia 11:** `PWM_COM` to bity podłączające timer do pinu. Ustawione razem `COM1A1` i `COM1A0` oznaczają tryb odwracający: pin ma 0 V od początku cyklu do chwili zrównania licznika z `OCR1A`.
- **Linia 12:** `led_off()` gasi diodę bez timera: ustawia pin na 5 V, bo przy diodzie świecącej stanem niskim 5 V oznacza „zgaszona”.
- **Linia 13:** W przeciwnym razie (dioda świeci stanem wysokim)...
- **Linia 14:** ...wystarczy sam bit `COM1A1`, czyli zwykły tryb: pin ma 5 V od początku cyklu do zrównania z `OCR1A`...
- **Linia 15:** ...a gaszenie to ustawienie pinu na 0 V.
- **Linia 16:** Koniec warunku.
- **Linia 18:** Obecna jasność w skali 0–1023.
- **Linia 19:** Docelowa jasność w tej samej skali.

### Zastosowanie jasności

```c
 21  static void apply(void)
 22  {
 23      // Wypełnienie = (jasność)^2, żeby zmiana jasności była równomierna dla oka
 24      uint16_t duty = (uint16_t)(((uint32_t)level * level + PWM_TOP) >> 10);
 25
 26      if (duty == 0) {
 27          // OCR1A = 0 daje w fast PWM krótkie impulsy, więc odłączamy wyjście
 28          TCCR1A &= ~PWM_COM;
 29          led_off();
 30      } else {
 31          OCR1A = duty;
 32          TCCR1A |= PWM_COM;
 33      }
 34  }
```

- **Linia 21:** Funkcja zapisująca bieżącą jasność do sprzętu. `static`: tylko dla tego pliku.
- **Linia 22:** Początek funkcji.
- **Linia 23:** Komentarz: wypełnienie to kwadrat jasności.
- **Linia 24:** Oblicza wypełnienie. `level * level` może sięgnąć około miliona, dlatego `(uint32_t)` każe liczyć na 32 bitach. `>> 10` dzieli przez 1024, wracając do skali 0–1023. Dodanie `PWM_TOP` zaokrągla w górę, więc najmniejsza niezerowa jasność daje niezerowe wypełnienie.
- **Linia 26:** Jeśli wypełnienie wynosi zero...
- **Linia 27:** Komentarz: wpisanie zera w trybie fast PWM wciąż daje bardzo krótkie impulsy, dlatego trzeba całkiem odłączyć wyjście.
- **Linia 28:** Skasuj bity `PWM_COM`. Timer przestaje sterować pinem.
- **Linia 29:** Zgaś diodę zwykłym stanem pinu. Dioda całkowicie gaśnie.
- **Linia 30:** W przeciwnym razie:
- **Linia 31:** Wpisz wypełnienie do rejestru `OCR1A`. Timer porównuje z nim swój licznik.
- **Linia 32:** Ustaw bity `PWM_COM`, czyli podłącz timer do pinu. Dioda świeci od początku cyklu do chwili zrównania licznika z `OCR1A`.
- **Linia 33:** Koniec warunku.
- **Linia 34:** Koniec funkcji.

### Uruchomienie PWM

```c
 36  void pwm_init(void)
 37  {
 38      led_off();
 39      LED_DDR |= _BV(LED_BIT);
 40      // Tryb 7 (fast PWM 10-bit), bez preskalera: F_CPU / 1024, przy 1 MHz ok. 977 Hz
 41      TCCR1A = _BV(WGM11) | _BV(WGM10);
 42      TCCR1B = _BV(WGM12) | _BV(CS10);
 43      apply();
 44  }
```

- **Linia 36:** Początek funkcji `pwm_init`.
- **Linia 37:** Początek treści funkcji.
- **Linia 38:** Najpierw ustaw w rejestrze wyjściowym stan „zgaszona”, by dioda nie błysnęła przy starcie.
- **Linia 39:** Ustaw pin jako wyjście.
- **Linia 40:** Komentarz: tryb 7 (fast PWM 10-bitowy) bez preskalera, co daje zegar / 1024: przy 1 MHz około 977 cykli na sekundę, przy kwarcu 11,0592 MHz około 10 800.
- **Linia 41:** Pierwsza część wyboru trybu: bity `WGM11` i `WGM10` w rejestrze `TCCR1A`.
- **Linia 42:** Druga część wyboru trybu: bit `WGM12` w `TCCR1B`. Bit `CS10` uruchamia timer bez preskalera.
- **Linia 43:** Zastosuj bieżącą jasność, na początku zero.
- **Linia 44:** Koniec funkcji.

### Ustawienie celu

```c
 46  void pwm_set_percent(uint8_t percent)
 47  {
 48      if (percent > 100)
 49          percent = 100;
 50      target = (uint16_t)(((uint32_t)percent * PWM_TOP + 50) / 100);
 51  }
```

- **Linia 46:** Funkcja dostaje jasność w procentach.
- **Linia 47:** Początek funkcji.
- **Linia 48:** Jeśli podano więcej niż 100...
- **Linia 49:** ...przytnij do 100, dla bezpieczeństwa.
- **Linia 50:** Przelicz procenty na skalę 0–1023. Dodanie 50 przed dzieleniem przez 100 daje zaokrąglenie, a nie obcięcie. Mnożenie na 32 bitach zapobiega przepełnieniu.
- **Linia 51:** Koniec funkcji. Sama zmiana celu niczego jeszcze nie zapala, resztę zrobi `pwm_tick`.

### Krok płynnej zmiany

```c
 53  void pwm_tick(void)
 54  {
 55      if (level == target)
 56          return;
 57      if (level < target)
 58          level = (target - level > PWM_FADE_STEP) ? level + PWM_FADE_STEP : target;
 59      else
 60          level = (level - target > PWM_FADE_STEP) ? level - PWM_FADE_STEP : target;
 61      apply();
 62  }
```

- **Linia 53:** Funkcja wywoływana co 1 ms.
- **Linia 54:** Początek funkcji.
- **Linia 55:** Jeśli jasność już jest równa celowi...
- **Linia 56:** ...nie ma nic do zrobienia.
- **Linia 57:** Jeśli jasność jest poniżej celu...
- **Linia 58:** ...zwiększ ją o krok, a jeśli do celu zostało mniej niż krok, ustaw od razu cel. Dzięki temu nie przeskoczymy celu.
- **Linia 59:** W przeciwnym razie, czyli jasność jest powyżej celu:
- **Linia 60:** Zmniejsz ją analogicznie.
- **Linia 61:** Zapisz nową jasność do sprzętu.
- **Linia 62:** Koniec funkcji.

---

[← Poprzedni](15-firmware-protocol.md) · [Spis treści](README.md) · [Następny →](17-firmware-adc.md)
