# Firmware: `main.c`, serce programu

[← Poprzedni](11-firmware-naglowki.md) · [Spis treści](README.md) · [Następny →](13-firmware-timer.md)

**Po co ten plik:** program w układzie zaczyna się od funkcji `main`. Tutaj układ jest przygotowywany do pracy, a potem w nieskończonej pętli obsługuje odebrane znaki, pomiary i zadania wykonywane co milisekundę. Tu też zapadają decyzje: co zrobić po naciśnięciu przycisku albo ruchu potencjometru.

Układ nie „kończy” pracy jak zwykły program. Działa, dopóki ma zasilanie. Dlatego główna pętla nigdy się nie kończy.

## `firmware/src/main.c`

[Otwórz ten plik w repozytorium](../../firmware/src/main.c)

### Dołączanie potrzebnych plików

```c
  1  // Sterownik oświetlenia LED: ATmega8 @ 1 MHz, sterowanie przez UART, przycisk i potencjometr
  2  #include <avr/io.h>
  3  #include <avr/interrupt.h>
  4  #include <avr/pgmspace.h>
  5  #include <avr/wdt.h>
  6  #include "config.h"
  7  #include "pinout.h"
  8  #include "adc.h"
  9  #include "button.h"
 10  #include "protocol.h"
 11  #include "pwm.h"
 12  #include "sensors.h"
 13  #include "state.h"
 14  #include "timer.h"
 15  #include "uart.h"
```

- **Linia 1:** Komentarz: to sterownik oświetlenia LED na ATmega8 z zegarem 1 MHz. Sterować można przez UART, przycisk i potencjometr.
- **Linia 2:** Biblioteka z nazwami rejestrów układu.
- **Linia 3:** Biblioteka obsługi przerwań, m.in. `sei()`.
- **Linia 4:** Biblioteka dostępu do pamięci flash, m.in. `PSTR`.
- **Linia 5:** Biblioteka obsługi watchdoga.
- **Linia 6:** Nasze ustawienia.
- **Linia 7:** Nasze przypisanie pinów.
- **Linia 8:** Nasz moduł pomiarów ADC.
- **Linia 9:** Nasz moduł przycisku.
- **Linia 10:** Nasz moduł protokołu tekstowego.
- **Linia 11:** Nasz moduł jasności diody.
- **Linia 12:** Nasz moduł przeliczania pomiarów.
- **Linia 13:** Nasz moduł stanu urządzenia.
- **Linia 14:** Nasz moduł zegara.
- **Linia 15:** Nasz moduł komunikacji UART.

### Stałe i zmienne

```c
 17  #define POT_PERIOD_MS   50
 18  #define AUTO_PERIOD_MS  200
 19  #define TEMP_PERIOD_MS  500
 20  #define ACTIVITY_MS     50
 21
 22  static uint8_t pot_timer;
 23  static uint8_t auto_timer;
 24  static uint16_t temp_timer;
 25  static uint8_t activity_ms;
 26  #if POT_ENABLE
 27  static uint8_t pot_last = 0xFF;
 28  #endif
```

- **Linia 17:** Potencjometr sprawdzamy co 50 ms.
- **Linia 18:** Jasność w trybie AUTO przeliczamy co 200 ms.
- **Linia 19:** Temperaturę sprawdzamy co 500 ms.
- **Linia 20:** Po odebraniu komendy dioda statusu świeci przez 50 ms.
- **Linia 22:** Licznik milisekund do sprawdzenia potencjometru. `static` oznacza, że widać go tylko w tym pliku, a zaczyna od 0.
- **Linia 23:** Licznik milisekund do przeliczenia trybu AUTO.
- **Linia 24:** Licznik milisekund do sprawdzenia temperatury. Musi być 16-bitowy, bo liczy do 500, a 8 bitów mieści najwyżej 255.
- **Linia 25:** Ile milisekund jeszcze ma świecić dioda statusu po komendzie.
- **Linia 26:** Dalsza linia jest kompilowana tylko wtedy, gdy potencjometr jest włączony w `config.h`.
- **Linia 27:** Ostatnie uwzględnione położenie potencjometru. `0xFF` (255) to wartość specjalna: „jeszcze nie było odczytu”. Prawdziwy odczyt ma najwyżej 100.
- **Linia 28:** Koniec warunku z linii 26.

