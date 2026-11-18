# Firmware: `main.c`, serce programu

[← Poprzedni](11-firmware-naglowki.md) · [Spis treści](README.md) · [Następny →](13-firmware-timer.md)

**Po co ten plik:** program w układzie zaczyna się od funkcji `main`. Tutaj układ jest przygotowywany do pracy, a potem w nieskończonej pętli obsługuje odebrane znaki, pomiary i zadania wykonywane co milisekundę. Tu też zapadają decyzje: co zrobić po naciśnięciu przycisku albo ruchu potencjometru.

Układ nie „kończy” pracy jak zwykły program. Działa, dopóki ma zasilanie. Dlatego główna pętla nigdy się nie kończy.

## `firmware/src/main.c`

[Otwórz ten plik w repozytorium](../../firmware/src/main.c)

### Dołączanie potrzebnych plików

```c
  1  // Sterownik oświetlenia LED: ATmega32A na płytce ATB 1.05, sterowanie przez UART, przycisk i potencjometr
  2  #include <avr/io.h>
  3  #include <avr/interrupt.h>
  4  #include <avr/pgmspace.h>
  5  #include <avr/wdt.h>
  6  #include "config.h"
  7  #include "pinout.h"
  8  #include "adc.h"
  9  #include "button.h"
 10  #include "ds18b20.h"
 11  #include "protocol.h"
 12  #include "pwm.h"
 13  #include "sensors.h"
 14  #include "state.h"
 15  #include "timer.h"
 16  #include "uart.h"
```

- **Linia 1:** Komentarz: to sterownik oświetlenia LED na ATmega32A na płytce ATB 1.05. Sterować można przez UART, przycisk i potencjometr. Zegar (1 MHz albo 11,0592 MHz) ustala Makefile.
- **Linia 2:** Biblioteka z nazwami rejestrów układu.
- **Linia 3:** Biblioteka obsługi przerwań, m.in. `sei()`.
- **Linia 4:** Biblioteka dostępu do pamięci flash, m.in. `PSTR`.
- **Linia 5:** Biblioteka obsługi watchdoga.
- **Linia 6:** Nasze ustawienia.
- **Linia 7:** Nasze przypisanie pinów.
- **Linia 8:** Nasz moduł pomiarów ADC.
- **Linia 9:** Nasz moduł przycisku.
- **Linia 10:** Nasz moduł czujnika temperatury DS18B20.
- **Linia 11:** Nasz moduł protokołu tekstowego.
- **Linia 12:** Nasz moduł jasności diody.
- **Linia 13:** Nasz moduł przeliczania pomiarów.
- **Linia 14:** Nasz moduł stanu urządzenia.
- **Linia 15:** Nasz moduł zegara.
- **Linia 16:** Nasz moduł komunikacji UART.

### Stałe i zmienne

```c
 18  #define POT_PERIOD_MS   50
 19  #define AUTO_PERIOD_MS  200
 20  #define TEMP_PERIOD_MS  500
 21  #define ACTIVITY_MS     50
 22
 23  static uint8_t pot_timer;
 24  static uint8_t auto_timer;
 25  static uint16_t temp_timer;
 26  static uint8_t activity_ms;
 27  #if POT_ENABLE
 28  static uint8_t pot_last = 0xFF;
 29  #endif
```

- **Linia 18:** Potencjometr sprawdzamy co 50 ms.
- **Linia 19:** Jasność w trybie AUTO przeliczamy co 200 ms.
- **Linia 20:** Temperaturę sprawdzamy co 500 ms.
- **Linia 21:** Po odebraniu komendy dioda statusu świeci przez 50 ms.
- **Linia 23:** Licznik milisekund do sprawdzenia potencjometru. `static` oznacza, że widać go tylko w tym pliku, a zaczyna od 0.
- **Linia 24:** Licznik milisekund do przeliczenia trybu AUTO.
- **Linia 25:** Licznik milisekund do sprawdzenia temperatury. Musi być 16-bitowy, bo liczy do 500, a 8 bitów mieści najwyżej 255.
- **Linia 26:** Ile milisekund jeszcze ma świecić dioda statusu po komendzie.
- **Linia 27:** Dalsza linia jest kompilowana tylko wtedy, gdy potencjometr jest włączony w `config.h`.
- **Linia 28:** Ostatnie uwzględnione położenie potencjometru. `0xFF` (255) to wartość specjalna: „jeszcze nie było odczytu”. Prawdziwy odczyt ma najwyżej 100.
- **Linia 29:** Koniec warunku z linii 26.

