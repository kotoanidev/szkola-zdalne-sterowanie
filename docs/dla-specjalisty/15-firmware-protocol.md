# Firmware: `protocol.c`, rozumienie komend

[← Poprzedni](14-firmware-uart.md) · [Spis treści](README.md) · [Następny →](16-firmware-pwm.md)

**Po co ten plik:** przez przewód przychodzą pojedyncze znaki. Ten moduł składa je w linię, rozpoznaje komendę (np. `PWM 60`), wykonuje ją i odsyła odpowiedź (np. `OK pwm=60`). Tu są też wiadomości wysyłane samodzielnie, czyli zdarzenia `EVT`. Pełna umowa jest w `docs/protokol.md`.

## `firmware/src/protocol.c`

[Otwórz ten plik w repozytorium](../../firmware/src/protocol.c)

### Początek pliku

```c
  1  // Protokół tekstowy UART – specyfikacja w docs/protokol.md
  2  #include <string.h>
  3  #include <avr/pgmspace.h>
  4  #include "config.h"
  5  #include "pinout.h"
  6  #include "adc.h"
  7  #include "sensors.h"
  8  #include "state.h"
  9  #include "timer.h"
 10  #include "uart.h"
 11  #include "protocol.h"
 12
 13  #define LINE_MAX 32
 14
 15  static char line[LINE_MAX + 1];
 16  static uint8_t len;
 17  static uint8_t too_long;
```

- **Linia 1:** Komentarz: protokół tekstowy UART, specyfikacja w `docs/protokol.md`.
- **Linia 2:** Biblioteka do pracy na napisach, m.in. `strchr` i `strlen`.
- **Linia 3:** Biblioteka dostępu do pamięci flash, m.in. `PSTR` i `strcmp_P`.
- **Linia 4:** Nasze ustawienia, m.in. `FW_VERSION`.
- **Linia 5:** Nasze przypisanie pinów, m.in. numery kanałów ADC.
- **Linia 6:** Nasz moduł pomiarów.
- **Linia 7:** Nasz moduł przeliczania pomiarów.
- **Linia 8:** Nasz moduł stanu urządzenia.
- **Linia 9:** Nasz moduł zegara.
- **Linia 10:** Nasz moduł komunikacji UART.
- **Linia 11:** Nasz własny nagłówek.
- **Linia 13:** Najdłuższa dopuszczalna komenda: 32 znaki.
- **Linia 15:** Bufor na zbieraną linię. O jeden bajt większy niż limit, by zmieścić znak końca napisu `'\0'`.
- **Linia 16:** Ile znaków jest już w buforze.
- **Linia 17:** Znacznik: czy linia przekroczyła limit długości.

### Małe klocki do budowania odpowiedzi

```c
 19  static void kv(PGM_P key, uint32_t value)
 20  {
 21      uart_puts_P(key);
 22      uart_put_uint(value);
 23  }
```

- **Linia 19:** Funkcja `kv` (*key–value*, klucz–wartość) wysyła parę, np. ` pwm=60`. Klucz jest napisem we flash.
- **Linia 20:** Początek funkcji.
- **Linia 21:** Wyślij klucz.
- **Linia 22:** Wyślij wartość jako tekst.
- **Linia 23:** Koniec funkcji.

```c
 25  static void kv_mode(void)
 26  {
 27      uart_puts_P(PSTR(" mode="));
 28      uart_puts_P(state.mode == MODE_AUTO ? PSTR("AUTO") : PSTR("MANUAL"));
 29  }
```

- **Linia 25:** Funkcja wysyłająca tryb pracy.
- **Linia 26:** Początek funkcji.
- **Linia 27:** Wyślij napis ` mode=`.
- **Linia 28:** Wyślij `AUTO` albo `MANUAL`, zależnie od aktualnego trybu.
- **Linia 29:** Koniec funkcji.

```c
 31  static void kv_temp(int16_t temp)
 32  {
 33      uart_puts_P(PSTR(" temp="));
 34      uart_put_tenths(temp);
 35  }
```

- **Linia 31:** Funkcja wysyłająca temperaturę.
- **Linia 32:** Początek funkcji.
- **Linia 33:** Wyślij napis ` temp=`.
- **Linia 34:** Wyślij liczbę, np. 235 jako `23.5`.
- **Linia 35:** Koniec funkcji.