### Przyczyna resetu

Rejestr `MCUCSR` zawiera flagi mówiące, dlaczego układ wystartował. Ta funkcja zamienia je na napis.

```c
 30  static PGM_P reset_reason(uint8_t flags)
 31  {
 32      if (flags & _BV(WDRF))
 33          return PSTR("WDT");
 34      if (flags & _BV(BORF))
 35          return PSTR("BOR");
 36      if (flags & _BV(EXTRF))
 37          return PSTR("EXT");
 38      return PSTR("POR");
 39  }
```

- **Linia 30:** Początek funkcji. Dostaje bajt z flagami i zwraca wskaźnik na napis w pamięci flash.
- **Linia 31:** Początek treści funkcji.
- **Linia 32:** Jeśli ustawiony jest bit `WDRF`, układ zresetował watchdog.
- **Linia 33:** Zwróć napis `WDT`. `PSTR` trzyma go we flash, a nie w RAM. `return` od razu kończy funkcję.
- **Linia 34:** Jeśli ustawiony jest bit `BORF`, reset spowodował spadek napięcia.
- **Linia 35:** Zwróć napis `BOR`.
- **Linia 36:** Jeśli ustawiony jest bit `EXTRF`, reset przyszedł przez nóżkę RESET.
- **Linia 37:** Zwróć napis `EXT`.
- **Linia 38:** Żaden z powyższych warunków nie zaszedł, więc zostaje włączenie zasilania: `POR`.
- **Linia 39:** Koniec funkcji.

### Pomocnicza różnica

```c
 41  static uint8_t diff(uint8_t a, uint8_t b)
 42  {
 43      return a > b ? a - b : b - a;
 44  }
```

- **Linia 41:** Funkcja `diff` dostaje dwie liczby i zwraca ich różnicę, zawsze nieujemną.
- **Linia 42:** Początek funkcji.
- **Linia 43:** Jeśli `a` jest większe od `b`, wynikiem jest `a - b`, a inaczej `b - a`. Liczby bez znaku nie mogą być ujemne, dlatego odejmujemy zawsze mniejszą od większej.
- **Linia 44:** Koniec funkcji.

### Dioda statusu

```c
 46  // Status: krótki błysk co ok. 1 s, miganie 4 Hz przy alarmie, błysk przy komendzie
 47  static void status_tick(uint16_t now)
 48  {
 49      uint8_t lit;
 50
 51      if (state.alarm)
 52          lit = (now & 0x80) != 0;
 53      else
 54          lit = (now & 0x3FF) < 30;
 55      if (activity_ms > 0) {
 56          activity_ms--;
 57          lit = 1;
 58      }
 59      if (lit)
 60          STATUS_PORT |= _BV(STATUS_BIT);
 61      else
 62          STATUS_PORT &= ~_BV(STATUS_BIT);
 63  }
```

- **Linia 46:** Komentarz: normalnie dioda błyska krótko co około 1 s. Przy alarmie miga około 4 razy na sekundę, a przy komendzie krótko się zapala.
- **Linia 47:** Początek funkcji. Dostaje aktualny czas w milisekundach.
- **Linia 48:** Początek treści funkcji.
- **Linia 49:** Zmienna `lit`: czy dioda ma w tej chwili świecić.
- **Linia 51:** Jeśli trwa alarm temperatury...
- **Linia 52:** ...dioda świeci, gdy bit 7 czasu jest ustawiony (`0x80` to 128). Ten bit zmienia się co 128 ms, więc pełny cykl trwa 256 ms, czyli daje około 4 mignięć na sekundę.
- **Linia 53:** W przeciwnym razie, czyli bez alarmu:
- **Linia 54:** `now & 0x3FF` to reszta z dzielenia czasu przez 1024. Dioda świeci przez pierwsze 30 ms z każdych 1024 ms, czyli daje krótki błysk co około sekundę.
- **Linia 55:** Jeśli po odebranej komendzie zostało jeszcze trochę czasu świecenia...
- **Linia 56:** ...zmniejsz ten czas o 1 ms...
- **Linia 57:** ...i zapal diodę niezależnie od wcześniejszego wyniku.
- **Linia 58:** Koniec warunku z linii 55.
- **Linia 59:** Jeśli dioda ma świecić...
- **Linia 60:** ...ustaw jej bit w porcie, czyli podaj 5 V na nóżkę.
- **Linia 61:** W przeciwnym razie:
- **Linia 62:** Skasuj jej bit, czyli podaj 0 V.
- **Linia 63:** Koniec funkcji.