### Przyczyna resetu

Rejestr `MCUCSR` zawiera flagi mówiące, dlaczego układ wystartował. Ta funkcja zamienia je na napis.

```c
 31  static PGM_P reset_reason(uint8_t flags)
 32  {
 33      if (flags & _BV(WDRF))
 34          return PSTR("WDT");
 35      if (flags & _BV(BORF))
 36          return PSTR("BOR");
 37      if (flags & _BV(EXTRF))
 38          return PSTR("EXT");
 39      return PSTR("POR");
 40  }
```

- **Linia 31:** Początek funkcji. Dostaje bajt z flagami i zwraca wskaźnik na napis w pamięci flash.
- **Linia 32:** Początek treści funkcji.
- **Linia 33:** Jeśli ustawiony jest bit `WDRF`, układ zresetował watchdog.
- **Linia 34:** Zwróć napis `WDT`. `PSTR` trzyma go we flash, a nie w RAM. `return` od razu kończy funkcję.
- **Linia 35:** Jeśli ustawiony jest bit `BORF`, reset spowodował spadek napięcia.
- **Linia 36:** Zwróć napis `BOR`.
- **Linia 37:** Jeśli ustawiony jest bit `EXTRF`, reset przyszedł przez nóżkę RESET.
- **Linia 38:** Zwróć napis `EXT`.
- **Linia 39:** Żaden z powyższych warunków nie zaszedł, więc zostaje włączenie zasilania: `POR`.
- **Linia 40:** Koniec funkcji.

### Pomocnicza różnica

```c
 42  static uint8_t diff(uint8_t a, uint8_t b)
 43  {
 44      return a > b ? a - b : b - a;
 45  }
```

- **Linia 42:** Funkcja `diff` dostaje dwie liczby i zwraca ich różnicę, zawsze nieujemną.
- **Linia 43:** Początek funkcji.
- **Linia 44:** Jeśli `a` jest większe od `b`, wynikiem jest `a - b`, a inaczej `b - a`. Liczby bez znaku nie mogą być ujemne, dlatego odejmujemy zawsze mniejszą od większej.
- **Linia 45:** Koniec funkcji.

### Dioda statusu

```c
 47  // Status: krótki błysk co ok. 1 s, miganie 4 Hz przy alarmie, błysk przy komendzie
 48  static void status_tick(uint16_t now)
 49  {
 50      uint8_t lit;
 51
 52      if (state.alarm)
 53          lit = (now & 0x80) != 0;
 54      else
 55          lit = (now & 0x3FF) < 30;
 56      if (activity_ms > 0) {
 57          activity_ms--;
 58          lit = 1;
 59      }
 60  #if LED_ACTIVE_LOW
 61      lit = !lit;
 62  #endif
 63      if (lit)
 64          STATUS_PORT |= _BV(STATUS_BIT);
 65      else
 66          STATUS_PORT &= ~_BV(STATUS_BIT);
 67  }
```

- **Linia 47:** Komentarz: normalnie dioda błyska krótko co około 1 s. Przy alarmie miga około 4 razy na sekundę, a przy komendzie krótko się zapala.
- **Linia 48:** Początek funkcji. Dostaje aktualny czas w milisekundach.
- **Linia 49:** Początek treści funkcji.
- **Linia 50:** Zmienna `lit`: czy dioda ma w tej chwili świecić.
- **Linia 52:** Jeśli trwa alarm temperatury...
- **Linia 53:** ...dioda świeci, gdy bit 7 czasu jest ustawiony (`0x80` to 128). Ten bit zmienia się co 128 ms, więc pełny cykl trwa 256 ms, czyli daje około 4 mignięć na sekundę.
- **Linia 54:** W przeciwnym razie, czyli bez alarmu:
- **Linia 55:** `now & 0x3FF` to reszta z dzielenia czasu przez 1024. Dioda świeci przez pierwsze 30 ms z każdych 1024 ms, czyli daje krótki błysk co około sekundę.
- **Linia 56:** Jeśli po odebranej komendzie zostało jeszcze trochę czasu świecenia...
- **Linia 57:** ...zmniejsz ten czas o 1 ms...
- **Linia 58:** ...i zapal diodę niezależnie od wcześniejszego wyniku.
- **Linia 59:** Koniec warunku z linii 55.
- **Linia 60:** Warunek sprawdzany przez kompilator: jeśli dioda świeci stanem niskim (płytka ATB)...
- **Linia 61:** ...odwróć wartość: „ma świecić” zamienia się w „ustaw 0 V”, a „ma nie świecić” w „ustaw 5 V”.
- **Linia 62:** Koniec warunku.
- **Linia 63:** Jeśli pin ma mieć stan wysoki...
- **Linia 64:** ...ustaw bit diody w porcie, czyli podaj 5 V na nóżkę.
- **Linia 65:** W przeciwnym razie:
- **Linia 66:** Skasuj jej bit, czyli podaj 0 V.
- **Linia 67:** Koniec funkcji.

