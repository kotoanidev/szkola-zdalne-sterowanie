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
  7  #include "ds18b20.h"
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
- **Linia 7:** Nasz moduł czujnika temperatury DS18B20, m.in. stała `TEMP_NONE` (brak odczytu).
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
 34      if (temp == TEMP_NONE)
 35          uart_puts_P(PSTR("ERR"));
 36      else
 37          uart_put_tenths(temp);
 38  }
```

- **Linia 31:** Funkcja wysyłająca temperaturę.
- **Linia 32:** Początek funkcji.
- **Linia 33:** Wyślij napis ` temp=`.
- **Linia 34:** Jeśli czujnik nie dał ważnego odczytu...
- **Linia 35:** ...wyślij `ERR` zamiast liczby.
- **Linia 36:** W przeciwnym razie...
- **Linia 37:** ...wyślij liczbę, np. 235 jako `23.5`.
- **Linia 38:** Koniec funkcji.

```c
 40  static void reply_P(PGM_P s)
 41  {
 42      uart_puts_P(s);
 43      uart_endl();
 44  }
```

- **Linia 40:** Funkcja wysyłająca gotową odpowiedź z flash wraz z końcem linii.
- **Linia 41:** Początek funkcji.
- **Linia 42:** Wyślij napis.
- **Linia 43:** Wyślij koniec linii.
- **Linia 44:** Koniec funkcji.

```c
 46  static void reply_on(void)
 47  {
 48      uart_puts_P(PSTR("OK"));
 49      kv(PSTR(" on="), state.on);
 50      uart_endl();
 51  }
```

- **Linia 46:** Funkcja wysyłająca odpowiedź po zmianie stanu wyjścia, np. `OK on=1`.
- **Linia 47:** Początek funkcji.
- **Linia 48:** Wyślij `OK`.
- **Linia 49:** Wyślij ` on=` oraz aktualną wartość.
- **Linia 50:** Wyślij koniec linii.
- **Linia 51:** Koniec funkcji.

### Odpowiedź na komendę `GET`

```c
 53  static void reply_state(void)
 54  {
 55      uart_puts_P(PSTR("STATE"));
 56      kv(PSTR(" on="), state.on);
 57      kv(PSTR(" pwm="), state.pwm);
 58      kv_mode();
 59      kv(PSTR(" pot="), adc_value(ADC_CH_POT));
 60      kv(PSTR(" ldr="), adc_value(ADC_CH_LDR));
 61      kv_temp(ds18b20_tenths());
 62      kv(PSTR(" alarm="), state.alarm);
 63      kv(PSTR(" tlim="), state.tlim);
 64      kv(PSTR(" up="), timer_ms() / 1000);
 65      uart_endl();
 66  }
```

- **Linia 53:** Funkcja wysyłająca pełny stan urządzenia jedną linią.
- **Linia 54:** Początek funkcji.
- **Linia 55:** Wyślij słowo `STATE`.
- **Linia 56:** Wyślij, czy wyjście jest włączone.
- **Linia 57:** Wyślij jasność.
- **Linia 58:** Wyślij tryb.
- **Linia 59:** Wyślij odczyt potencjometru (0–1023).
- **Linia 60:** Wyślij odczyt fotorezystora (0–1023).
- **Linia 61:** Wyślij temperaturę z czujnika DS18B20 (albo `ERR`).
- **Linia 62:** Wyślij, czy trwa alarm.
- **Linia 63:** Wyślij próg alarmu.
- **Linia 64:** Wyślij czas pracy w sekundach: milisekundy podzielone przez 1000.
- **Linia 65:** Wyślij koniec linii.
- **Linia 66:** Koniec funkcji.

### Zamiana tekstu na liczbę

Komenda `PWM 60` przychodzi jako tekst. Tekst `"60"` to znaki `6` i `0`, a nie liczba 60. Ta funkcja dokonuje zamiany i odrzuca błędne dane.

```c
 68  static uint8_t parse_uint(const char *s, uint8_t max, uint8_t *out)
 69  {
 70      uint16_t v = 0;
 71
 72      if (*s == '\0')
 73          return 0;
 74      for (; *s != '\0'; s++) {
 75          if (*s < '0' || *s > '9')
 76              return 0;
 77          v = v * 10 + (uint16_t)(*s - '0');
 78          if (v > max)
 79              return 0;
 80      }
 81      *out = (uint8_t)v;
 82      return 1;
 83  }