```c
 37  static void reply_P(PGM_P s)
 38  {
 39      uart_puts_P(s);
 40      uart_endl();
 41  }
```

- **Linia 37:** Funkcja wysyłająca gotową odpowiedź z flash wraz z końcem linii.
- **Linia 38:** Początek funkcji.
- **Linia 39:** Wyślij napis.
- **Linia 40:** Wyślij koniec linii.
- **Linia 41:** Koniec funkcji.

```c
 43  static void reply_on(void)
 44  {
 45      uart_puts_P(PSTR("OK"));
 46      kv(PSTR(" on="), state.on);
 47      uart_endl();
 48  }
```

- **Linia 43:** Funkcja wysyłająca odpowiedź po zmianie stanu wyjścia, np. `OK on=1`.
- **Linia 44:** Początek funkcji.
- **Linia 45:** Wyślij `OK`.
- **Linia 46:** Wyślij ` on=` oraz aktualną wartość.
- **Linia 47:** Wyślij koniec linii.
- **Linia 48:** Koniec funkcji.

### Odpowiedź na komendę `GET`

```c
 50  static void reply_state(void)
 51  {
 52      uart_puts_P(PSTR("STATE"));
 53      kv(PSTR(" on="), state.on);
 54      kv(PSTR(" pwm="), state.pwm);
 55      kv_mode();
 56      kv(PSTR(" pot="), adc_value(ADC_CH_POT));
 57      kv(PSTR(" ldr="), adc_value(ADC_CH_LDR));
 58      kv_temp(sensors_temp_tenths());
 59      kv(PSTR(" alarm="), state.alarm);
 60      kv(PSTR(" tlim="), state.tlim);
 61      kv(PSTR(" up="), timer_ms() / 1000);
 62      uart_endl();
 63  }
```

- **Linia 50:** Funkcja wysyłająca pełny stan urządzenia jedną linią.
- **Linia 51:** Początek funkcji.
- **Linia 52:** Wyślij słowo `STATE`.
- **Linia 53:** Wyślij, czy wyjście jest włączone.
- **Linia 54:** Wyślij jasność.
- **Linia 55:** Wyślij tryb.
- **Linia 56:** Wyślij odczyt potencjometru (0–1023).
- **Linia 57:** Wyślij odczyt fotorezystora (0–1023).
- **Linia 58:** Wyślij temperaturę.
- **Linia 59:** Wyślij, czy trwa alarm.
- **Linia 60:** Wyślij próg alarmu.
- **Linia 61:** Wyślij czas pracy w sekundach: milisekundy podzielone przez 1000.
- **Linia 62:** Wyślij koniec linii.
- **Linia 63:** Koniec funkcji.

### Zamiana tekstu na liczbę

Komenda `PWM 60` przychodzi jako tekst. Tekst `"60"` to znaki `6` i `0`, a nie liczba 60. Ta funkcja dokonuje zamiany i odrzuca błędne dane.

```c
 65  static uint8_t parse_uint(const char *s, uint8_t max, uint8_t *out)
 66  {
 67      uint16_t v = 0;
 68
 69      if (*s == '\0')
 70          return 0;
 71      for (; *s != '\0'; s++) {
 72          if (*s < '0' || *s > '9')
 73              return 0;
 74          v = v * 10 + (uint16_t)(*s - '0');
 75          if (v > max)
 76              return 0;
 77      }
 78      *out = (uint8_t)v;
 79      return 1;
 80  }
```

- **Linia 65:** Funkcja dostaje tekst `s`, największą dozwoloną wartość `max` i adres `out`, pod który wpisze wynik. Zwraca 1, gdy się udało, a 0, gdy nie.
- **Linia 66:** Początek funkcji.
- **Linia 67:** Budowana liczba. 16 bitów, żeby wykryć przekroczenie limitu przed obcięciem do 8 bitów.
- **Linia 69:** Jeśli tekst jest pusty, czyli pierwszy znak to już koniec napisu...
- **Linia 70:** ...to błąd, zwróć 0.
- **Linia 71:** Pętla po znakach. Trwa do końca napisu, a `s++` przesuwa wskaźnik na kolejny znak.
- **Linia 72:** Jeśli znak nie jest cyfrą...
- **Linia 73:** ...to błąd, zwróć 0.
- **Linia 74:** Dopisz cyfrę do liczby: pomnóż dotychczasową wartość przez 10 i dodaj nową cyfrę. Dla `"60"`: 0·10+6 = 6, potem 6·10+0 = 60.
- **Linia 75:** Jeśli liczba przekroczyła dozwoloną wartość...
- **Linia 76:** ...to błąd, zwróć 0. Sprawdzamy to na bieżąco, by bardzo długa liczba nie przepełniła zmiennej.
- **Linia 77:** Koniec pętli.
- **Linia 78:** Wpisz wynik pod podany adres.
- **Linia 79:** Zwróć 1: sukces.
- **Linia 80:** Koniec funkcji.