### Sprawdzanie potencjometru

```c
 69  static void pot_check(void)
 70  {
 71  #if POT_ENABLE
 72      uint8_t p = sensors_pot_percent();
 73
 74      // Pierwszy odczyt to punkt odniesienia – po restarcie liczy się stan z EEPROM
 75      if (pot_last == 0xFF) {
 76          pot_last = p;
 77          return;
 78      }
 79      if (p == pot_last)
 80          return;
 81      if (diff(p, pot_last) < POT_HYST && p != 0 && p != 100)
 82          return;
 83      pot_last = p;
 84      protocol_event_pot(state_set_pwm(p));
 85  #endif
 86  }
```

- **Linia 69:** Początek funkcji sprawdzającej, czy ktoś obrócił pokrętło.
- **Linia 70:** Początek treści funkcji.
- **Linia 71:** Cała treść jest kompilowana tylko wtedy, gdy potencjometr jest włączony w `config.h`.
- **Linia 72:** Odczytaj położenie potencjometru w procentach i zapisz w `p`.
- **Linia 74:** Komentarz: po restarcie pierwszy odczyt nie może zmienić jasności, bo obowiązuje stan zapisany w EEPROM.
- **Linia 75:** Jeśli to pierwszy odczyt (wartość specjalna z linii 27)...
- **Linia 76:** ...zapamiętaj go jako punkt odniesienia...
- **Linia 77:** ...i zakończ funkcję.
- **Linia 78:** Koniec warunku.
- **Linia 79:** Jeśli położenie się nie zmieniło...
- **Linia 80:** ...nie ma nic do zrobienia, zakończ.
- **Linia 81:** Jeśli zmiana jest mniejsza niż margines z `config.h`, a pokrętło nie stoi na skraju (0 lub 100)...
- **Linia 82:** ...uznaj to za szum i zakończ. Warunek skrajów pozwala zawsze dojść do pełnych 0 % i 100 %.
- **Linia 83:** Zapamiętaj nowe położenie.
- **Linia 84:** Dwa kroki: `state_set_pwm(p)` ustawia jasność i zwraca, czy zmienił się tryb, a wynik trafia do funkcji wysyłającej zdarzenie `EVT POT`.
- **Linia 85:** Koniec warunku z linii 67.
- **Linia 86:** Koniec funkcji.

### Tryb AUTO

```c
 88  static void auto_check(void)
 89  {
 90      uint8_t p;
 91
 92      if (state.mode != MODE_AUTO)
 93          return;
 94      p = sensors_auto_percent();
 95      if (p == state.pwm)
 96          return;
 97      if (diff(p, state.pwm) >= AUTO_HYST || p == 0 || p == 100)
 98          state_auto_pwm(p);
 99  }
```

- **Linia 88:** Początek funkcji dopasowującej jasność do światła.
- **Linia 89:** Początek treści funkcji.
- **Linia 90:** Zmienna na wyliczoną jasność.
- **Linia 92:** Jeśli tryb to nie AUTO...
- **Linia 93:** ...nic nie rób, zakończ.
- **Linia 94:** Oblicz jasność na podstawie fotorezystora.
- **Linia 95:** Jeśli jest taka sama jak obecna...
- **Linia 96:** ...zakończ.
- **Linia 97:** Jeśli różnica jest wystarczająco duża albo osiągnięto skraj (0 lub 100)...
- **Linia 98:** ...ustaw nową jasność. Margines zapobiega miganiu, gdy światło minimalnie się waha.
- **Linia 99:** Koniec funkcji.

### Alarm temperatury

```c
101  static void temp_check(void)
102  {
103      int16_t t = ds18b20_tenths();
104      int16_t limit = (int16_t)state.tlim * 10;
105
106      // Bez odczytu alarm zostaje w dotychczasowym stanie
107      if (t == TEMP_NONE)
108          return;
109      if (!state.alarm && t >= limit) {
110          state.alarm = 1;
111          protocol_event_temp(t);
112      } else if (state.alarm && t < limit - TEMP_HYST_TENTHS) {
113          state.alarm = 0;
114          protocol_event_temp(t);
115      }
116  }
```