### Sprawdzanie potencjometru

```c
 65  static void pot_check(void)
 66  {
 67  #if POT_ENABLE
 68      uint8_t p = sensors_pot_percent();
 69
 70      // Pierwszy odczyt to punkt odniesienia – po restarcie liczy się stan z EEPROM
 71      if (pot_last == 0xFF) {
 72          pot_last = p;
 73          return;
 74      }
 75      if (p == pot_last)
 76          return;
 77      if (diff(p, pot_last) < POT_HYST && p != 0 && p != 100)
 78          return;
 79      pot_last = p;
 80      protocol_event_pot(state_set_pwm(p));
 81  #endif
 82  }
```

- **Linia 65:** Początek funkcji sprawdzającej, czy ktoś obrócił pokrętło.
- **Linia 66:** Początek treści funkcji.
- **Linia 67:** Cała treść jest kompilowana tylko wtedy, gdy potencjometr jest włączony w `config.h`.
- **Linia 68:** Odczytaj położenie potencjometru w procentach i zapisz w `p`.
- **Linia 70:** Komentarz: po restarcie pierwszy odczyt nie może zmienić jasności, bo obowiązuje stan zapisany w EEPROM.
- **Linia 71:** Jeśli to pierwszy odczyt (wartość specjalna z linii 27)...
- **Linia 72:** ...zapamiętaj go jako punkt odniesienia...
- **Linia 73:** ...i zakończ funkcję.
- **Linia 74:** Koniec warunku.
- **Linia 75:** Jeśli położenie się nie zmieniło...
- **Linia 76:** ...nie ma nic do zrobienia, zakończ.
- **Linia 77:** Jeśli zmiana jest mniejsza niż margines z `config.h`, a pokrętło nie stoi na skraju (0 lub 100)...
- **Linia 78:** ...uznaj to za szum i zakończ. Warunek skrajów pozwala zawsze dojść do pełnych 0 % i 100 %.
- **Linia 79:** Zapamiętaj nowe położenie.
- **Linia 80:** Dwa kroki: `state_set_pwm(p)` ustawia jasność i zwraca, czy zmienił się tryb, a wynik trafia do funkcji wysyłającej zdarzenie `EVT POT`.
- **Linia 81:** Koniec warunku z linii 67.
- **Linia 82:** Koniec funkcji.

### Tryb AUTO

```c
 84  static void auto_check(void)
 85  {
 86      uint8_t p;
 87
 88      if (state.mode != MODE_AUTO)
 89          return;
 90      p = sensors_auto_percent();
 91      if (p == state.pwm)
 92          return;
 93      if (diff(p, state.pwm) >= AUTO_HYST || p == 0 || p == 100)
 94          state_auto_pwm(p);
 95  }
```

- **Linia 84:** Początek funkcji dopasowującej jasność do światła.
- **Linia 85:** Początek treści funkcji.
- **Linia 86:** Zmienna na wyliczoną jasność.
- **Linia 88:** Jeśli tryb to nie AUTO...
- **Linia 89:** ...nic nie rób, zakończ.
- **Linia 90:** Oblicz jasność na podstawie fotorezystora.
- **Linia 91:** Jeśli jest taka sama jak obecna...
- **Linia 92:** ...zakończ.
- **Linia 93:** Jeśli różnica jest wystarczająco duża albo osiągnięto skraj (0 lub 100)...
- **Linia 94:** ...ustaw nową jasność. Margines zapobiega miganiu, gdy światło minimalnie się waha.
- **Linia 95:** Koniec funkcji.

### Alarm temperatury

```c
 97  static void temp_check(void)
 98  {
 99      int16_t t = sensors_temp_tenths();
100      int16_t limit = (int16_t)state.tlim * 10;
101
102      if (!state.alarm && t >= limit) {
103          state.alarm = 1;
104          protocol_event_temp(t);
105      } else if (state.alarm && t < limit - TEMP_HYST_TENTHS) {
106          state.alarm = 0;
107          protocol_event_temp(t);
108      }
109  }
```