### Wykonanie komendy

Funkcja dostaje linię, np. `PWM 60`. Najpierw rozdziela ją na samą komendę (`PWM`) i argument (`60`), a potem szuka pasującej komendy.

```c
 82  static void execute(char *cmd)
 83  {
 84      char *arg = strchr(cmd, ' ');
 85      uint8_t value;
 86
 87      if (arg != NULL) {
 88          *arg++ = '\0';
 89          while (*arg == ' ')
 90              arg++;
 91      } else {
 92          arg = cmd + strlen(cmd);
 93      }
```

- **Linia 82:** Początek funkcji `execute`. `cmd` wskazuje na tekst komendy.
- **Linia 83:** Początek treści funkcji.
- **Linia 84:** `strchr` szuka pierwszej spacji i zwraca wskaźnik na nią. Gdy spacji nie ma, zwraca `NULL`.
- **Linia 85:** Zmienna na liczbę z argumentu.
- **Linia 87:** Jeśli spacja jest...
- **Linia 88:** ...wstaw w jej miejsce koniec napisu. W ten sposób `cmd` kończy się na `PWM`. Następnie `arg++` przesuwa wskaźnik na początek argumentu.
- **Linia 89:** Dopóki w argumencie na początku jest spacja...
- **Linia 90:** ...przesuń wskaźnik, czyli pomiń nadmiarową spację.
- **Linia 91:** W przeciwnym razie, czyli bez spacji:
- **Linia 92:** Niech `arg` wskazuje na koniec napisu `cmd`, czyli na pusty tekst. Dzięki temu dalej można go sprawdzać bezpiecznie.
- **Linia 93:** Koniec warunku.

```c
 95      if (strcmp_P(cmd, PSTR("PWM")) == 0) {
 96          if (!parse_uint(arg, 100, &value)) {
 97              reply_P(PSTR("ERR ARG"));
 98              return;
 99          }
100          uint8_t mode_changed = state_set_pwm(value);
101
102          uart_puts_P(PSTR("OK"));
103          kv(PSTR(" pwm="), value);
104          if (mode_changed)
105              kv_mode();
106          uart_endl();
```

- **Linia 95:** Jeśli komenda to `PWM`... (`strcmp_P` zwraca 0, gdy napisy są takie same.)
- **Linia 96:** Spróbuj zamienić argument na liczbę z zakresu 0–100. `&value` to adres wyniku. Jeśli się nie uda...
- **Linia 97:** ...odeślij `ERR ARG`, czyli „zły argument”...
- **Linia 98:** ...i zakończ.
- **Linia 99:** Koniec warunku.
- **Linia 100:** Ustaw jasność. Funkcja zwraca, czy przy okazji tryb zmienił się z AUTO na MANUAL.
- **Linia 102:** Wyślij `OK`.
- **Linia 103:** Wyślij ` pwm=` z wartością.
- **Linia 104:** Jeśli tryb się zmienił...
- **Linia 105:** ...dopisz informację o trybie.
- **Linia 106:** Wyślij koniec linii.

```c
107      } else if (strcmp_P(cmd, PSTR("MODE")) == 0) {
108          if (strcmp_P(arg, PSTR("AUTO")) == 0) {
109              state_set_mode(MODE_AUTO);
110          } else if (strcmp_P(arg, PSTR("MANUAL")) == 0) {
111              state_set_mode(MODE_MANUAL);
112          } else {
113              reply_P(PSTR("ERR ARG"));
114              return;
115          }
116          uart_puts_P(PSTR("OK"));
117          kv_mode();
118          uart_endl();
```