```

- **Linia 68:** Funkcja dostaje tekst `s`, największą dozwoloną wartość `max` i adres `out`, pod który wpisze wynik. Zwraca 1, gdy się udało, a 0, gdy nie.
- **Linia 69:** Początek funkcji.
- **Linia 70:** Budowana liczba. 16 bitów, żeby wykryć przekroczenie limitu przed obcięciem do 8 bitów.
- **Linia 72:** Jeśli tekst jest pusty, czyli pierwszy znak to już koniec napisu...
- **Linia 73:** ...to błąd, zwróć 0.
- **Linia 74:** Pętla po znakach. Trwa do końca napisu, a `s++` przesuwa wskaźnik na kolejny znak.
- **Linia 75:** Jeśli znak nie jest cyfrą...
- **Linia 76:** ...to błąd, zwróć 0.
- **Linia 77:** Dopisz cyfrę do liczby: pomnóż dotychczasową wartość przez 10 i dodaj nową cyfrę. Dla `"60"`: 0·10+6 = 6, potem 6·10+0 = 60.
- **Linia 78:** Jeśli liczba przekroczyła dozwoloną wartość...
- **Linia 79:** ...to błąd, zwróć 0. Sprawdzamy to na bieżąco, by bardzo długa liczba nie przepełniła zmiennej.
- **Linia 80:** Koniec pętli.
- **Linia 81:** Wpisz wynik pod podany adres.
- **Linia 82:** Zwróć 1: sukces.
- **Linia 83:** Koniec funkcji.

### Wykonanie komendy

Funkcja dostaje linię, np. `PWM 60`. Najpierw rozdziela ją na samą komendę (`PWM`) i argument (`60`), a potem szuka pasującej komendy.

```c
 85  static void execute(char *cmd)
 86  {
 87      char *arg = strchr(cmd, ' ');
 88      uint8_t value;
 89
 90      if (arg != NULL) {
 91          *arg++ = '\0';
 92          while (*arg == ' ')
 93              arg++;
 94      } else {
 95          arg = cmd + strlen(cmd);
 96      }
```

- **Linia 85:** Początek funkcji `execute`. `cmd` wskazuje na tekst komendy.
- **Linia 86:** Początek treści funkcji.
- **Linia 87:** `strchr` szuka pierwszej spacji i zwraca wskaźnik na nią. Gdy spacji nie ma, zwraca `NULL`.
- **Linia 88:** Zmienna na liczbę z argumentu.
- **Linia 90:** Jeśli spacja jest...
- **Linia 91:** ...wstaw w jej miejsce koniec napisu. W ten sposób `cmd` kończy się na `PWM`. Następnie `arg++` przesuwa wskaźnik na początek argumentu.
- **Linia 92:** Dopóki w argumencie na początku jest spacja...
- **Linia 93:** ...przesuń wskaźnik, czyli pomiń nadmiarową spację.
- **Linia 94:** W przeciwnym razie, czyli bez spacji:
- **Linia 95:** Niech `arg` wskazuje na koniec napisu `cmd`, czyli na pusty tekst. Dzięki temu dalej można go sprawdzać bezpiecznie.
- **Linia 96:** Koniec warunku.

```c
 98      if (strcmp_P(cmd, PSTR("PWM")) == 0) {
 99          if (!parse_uint(arg, 100, &value)) {
100              reply_P(PSTR("ERR ARG"));
101              return;
102          }
103          uint8_t mode_changed = state_set_pwm(value);
104
105          uart_puts_P(PSTR("OK"));
106          kv(PSTR(" pwm="), value);
107          if (mode_changed)
108              kv_mode();
109          uart_endl();
```

- **Linia 98:** Jeśli komenda to `PWM`... (`strcmp_P` zwraca 0, gdy napisy są takie same.)
- **Linia 99:** Spróbuj zamienić argument na liczbę z zakresu 0–100. `&value` to adres wyniku. Jeśli się nie uda...
- **Linia 100:** ...odeślij `ERR ARG`, czyli „zły argument”...
- **Linia 101:** ...i zakończ.
- **Linia 102:** Koniec warunku.
- **Linia 103:** Ustaw jasność. Funkcja zwraca, czy przy okazji tryb zmienił się z AUTO na MANUAL.
- **Linia 105:** Wyślij `OK`.
- **Linia 106:** Wyślij ` pwm=` z wartością.
- **Linia 107:** Jeśli tryb się zmienił...
- **Linia 108:** ...dopisz informację o trybie.
- **Linia 109:** Wyślij koniec linii.

```c
110      } else if (strcmp_P(cmd, PSTR("MODE")) == 0) {
111          if (strcmp_P(arg, PSTR("AUTO")) == 0) {
112              state_set_mode(MODE_AUTO);
113          } else if (strcmp_P(arg, PSTR("MANUAL")) == 0) {
114              state_set_mode(MODE_MANUAL);
115          } else {
116              reply_P(PSTR("ERR ARG"));
117              return;
118          }
119          uart_puts_P(PSTR("OK"));
120          kv_mode();
121          uart_endl();
```

- **Linia 110:** W przeciwnym razie, jeśli komenda to `MODE`...
- **Linia 111:** Jeśli argument to `AUTO`...
- **Linia 112:** ...ustaw tryb AUTO.
- **Linia 113:** Jeśli argument to `MANUAL`...
- **Linia 114:** ...ustaw tryb ręczny.
- **Linia 115:** W przeciwnym razie:
- **Linia 116:** Odeślij `ERR ARG`.
- **Linia 117:** Zakończ.
- **Linia 118:** Koniec warunku.
- **Linia 119:** Wyślij `OK`.
- **Linia 120:** Wyślij aktualny tryb.
- **Linia 121:** Wyślij koniec linii.

```c
122      } else if (strcmp_P(cmd, PSTR("TLIM")) == 0) {
123          if (!parse_uint(arg, TEMP_LIMIT_MAX, &value)) {
124              reply_P(PSTR("ERR ARG"));
125              return;
126          }
127          state_set_tlim(value);
128          uart_puts_P(PSTR("OK"));
129          kv(PSTR(" tlim="), value);
130          uart_endl();
```

- **Linia 122:** W przeciwnym razie, jeśli komenda to `TLIM`...
- **Linia 123:** Zamień argument na liczbę z zakresu do 125. Jeśli się nie uda...
- **Linia 124:** ...odeślij `ERR ARG`...
- **Linia 125:** ...i zakończ.
- **Linia 126:** Koniec warunku.
- **Linia 127:** Ustaw nowy próg alarmu.
- **Linia 128:** Wyślij `OK`.
- **Linia 129:** Wyślij ` tlim=` z wartością.
- **Linia 130:** Wyślij koniec linii.

```c
131      } else if (*arg != '\0') {
132          // Pozostałe komendy nie przyjmują argumentów
133          if (strcmp_P(cmd, PSTR("PING")) == 0 || strcmp_P(cmd, PSTR("VER")) == 0
134              || strcmp_P(cmd, PSTR("ON")) == 0 || strcmp_P(cmd, PSTR("OFF")) == 0
135              || strcmp_P(cmd, PSTR("TOGGLE")) == 0 || strcmp_P(cmd, PSTR("GET")) == 0)
136              reply_P(PSTR("ERR ARG"));
137          else
138              reply_P(PSTR("ERR UNKNOWN"));
```

- **Linia 131:** W przeciwnym razie, jeśli podano jakikolwiek argument (a pozostałe komendy go nie przyjmują)...
- **Linia 132:** Komentarz wyjaśnia to samo.
- **Linia 133:** Sprawdź, czy to jedna ze znanych komend bez argumentów: `PING` albo `VER`...
- **Linia 134:** ...albo `ON`, albo `OFF`...
- **Linia 135:** ...albo `TOGGLE`, albo `GET`.
- **Linia 136:** Znana komenda z niepotrzebnym argumentem: odeślij `ERR ARG`.
- **Linia 137:** W przeciwnym razie:
- **Linia 138:** Odeślij `ERR UNKNOWN`, czyli „nieznana komenda”.

```c
139      } else if (strcmp_P(cmd, PSTR("PING")) == 0) {
140          reply_P(PSTR("PONG"));
141      } else if (strcmp_P(cmd, PSTR("VER")) == 0) {
142          reply_P(PSTR("VER " FW_VERSION));
143      } else if (strcmp_P(cmd, PSTR("ON")) == 0) {
144          state_set_on(1);
145          reply_on();
146      } else if (strcmp_P(cmd, PSTR("OFF")) == 0) {
147          state_set_on(0);
148          reply_on();
149      } else if (strcmp_P(cmd, PSTR("TOGGLE")) == 0) {
150          state_set_on(!state.on);
151          reply_on();
152      } else if (strcmp_P(cmd, PSTR("GET")) == 0) {
153          reply_state();
154      } else {
155          reply_P(PSTR("ERR UNKNOWN"));
156      }
157  }
```

- **Linia 139:** Komenda `PING`, czyli test łącza...
- **Linia 140:** ...odpowiedź to `PONG`.
- **Linia 141:** Komenda `VER`, czyli pytanie o wersję...
- **Linia 142:** ...odpowiedź to `VER 0.9.0`. Kompilator skleja napis `"VER "` ze stałą `FW_VERSION`.
- **Linia 143:** Komenda `ON`...
- **Linia 144:** ...włącz wyjście...
- **Linia 145:** ...i odeślij `OK on=1`.
- **Linia 146:** Komenda `OFF`...
- **Linia 147:** ...wyłącz wyjście...
- **Linia 148:** ...i odeślij `OK on=0`.
- **Linia 149:** Komenda `TOGGLE`, czyli przełącz...
- **Linia 150:** ...odwróć stan wyjścia...
- **Linia 151:** ...i odeślij nowy stan.
- **Linia 152:** Komenda `GET`...
- **Linia 153:** ...odeślij pełny stan.
- **Linia 154:** Żadna komenda nie pasuje:
- **Linia 155:** Odeślij `ERR UNKNOWN`.
- **Linia 156:** Koniec całego łańcucha warunków.
- **Linia 157:** Koniec funkcji `execute`.

### Odbiór kolejnych znaków

Pętla główna przekazuje tu każdy odebrany znak. Funkcja składa je w linię, a po znaku `\n` wykonuje komendę.

```c
159  uint8_t protocol_rx(char c)
160  {
161      if (c == '\r')
162          return 0;
163
164      if (c == '\n') {
165          uint8_t executed = 0;
166
167          while (len > 0 && line[len - 1] == ' ')
168              len--;
169          line[len] = '\0';
170          if (too_long) {
171              reply_P(PSTR("ERR TOOLONG"));
172              executed = 1;
173          } else if (len > 0) {
174              char *cmd = line;
175              while (*cmd == ' ')
176                  cmd++;
177              execute(cmd);
178              executed = 1;
179          }
180          len = 0;
181          too_long = 0;
182          return executed;
183      }
```

- **Linia 159:** Funkcja dostaje jeden znak. Zwraca 1, jeśli wykonano komendę.
- **Linia 160:** Początek funkcji.
- **Linia 161:** Jeśli to znak powrotu karetki `\r` (terminale dodają go do Entera)...
- **Linia 162:** ...pomiń go, zwróć 0.
- **Linia 164:** Jeśli to znak końca linii, komenda jest kompletna:
- **Linia 165:** Znacznik, czy coś wykonano. Na razie 0.
- **Linia 167:** Dopóki na końcu linii są spacje...
- **Linia 168:** ...skróć linię, czyli usuń końcowe spacje.
- **Linia 169:** Zakończ napis znakiem `'\0'`, by funkcje napisowe wiedziały, gdzie jest koniec.
- **Linia 170:** Jeśli linia była zbyt długa...
- **Linia 171:** ...odeślij `ERR TOOLONG`...
- **Linia 172:** ...i oznacz, że była reakcja.
- **Linia 173:** W przeciwnym razie, jeśli linia nie jest pusta:
- **Linia 174:** Ustaw wskaźnik `cmd` na początek linii.
- **Linia 175:** Dopóki na początku są spacje...
- **Linia 176:** ...przesuń wskaźnik, by je pominąć.
- **Linia 177:** Wykonaj komendę.
- **Linia 178:** Oznacz, że była reakcja.
- **Linia 179:** Koniec warunku.
- **Linia 180:** Wyzeruj długość, by przygotować bufor na następną linię.
- **Linia 181:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 182:** Zwróć wynik.
- **Linia 183:** Koniec obsługi końca linii.

```c
185      // Zgodność z instrukcją: pojedyncze 1 / 0 działa od razu, bez Entera
186      if (len == 0 && !too_long && (c == '1' || c == '0')) {
187          state_set_on(c == '1');
188          reply_on();
189          return 1;
190      }
191
192      if (len >= LINE_MAX) {
193          too_long = 1;
194          return 0;
195      }
196      if (c >= 'a' && c <= 'z')
197          c = (char)(c - 'a' + 'A');
198      line[len++] = c;
199      return 0;
200  }
```

- **Linia 185:** Komentarz: zgodność z instrukcją do etapu 2. Znak `1` lub `0` działa natychmiast, bez Entera.
- **Linia 186:** Jeśli linia jest pusta, nie trwa zbyt długa linia, a znak to `1` albo `0`...
- **Linia 187:** ...włącz lub wyłącz wyjście. Porównanie `c == '1'` daje 1 lub 0.
- **Linia 188:** Odeślij nowy stan.
- **Linia 189:** Zwróć 1: wykonano komendę.
- **Linia 190:** Koniec warunku.
- **Linia 192:** Jeśli w buforze nie ma już miejsca...
- **Linia 193:** ...zapamiętaj, że linia jest za długa...
- **Linia 194:** ...i zwróć 0. Błąd zostanie zgłoszony dopiero po końcu linii.
- **Linia 195:** Koniec warunku.
- **Linia 196:** Jeśli znak to mała litera...
- **Linia 197:** ...zamień ją na wielką: odejmij pozycję `a`, dodaj pozycję `A`. Dzięki temu wielkość liter nie ma znaczenia.
- **Linia 198:** Zapisz znak w buforze i zwiększ długość.
- **Linia 199:** Zwróć 0: linia jeszcze niekompletna.
- **Linia 200:** Koniec funkcji.

### Zdarzenia wysyłane samodzielnie

```c
202  void protocol_event_boot(PGM_P reason)
203  {
204      uart_puts_P(PSTR("EVT BOOT reset="));
205      uart_puts_P(reason);
206      uart_puts_P(PSTR(" ver=" FW_VERSION));
207      uart_endl();
208  }
```

- **Linia 202:** Funkcja wysyłająca informację o starcie układu.
- **Linia 203:** Początek funkcji.
- **Linia 204:** Wyślij początek wiadomości.
- **Linia 205:** Wyślij przyczynę resetu.
- **Linia 206:** Wyślij numer wersji. Kompilator skleja `" ver="` z wersją.
- **Linia 207:** Wyślij koniec linii.
- **Linia 208:** Koniec funkcji.

```c
210  void protocol_event_btn_on(void)
211  {
212      uart_puts_P(PSTR("EVT BTN"));
213      kv(PSTR(" on="), state.on);
214      uart_endl();
215  }
```

- **Linia 210:** Funkcja wysyłająca zdarzenie: przycisk przełączył wyjście.
- **Linia 211:** Początek funkcji.
- **Linia 212:** Wyślij `EVT BTN`.
- **Linia 213:** Wyślij nowy stan wyjścia.
- **Linia 214:** Wyślij koniec linii.
- **Linia 215:** Koniec funkcji.

```c
217  void protocol_event_btn_mode(void)
218  {
219      uart_puts_P(PSTR("EVT BTN"));
220      kv_mode();
221      uart_endl();
222  }
```

- **Linia 217:** Funkcja wysyłająca zdarzenie: przycisk zmienił tryb.
- **Linia 218:** Początek funkcji.
- **Linia 219:** Wyślij `EVT BTN`.
- **Linia 220:** Wyślij nowy tryb.
- **Linia 221:** Wyślij koniec linii.
- **Linia 222:** Koniec funkcji.

```c
224  void protocol_event_pot(uint8_t mode_changed)
225  {
226      uart_puts_P(PSTR("EVT POT"));
227      kv(PSTR(" pwm="), state.pwm);
228      if (mode_changed)
229          kv_mode();
230      uart_endl();
231  }
```

- **Linia 224:** Funkcja wysyłająca zdarzenie: potencjometr zmienił jasność. Parametr mówi, czy zmienił się też tryb.
- **Linia 225:** Początek funkcji.
- **Linia 226:** Wyślij `EVT POT`.
- **Linia 227:** Wyślij nową jasność.
- **Linia 228:** Jeśli tryb się zmienił...
- **Linia 229:** ...dopisz nowy tryb.
- **Linia 230:** Wyślij koniec linii.
- **Linia 231:** Koniec funkcji.

```c
233  void protocol_event_temp(int16_t temp)
234  {
235      uart_puts_P(PSTR("EVT TEMP"));
236      kv(PSTR(" alarm="), state.alarm);
237      kv_temp(temp);
238      uart_endl();
239  }
```

- **Linia 233:** Funkcja wysyłająca zdarzenie: alarm temperatury włączył się lub wyłączył.
- **Linia 234:** Początek funkcji.
- **Linia 235:** Wyślij `EVT TEMP`.
- **Linia 236:** Wyślij stan alarmu.
- **Linia 237:** Wyślij temperaturę.
- **Linia 238:** Wyślij koniec linii.
- **Linia 239:** Koniec funkcji.

---

[← Poprzedni](14-firmware-uart.md) · [Spis treści](README.md) · [Następny →](16-firmware-pwm.md)