- **Linia 101:** Początek funkcji sprawdzającej temperaturę.
- **Linia 102:** Początek treści funkcji.
- **Linia 103:** Odczytaj ostatni pomiar z czujnika DS18B20 w dziesiątych częściach stopnia, np. 235 to 23,5 °C.
- **Linia 104:** Przelicz próg na tę samą jednostkę: stopnie razy 10. `(int16_t)` wymusza 16 bitów, by 125 × 10 się zmieściło.
- **Linia 106:** Komentarz: gdy nie ma odczytu, alarm zostaje taki, jaki był.
- **Linia 107:** Jeśli czujnik nie dał ważnego odczytu (nie odpowiada albo zła suma kontrolna)...
- **Linia 108:** ...zakończ funkcję bez zmian.
- **Linia 109:** Jeśli alarmu jeszcze nie ma, a temperatura osiągnęła próg...
- **Linia 110:** ...włącz alarm...
- **Linia 111:** ...i wyślij zdarzenie `EVT TEMP`.
- **Linia 112:** Jeśli alarm trwa, a temperatura spadła poniżej progu minus histereza...
- **Linia 113:** ...wyłącz alarm...
- **Linia 114:** ...i wyślij zdarzenie `EVT TEMP`.
- **Linia 115:** Koniec warunków.
- **Linia 116:** Koniec funkcji.

### Zadania wykonywane co 1 ms

```c
118  static void tick(uint16_t now)
119  {
120      uint8_t b = button_tick();
121
122      if (b == BTN_SHORT) {
123          state_set_on(!state.on);
124          protocol_event_btn_on();
125      } else if (b == BTN_LONG) {
126          state_set_mode(state.mode == MODE_AUTO ? MODE_MANUAL : MODE_AUTO);
127          protocol_event_btn_mode();
128      }
129      pwm_tick();
130      state_tick();
131      status_tick(now);
132      ds18b20_tick();
133
134      if (!adc_ready())
135          return;
136      if (++pot_timer >= POT_PERIOD_MS) {
137          pot_timer = 0;
138          pot_check();
139      }
140      if (++auto_timer >= AUTO_PERIOD_MS) {
141          auto_timer = 0;
142          auto_check();
143      }
144      if (++temp_timer >= TEMP_PERIOD_MS) {
145          temp_timer = 0;
146          temp_check();
147      }
148  }
```

- **Linia 118:** Początek funkcji `tick`. Pętla główna uruchamia ją raz na każdą milisekundę. `now` to czas tej milisekundy.
- **Linia 119:** Początek treści funkcji.
- **Linia 120:** Sprawdź przycisk i zapisz wynik: brak, krótkie lub długie naciśnięcie.
- **Linia 122:** Przy krótkim naciśnięciu...
- **Linia 123:** ...odwróć stan wyjścia. `!state.on` zamienia 0 na 1, a 1 na 0.
- **Linia 124:** Wyślij zdarzenie `EVT BTN on=...`.
- **Linia 125:** Przy długim naciśnięciu...
- **Linia 126:** ...zmień tryb: jeśli teraz jest AUTO, ustaw MANUAL, a w przeciwnym razie AUTO.
- **Linia 127:** Wyślij zdarzenie `EVT BTN mode=...`.
- **Linia 128:** Koniec warunków.
- **Linia 129:** Wykonaj kolejny krok płynnej zmiany jasności.
- **Linia 130:** Obsłuż opóźniony zapis do EEPROM.
- **Linia 131:** Zaktualizuj diodę statusu.
- **Linia 132:** Wykonaj kolejny krok obsługi czujnika DS18B20 (co najwyżej jeden reset linii albo jeden bajt na milisekundę).
- **Linia 134:** Jeśli nie ma jeszcze pełnego kompletu pomiarów...
- **Linia 135:** ...zakończ tutaj, by nie decydować na podstawie pustych wartości.
- **Linia 136:** Zwiększ licznik potencjometru. Gdy osiągnie 50 ms...
- **Linia 137:** ...wyzeruj go...
- **Linia 138:** ...i sprawdź potencjometr.
- **Linia 139:** Koniec warunku.
- **Linia 140:** To samo dla trybu AUTO: co 200 ms...
- **Linia 141:** ...wyzeruj licznik...
- **Linia 142:** ...i przelicz jasność.
- **Linia 143:** Koniec warunku.
- **Linia 144:** To samo dla temperatury: co 500 ms...
- **Linia 145:** ...wyzeruj licznik...
- **Linia 146:** ...i sprawdź alarm.
- **Linia 147:** Koniec warunku.
- **Linia 148:** Koniec funkcji.