- **Linia 107:** W przeciwnym razie, jeśli komenda to `MODE`...
- **Linia 108:** Jeśli argument to `AUTO`...
- **Linia 109:** ...ustaw tryb AUTO.
- **Linia 110:** Jeśli argument to `MANUAL`...
- **Linia 111:** ...ustaw tryb ręczny.
- **Linia 112:** W przeciwnym razie:
- **Linia 113:** Odeślij `ERR ARG`.
- **Linia 114:** Zakończ.
- **Linia 115:** Koniec warunku.
- **Linia 116:** Wyślij `OK`.
- **Linia 117:** Wyślij aktualny tryb.
- **Linia 118:** Wyślij koniec linii.

```c
119      } else if (strcmp_P(cmd, PSTR("TLIM")) == 0) {
120          if (!parse_uint(arg, TEMP_LIMIT_MAX, &value)) {
121              reply_P(PSTR("ERR ARG"));
122              return;
123          }
124          state_set_tlim(value);
125          uart_puts_P(PSTR("OK"));
126          kv(PSTR(" tlim="), value);
127          uart_endl();
```

- **Linia 119:** W przeciwnym razie, jeśli komenda to `TLIM`...
- **Linia 120:** Zamień argument na liczbę z zakresu do 125. Jeśli się nie uda...
- **Linia 121:** ...odeślij `ERR ARG`...
- **Linia 122:** ...i zakończ.
- **Linia 123:** Koniec warunku.
- **Linia 124:** Ustaw nowy próg alarmu.
- **Linia 125:** Wyślij `OK`.
- **Linia 126:** Wyślij ` tlim=` z wartością.
- **Linia 127:** Wyślij koniec linii.

```c
128      } else if (*arg != '\0') {
129          // Pozostałe komendy nie przyjmują argumentów
130          if (strcmp_P(cmd, PSTR("PING")) == 0 || strcmp_P(cmd, PSTR("VER")) == 0
131              || strcmp_P(cmd, PSTR("ON")) == 0 || strcmp_P(cmd, PSTR("OFF")) == 0
132              || strcmp_P(cmd, PSTR("TOGGLE")) == 0 || strcmp_P(cmd, PSTR("GET")) == 0)
133              reply_P(PSTR("ERR ARG"));
134          else
135              reply_P(PSTR("ERR UNKNOWN"));
```

- **Linia 128:** W przeciwnym razie, jeśli podano jakikolwiek argument (a pozostałe komendy go nie przyjmują)...
- **Linia 129:** Komentarz wyjaśnia to samo.
- **Linia 130:** Sprawdź, czy to jedna ze znanych komend bez argumentów: `PING` albo `VER`...
- **Linia 131:** ...albo `ON`, albo `OFF`...
- **Linia 132:** ...albo `TOGGLE`, albo `GET`.
- **Linia 133:** Znana komenda z niepotrzebnym argumentem: odeślij `ERR ARG`.
- **Linia 134:** W przeciwnym razie:
- **Linia 135:** Odeślij `ERR UNKNOWN`, czyli „nieznana komenda”.

```c
136      } else if (strcmp_P(cmd, PSTR("PING")) == 0) {
137          reply_P(PSTR("PONG"));
138      } else if (strcmp_P(cmd, PSTR("VER")) == 0) {
139          reply_P(PSTR("VER " FW_VERSION));
140      } else if (strcmp_P(cmd, PSTR("ON")) == 0) {
141          state_set_on(1);
142          reply_on();
143      } else if (strcmp_P(cmd, PSTR("OFF")) == 0) {
144          state_set_on(0);
145          reply_on();
146      } else if (strcmp_P(cmd, PSTR("TOGGLE")) == 0) {
147          state_set_on(!state.on);
148          reply_on();
149      } else if (strcmp_P(cmd, PSTR("GET")) == 0) {
150          reply_state();
151      } else {
152          reply_P(PSTR("ERR UNKNOWN"));
153      }
154  }
```