- **Linia 97:** Początek funkcji sprawdzającej temperaturę.
- **Linia 98:** Początek treści funkcji.
- **Linia 99:** Odczytaj temperaturę w dziesiątych częściach stopnia, np. 235 to 23,5 °C.
- **Linia 100:** Przelicz próg na tę samą jednostkę: stopnie razy 10. `(int16_t)` wymusza 16 bitów, by 125 × 10 się zmieściło.
- **Linia 102:** Jeśli alarmu jeszcze nie ma, a temperatura osiągnęła próg...
- **Linia 103:** ...włącz alarm...
- **Linia 104:** ...i wyślij zdarzenie `EVT TEMP`.
- **Linia 105:** Jeśli alarm trwa, a temperatura spadła poniżej progu minus histereza...
- **Linia 106:** ...wyłącz alarm...
- **Linia 107:** ...i wyślij zdarzenie `EVT TEMP`.
- **Linia 108:** Koniec warunków.
- **Linia 109:** Koniec funkcji.

### Zadania wykonywane co 1 ms

```c
111  static void tick(uint16_t now)
112  {
113      uint8_t b = button_tick();
114
115      if (b == BTN_SHORT) {
116          state_set_on(!state.on);
117          protocol_event_btn_on();
118      } else if (b == BTN_LONG) {
119          state_set_mode(state.mode == MODE_AUTO ? MODE_MANUAL : MODE_AUTO);
120          protocol_event_btn_mode();
121      }
122      pwm_tick();
123      state_tick();
124      status_tick(now);
125
126      if (!adc_ready())
127          return;
128      if (++pot_timer >= POT_PERIOD_MS) {
129          pot_timer = 0;
130          pot_check();
131      }
132      if (++auto_timer >= AUTO_PERIOD_MS) {
133          auto_timer = 0;
134          auto_check();
135      }
136      if (++temp_timer >= TEMP_PERIOD_MS) {
137          temp_timer = 0;
138          temp_check();
139      }
140  }
```

- **Linia 111:** Początek funkcji `tick`. Pętla główna uruchamia ją raz na każdą milisekundę. `now` to czas tej milisekundy.
- **Linia 112:** Początek treści funkcji.
- **Linia 113:** Sprawdź przycisk i zapisz wynik: brak, krótkie lub długie naciśnięcie.
- **Linia 115:** Przy krótkim naciśnięciu...
- **Linia 116:** ...odwróć stan wyjścia. `!state.on` zamienia 0 na 1, a 1 na 0.
- **Linia 117:** Wyślij zdarzenie `EVT BTN on=...`.
- **Linia 118:** Przy długim naciśnięciu...
- **Linia 119:** ...zmień tryb: jeśli teraz jest AUTO, ustaw MANUAL, a w przeciwnym razie AUTO.
- **Linia 120:** Wyślij zdarzenie `EVT BTN mode=...`.
- **Linia 121:** Koniec warunków.
- **Linia 122:** Wykonaj kolejny krok płynnej zmiany jasności.
- **Linia 123:** Obsłuż opóźniony zapis do EEPROM.
- **Linia 124:** Zaktualizuj diodę statusu.
- **Linia 126:** Jeśli nie ma jeszcze pełnego kompletu pomiarów...
- **Linia 127:** ...zakończ tutaj, by nie decydować na podstawie pustych wartości.
- **Linia 128:** Zwiększ licznik potencjometru. Gdy osiągnie 50 ms...
- **Linia 129:** ...wyzeruj go...
- **Linia 130:** ...i sprawdź potencjometr.
- **Linia 131:** Koniec warunku.
- **Linia 132:** To samo dla trybu AUTO: co 200 ms...
- **Linia 133:** ...wyzeruj licznik...
- **Linia 134:** ...i przelicz jasność.
- **Linia 135:** Koniec warunku.
- **Linia 136:** To samo dla temperatury: co 500 ms...
- **Linia 137:** ...wyzeruj licznik...
- **Linia 138:** ...i sprawdź alarm.
- **Linia 139:** Koniec warunku.
- **Linia 140:** Koniec funkcji.

### Funkcja `main`: tu wszystko się zaczyna