### Funkcja `main`: tu wszystko się zaczyna

```c
150  int main(void)
151  {
152      uint8_t reset_flags = MCUCSR;
153      uint16_t last;
154
155      MCUCSR = 0;
156      wdt_enable(WDTO_500MS);
157  #ifdef OSCCAL_VALUE
158      OSCCAL = OSCCAL_VALUE;
159  #endif
160
161      STATUS_DDR |= _BV(STATUS_BIT);
162      button_init();
163      pwm_init();
164      adc_init();
165      timer_init();
166      uart_init();
167      state_init();
168      sei();
169
170      protocol_event_boot(reset_reason(reset_flags));
171      last = timer_ms16();
```

- **Linia 150:** Funkcja główna. Układ uruchamia ją po każdym resecie. `int` to wymóg języka, choć wynik nigdy nie zostanie zwrócony.
- **Linia 151:** Początek treści funkcji.
- **Linia 152:** Zapisz przyczynę resetu z rejestru `MCUCSR`, zanim zostanie skasowana.
- **Linia 153:** Zmienna `last`: czas ostatnio obsłużonej milisekundy.
- **Linia 155:** Wyzeruj flagi przyczyny resetu, by przy następnym starcie były aktualne.
- **Linia 156:** Włącz watchdog z czasem 500 ms. Jeśli program przestanie go zerować na tak długo, układ sam się zresetuje.
- **Linia 157:** Następna linia zostanie skompilowana tylko wtedy, gdy w `config.h` włączono korektę oscylatora.
- **Linia 158:** Wpisz korektę do rejestru kalibracji oscylatora.
- **Linia 159:** Koniec warunku z linii 149.
- **Linia 161:** Ustaw pin diody statusu jako wyjście.
- **Linia 162:** Przygotuj przycisk.
- **Linia 163:** Przygotuj PWM diody głównej.
- **Linia 164:** Przygotuj przetwornik ADC.
- **Linia 165:** Uruchom zegar milisekundowy.
- **Linia 166:** Przygotuj UART.
- **Linia 167:** Odczytaj zapisany stan z EEPROM.
- **Linia 168:** Włącz obsługę przerwań w całym układzie. Od tej chwili działa zegar i odbiór znaków.
- **Linia 170:** Wyślij zdarzenie `EVT BOOT` z przyczyną resetu.
- **Linia 171:** Zapamiętaj aktualny czas jako punkt startowy.

### Pętla główna

```c
173      for (;;) {
174          char c;
175          uint16_t now;
176
177          wdt_reset();
178          while (uart_read(&c)) {
179              if (protocol_rx(c))
180                  activity_ms = ACTIVITY_MS;
181          }
182          adc_poll();
183          now = timer_ms16();
184          while (last != now)
185              tick(++last);
186      }
187  }
```

- **Linia 173:** `for (;;)` to pętla bez końca.
- **Linia 174:** Zmienna na odebrany znak.
- **Linia 175:** Zmienna na aktualny czas.
- **Linia 177:** Wyzeruj watchdog: „żyję”. Jeśli pętla się zawiesi, ta linia przestanie się wykonywać i watchdog zresetuje układ.
- **Linia 178:** Dopóki w buforze są odebrane znaki, pobieraj po jednym do `c`. `&c` to adres zmiennej, pod który funkcja wpisze znak.
- **Linia 179:** Przekaż znak do protokołu. Wynik 1 oznacza wykonanie komendy.
- **Linia 180:** Włącz na chwilę diodę statusu jako sygnał aktywności.
- **Linia 181:** Koniec pętli odbioru.
- **Linia 182:** Sprawdź, czy skończył się pomiar ADC. Jeśli tak, zapisz go i rozpocznij kolejny.
- **Linia 183:** Odczytaj aktualny czas.
- **Linia 184:** Dopóki obsłużony czas jest różny od aktualnego...
- **Linia 185:** ...zwiększ `last` i wykonaj zadania tej milisekundy. Jeśli pętla się spóźniła, zaległe milisekundy nadrobią się kolejno, więc żadna nie przepadnie.
- **Linia 186:** Koniec pętli `for`.
- **Linia 187:** Koniec funkcji `main`.

---

[← Poprzedni](11-firmware-naglowki.md) · [Spis treści](README.md) · [Następny →](13-firmware-timer.md)