- **Linia 136:** Komenda `PING`, czyli test łącza...
- **Linia 137:** ...odpowiedź to `PONG`.
- **Linia 138:** Komenda `VER`, czyli pytanie o wersję...
- **Linia 139:** ...odpowiedź to `VER 0.9.0`. Kompilator skleja napis `"VER "` ze stałą `FW_VERSION`.
- **Linia 140:** Komenda `ON`...
- **Linia 141:** ...włącz wyjście...
- **Linia 142:** ...i odeślij `OK on=1`.
- **Linia 143:** Komenda `OFF`...
- **Linia 144:** ...wyłącz wyjście...
- **Linia 145:** ...i odeślij `OK on=0`.
- **Linia 146:** Komenda `TOGGLE`, czyli przełącz...
- **Linia 147:** ...odwróć stan wyjścia...
- **Linia 148:** ...i odeślij nowy stan.
- **Linia 149:** Komenda `GET`...
- **Linia 150:** ...odeślij pełny stan.
- **Linia 151:** Żadna komenda nie pasuje:
- **Linia 152:** Odeślij `ERR UNKNOWN`.
- **Linia 153:** Koniec całego łańcucha warunków.
- **Linia 154:** Koniec funkcji `execute`.

### Odbiór kolejnych znaków

Pętla główna przekazuje tu każdy odebrany znak. Funkcja składa je w linię, a po znaku `\n` wykonuje komendę.

```c
156  uint8_t protocol_rx(char c)
157  {
158      if (c == '\r')
159          return 0;
160
161      if (c == '\n') {
162          uint8_t executed = 0;
163
164          while (len > 0 && line[len - 1] == ' ')
165              len--;
166          line[len] = '\0';
167          if (too_long) {
168              reply_P(PSTR("ERR TOOLONG"));
169              executed = 1;
170          } else if (len > 0) {
171              char *cmd = line;
172              while (*cmd == ' ')
173                  cmd++;
174              execute(cmd);
175              executed = 1;
176          }
177          len = 0;
178          too_long = 0;
179          return executed;
180      }
```

- **Linia 156:** Funkcja dostaje jeden znak. Zwraca 1, jeśli wykonano komendę.
- **Linia 157:** Początek funkcji.
- **Linia 158:** Jeśli to znak powrotu karetki `\r` (terminale dodają go do Entera)...
- **Linia 159:** ...pomiń go, zwróć 0.
- **Linia 161:** Jeśli to znak końca linii, komenda jest kompletna:
- **Linia 162:** Znacznik, czy coś wykonano. Na razie 0.
- **Linia 164:** Dopóki na końcu linii są spacje...
- **Linia 165:** ...skróć linię, czyli usuń końcowe spacje.
- **Linia 166:** Zakończ napis znakiem `'\0'`, by funkcje napisowe wiedziały, gdzie jest koniec.
- **Linia 167:** Jeśli linia była zbyt długa...
- **Linia 168:** ...odeślij `ERR TOOLONG`...
- **Linia 169:** ...i oznacz, że była reakcja.
- **Linia 170:** W przeciwnym razie, jeśli linia nie jest pusta:
- **Linia 171:** Ustaw wskaźnik `cmd` na początek linii.
- **Linia 172:** Dopóki na początku są spacje...
- **Linia 173:** ...przesuń wskaźnik, by je pominąć.
- **Linia 174:** Wykonaj komendę.
- **Linia 175:** Oznacz, że była reakcja.
- **Linia 176:** Koniec warunku.
- **Linia 177:** Wyzeruj długość, by przygotować bufor na następną linię.
- **Linia 178:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 179:** Zwróć wynik.
- **Linia 180:** Koniec obsługi końca linii.

```c
182      // Zgodność z instrukcją: pojedyncze 1 / 0 działa od razu, bez Entera
183      if (len == 0 && !too_long && (c == '1' || c == '0')) {
184          state_set_on(c == '1');
185          reply_on();
186          return 1;
187      }
188
189      if (len >= LINE_MAX) {
190          too_long = 1;
191          return 0;
192      }
193      if (c >= 'a' && c <= 'z')
194          c = (char)(c - 'a' + 'A');
195      line[len++] = c;
196      return 0;
197  }
```