```c
142  int main(void)
143  {
144      uint8_t reset_flags = MCUCSR;
145      uint16_t last;
146
147      MCUCSR = 0;
148      wdt_enable(WDTO_500MS);
149  #ifdef OSCCAL_VALUE
150      OSCCAL = OSCCAL_VALUE;
151  #endif
152
153      STATUS_DDR |= _BV(STATUS_BIT);
154      button_init();
155      pwm_init();
156      adc_init();
157      timer_init();
158      uart_init();
159      state_init();
160      sei();
161
162      protocol_event_boot(reset_reason(reset_flags));
163      last = timer_ms16();
```

- **Linia 142:** Funkcja główna. Układ uruchamia ją po każdym resecie. `int` to wymóg języka, choć wynik nigdy nie zostanie zwrócony.
- **Linia 143:** Początek treści funkcji.
- **Linia 144:** Zapisz przyczynę resetu z rejestru `MCUCSR`, zanim zostanie skasowana.
- **Linia 145:** Zmienna `last`: czas ostatnio obsłużonej milisekundy.
- **Linia 147:** Wyzeruj flagi przyczyny resetu, by przy następnym starcie były aktualne.
- **Linia 148:** Włącz watchdog z czasem 500 ms. Jeśli program przestanie go zerować na tak długo, układ sam się zresetuje.
- **Linia 149:** Następna linia zostanie skompilowana tylko wtedy, gdy w `config.h` włączono korektę oscylatora.
- **Linia 150:** Wpisz korektę do rejestru kalibracji oscylatora.
- **Linia 151:** Koniec warunku z linii 149.
- **Linia 153:** Ustaw pin diody statusu jako wyjście.
- **Linia 154:** Przygotuj przycisk.
- **Linia 155:** Przygotuj PWM diody głównej.
- **Linia 156:** Przygotuj przetwornik ADC.
- **Linia 157:** Uruchom zegar milisekundowy.
- **Linia 158:** Przygotuj UART.
- **Linia 159:** Odczytaj zapisany stan z EEPROM.
- **Linia 160:** Włącz obsługę przerwań w całym układzie. Od tej chwili działa zegar i odbiór znaków.
- **Linia 162:** Wyślij zdarzenie `EVT BOOT` z przyczyną resetu.
- **Linia 163:** Zapamiętaj aktualny czas jako punkt startowy.

### Pętla główna

```c
165      for (;;) {
166          char c;
167          uint16_t now;
168
169          wdt_reset();
170          while (uart_read(&c)) {
171              if (protocol_rx(c))
172                  activity_ms = ACTIVITY_MS;
173          }
174          adc_poll();
175          now = timer_ms16();
176          while (last != now)
177              tick(++last);
178      }
179  }
```

- **Linia 165:** `for (;;)` to pętla bez końca.
- **Linia 166:** Zmienna na odebrany znak.
- **Linia 167:** Zmienna na aktualny czas.
- **Linia 169:** Wyzeruj watchdog: „żyję”. Jeśli pętla się zawiesi, ta linia przestanie się wykonywać i watchdog zresetuje układ.
- **Linia 170:** Dopóki w buforze są odebrane znaki, pobieraj po jednym do `c`. `&c` to adres zmiennej, pod który funkcja wpisze znak.
- **Linia 171:** Przekaż znak do protokołu. Wynik 1 oznacza wykonanie komendy.
- **Linia 172:** Włącz na chwilę diodę statusu jako sygnał aktywności.
- **Linia 173:** Koniec pętli odbioru.
- **Linia 174:** Sprawdź, czy skończył się pomiar ADC. Jeśli tak, zapisz go i rozpocznij kolejny.
- **Linia 175:** Odczytaj aktualny czas.
- **Linia 176:** Dopóki obsłużony czas jest różny od aktualnego...
- **Linia 177:** ...zwiększ `last` i wykonaj zadania tej milisekundy. Jeśli pętla się spóźniła, zaległe milisekundy nadrobią się kolejno, więc żadna nie przepadnie.
- **Linia 178:** Koniec pętli `for`.
- **Linia 179:** Koniec funkcji `main`.

---

[← Poprzedni](11-firmware-naglowki.md) · [Spis treści](README.md) · [Następny →](13-firmware-timer.md)