- **Linia 182:** Komentarz: zgodność z instrukcją do etapu 2. Znak `1` lub `0` działa natychmiast, bez Entera.
- **Linia 183:** Jeśli linia jest pusta, nie trwa zbyt długa linia, a znak to `1` albo `0`...
- **Linia 184:** ...włącz lub wyłącz wyjście. Porównanie `c == '1'` daje 1 lub 0.
- **Linia 185:** Odeślij nowy stan.
- **Linia 186:** Zwróć 1: wykonano komendę.
- **Linia 187:** Koniec warunku.
- **Linia 189:** Jeśli w buforze nie ma już miejsca...
- **Linia 190:** ...zapamiętaj, że linia jest za długa...
- **Linia 191:** ...i zwróć 0. Błąd zostanie zgłoszony dopiero po końcu linii.
- **Linia 192:** Koniec warunku.
- **Linia 193:** Jeśli znak to mała litera...
- **Linia 194:** ...zamień ją na wielką: odejmij pozycję `a`, dodaj pozycję `A`. Dzięki temu wielkość liter nie ma znaczenia.
- **Linia 195:** Zapisz znak w buforze i zwiększ długość.
- **Linia 196:** Zwróć 0: linia jeszcze niekompletna.
- **Linia 197:** Koniec funkcji.

### Zdarzenia wysyłane samodzielnie

```c
199  void protocol_event_boot(PGM_P reason)
200  {
201      uart_puts_P(PSTR("EVT BOOT reset="));
202      uart_puts_P(reason);
203      uart_puts_P(PSTR(" ver=" FW_VERSION));
204      uart_endl();
205  }
```

- **Linia 199:** Funkcja wysyłająca informację o starcie układu.
- **Linia 200:** Początek funkcji.
- **Linia 201:** Wyślij początek wiadomości.
- **Linia 202:** Wyślij przyczynę resetu.
- **Linia 203:** Wyślij numer wersji. Kompilator skleja `" ver="` z wersją.
- **Linia 204:** Wyślij koniec linii.
- **Linia 205:** Koniec funkcji.

```c
207  void protocol_event_btn_on(void)
208  {
209      uart_puts_P(PSTR("EVT BTN"));
210      kv(PSTR(" on="), state.on);
211      uart_endl();
212  }
```

- **Linia 207:** Funkcja wysyłająca zdarzenie: przycisk przełączył wyjście.
- **Linia 208:** Początek funkcji.
- **Linia 209:** Wyślij `EVT BTN`.
- **Linia 210:** Wyślij nowy stan wyjścia.
- **Linia 211:** Wyślij koniec linii.
- **Linia 212:** Koniec funkcji.

```c
214  void protocol_event_btn_mode(void)
215  {
216      uart_puts_P(PSTR("EVT BTN"));
217      kv_mode();
218      uart_endl();
219  }
```

- **Linia 214:** Funkcja wysyłająca zdarzenie: przycisk zmienił tryb.
- **Linia 215:** Początek funkcji.
- **Linia 216:** Wyślij `EVT BTN`.
- **Linia 217:** Wyślij nowy tryb.
- **Linia 218:** Wyślij koniec linii.
- **Linia 219:** Koniec funkcji.

```c
221  void protocol_event_pot(uint8_t mode_changed)
222  {
223      uart_puts_P(PSTR("EVT POT"));
224      kv(PSTR(" pwm="), state.pwm);
225      if (mode_changed)
226          kv_mode();
227      uart_endl();
228  }
```

- **Linia 221:** Funkcja wysyłająca zdarzenie: potencjometr zmienił jasność. Parametr mówi, czy zmienił się też tryb.
- **Linia 222:** Początek funkcji.
- **Linia 223:** Wyślij `EVT POT`.
- **Linia 224:** Wyślij nową jasność.
- **Linia 225:** Jeśli tryb się zmienił...
- **Linia 226:** ...dopisz nowy tryb.
- **Linia 227:** Wyślij koniec linii.
- **Linia 228:** Koniec funkcji.

```c
230  void protocol_event_temp(int16_t temp)
231  {
232      uart_puts_P(PSTR("EVT TEMP"));
233      kv(PSTR(" alarm="), state.alarm);
234      kv_temp(temp);
235      uart_endl();
236  }
```

- **Linia 230:** Funkcja wysyłająca zdarzenie: alarm temperatury włączył się lub wyłączył.
- **Linia 231:** Początek funkcji.
- **Linia 232:** Wyślij `EVT TEMP`.
- **Linia 233:** Wyślij stan alarmu.
- **Linia 234:** Wyślij temperaturę.
- **Linia 235:** Wyślij koniec linii.
- **Linia 236:** Koniec funkcji.

---

[← Poprzedni](14-firmware-uart.md) · [Spis treści](README.md) · [Następny →](16-firmware-pwm.md)
