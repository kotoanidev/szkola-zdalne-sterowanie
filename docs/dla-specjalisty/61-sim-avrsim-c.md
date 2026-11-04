# Symulator: `sim/avrsim.c`, wirtualna płytka

[← Poprzedni](60-sim-makefile.md) · [Spis treści](README.md) · [Następny →](70-narzedzia-github-tymczasowo.md)

**Po co ten plik:** to program na zwykły komputer z Linuksem. Korzysta z biblioteki **simavr**, która wykonuje kod `firmware.hex` tak, jak zrobiłby to prawdziwy układ. Wokół niej budujemy wirtualną płytkę:

- UART jest dostępny przez **pseudoterminal**, czyli programową imitację portu szeregowego,
- przycisk i czujniki ustawia się poleceniami tekstowymi na standardowym wejściu (`stdin`),
- zdarzenia, np. zmiana jasności diody, wychodzą w formacie JSON na standardowe wyjście (`stdout`).

**Pojęcia z simavr:**

- `avr_t` to obiekt reprezentujący symulowany układ,
- **IRQ** w simavr to wirtualny przewód. Można podać na niego wartość (`avr_raise_irq`) albo poprosić o wiadomość, gdy się zmieni (`avr_irq_register_notify`),
- **hook** to nasza funkcja wywoływana przez bibliotekę, gdy coś się stanie.

Podstawy języka C są w [osobnym dokumencie](01-podstawy-jezyka-c.md).

## `sim/avrsim.c`

[Otwórz ten plik w repozytorium](../../sim/avrsim.c)

### Opis programu

```c
  1  // Symulacja firmware ATmega8 w simavr: UART na pseudoterminalu, wirtualny przycisk i czujniki.
  2  //
  3  //   avrsim [--speed 1.0] [--link ŚCIEŻKA] [--eeprom PLIK] [--sensor lm35|ntc] firmware.hex
  4  //
  5  // Sterowanie liniami na stdin: btn press [ms] | btn down | btn up | pot <0-1023> | ldr <0-1023>
  6  // | temp <°C> | mv <kanał> <mV> | reset | power | state | quit
  7  // Zdarzenia jako JSON (po jednym w linii) na stdout: ready, led, status, tx, btn, reset, state.
```

- **Linia 1:** Komentarz: symulacja firmware w simavr z UART na pseudoterminalu oraz wirtualnym przyciskiem i czujnikami.
- **Linia 2:** Pusty komentarz, odstęp.
- **Linia 3:** Sposób uruchomienia. Nawiasy kwadratowe oznaczają opcje, które można pominąć.
- **Linia 4:** Pusty komentarz, odstęp.
- **Linia 5:** Polecenia przyjmowane ze standardowego wejścia: przycisk, potencjometr, fotorezystor...
- **Linia 6:** ...temperatura, napięcie na kanale, reset, zanik zasilania, stan i wyjście.
- **Linia 7:** Na standardowe wyjście wychodzą zdarzenia w JSON, po jednym w linii.

### Biblioteki

```c
  8  #define _GNU_SOURCE
  9  #include <errno.h>
 10  #include <fcntl.h>
 11  #include <math.h>
 12  #include <poll.h>
 13  #include <signal.h>
 14  #include <stdarg.h>
 15  #include <stdio.h>
 16  #include <stdlib.h>
 17  #include <string.h>
 18  #include <termios.h>
 19  #include <time.h>
 20  #include <unistd.h>
```

- **Linia 8:** Włącza dodatkowe funkcje systemu Linux. Musi być przed wszystkimi `#include`.
- **Linia 9:** Kody błędów systemu, np. `errno`.
- **Linia 10:** Obsługa plików, np. `O_RDWR`.
- **Linia 11:** Funkcje matematyczne, np. `exp`.
- **Linia 12:** Czekanie na wiele źródeł danych: `poll`.
- **Linia 13:** Obsługa sygnałów, np. Ctrl+C.
- **Linia 14:** Funkcje ze zmienną liczbą parametrów.
- **Linia 15:** Wejście i wyjście, np. `printf`.
- **Linia 16:** Funkcje ogólne, np. `exit`.
- **Linia 17:** Praca z napisami.
- **Linia 18:** Ustawienia terminala.
- **Linia 19:** Odczyt czasu.
- **Linia 20:** Funkcje systemowe, np. `read`, `write`.

```c
 22  #include "sim_avr.h"
 23  #include "sim_elf.h"
 24  #include "sim_hex.h"
 25  #include "sim_io.h"
 26  #include "sim_time.h"
 27  #include "avr_adc.h"
 28  #include "avr_eeprom.h"
 29  #include "avr_ioport.h"
 30  #include "avr_uart.h"
```

- **Linia 22:** simavr: główny obiekt układu.
- **Linia 23:** simavr: opis firmware.
- **Linia 24:** simavr: odczyt plików `.hex`.
- **Linia 25:** simavr: wejścia i wyjścia.
- **Linia 26:** simavr: czas symulacji.
- **Linia 27:** simavr: przetwornik ADC.
- **Linia 28:** simavr: pamięć EEPROM.
- **Linia 29:** simavr: porty, czyli piny.
- **Linia 30:** simavr: UART.

### Stałe

```c
 32  #define F_CPU 1000000
 33  #define VREF_MV 5000
 34
 35  // Rejestry ATmega8 w przestrzeni danych (adres I/O + 0x20)
 36  #define REG_OCR1AL 0x4A
 37  #define REG_OCR1AH 0x4B
 38  #define REG_TCCR1A 0x4F
 39  #define REG_PORTB  0x38
 40  #define REG_MCUCSR 0x54
 41  #define BIT_COM1A1 7
 42  #define BIT_PORF   0
 43  #define BIT_EXTRF  1
 44
 45  #define LED_REPORT_NS 20000000ULL
```

- **Linia 32:** Częstotliwość zegara układu: 1 MHz.
- **Linia 33:** Napięcie odniesienia w miliwoltach.
- **Linia 35:** Komentarz: rejestry w przestrzeni danych. Adres w emulatorze to adres I/O plus `0x20`.
- **Linia 36:** Adres dolnego bajtu rejestru `OCR1A`, czyli wypełnienia PWM.
- **Linia 37:** Adres górnego bajtu rejestru `OCR1A`.
- **Linia 38:** Adres rejestru `TCCR1A`, sterującego Timer1.
- **Linia 39:** Adres rejestru wyjściowego portu B.
- **Linia 40:** Adres rejestru z przyczyną resetu.
- **Linia 41:** Numer bitu `COM1A1`, czyli „timer steruje pinem”.
- **Linia 42:** Numer bitu `PORF`, czyli „reset po włączeniu zasilania”.
- **Linia 43:** Numer bitu `EXTRF`, czyli „reset zewnętrzny”.
- **Linia 45:** Stan diody zgłaszamy najczęściej co 20 ms, czyli 20 000 000 ns. Unikamy dzięki temu zalania wyjścia zdarzeniami.

### Zmienne globalne

```c
 47  static avr_t *avr;
 48  static int pty_master = -1;
 49  static int pty_slave = -1;
 50  static const char *link_path;
 51  static const char *eeprom_path;
 52  static int sensor_ntc;
 53  static double speed = 1.0;
 54  static uint64_t t0;
 55  static volatile sig_atomic_t stop_requested;
```

- **Linia 47:** Wskaźnik na symulowany układ.
- **Linia 48:** Numer strony głównej pseudoterminala. `-1` oznacza „jeszcze nieotwarty”.
- **Linia 49:** Numer strony podrzędnej pseudoterminala.
- **Linia 50:** Ścieżka dowiązania do pseudoterminala, z opcji `--link`.
- **Linia 51:** Ścieżka pliku EEPROM, z opcji `--eeprom`.
- **Linia 52:** Czy czujnik to NTC.
- **Linia 53:** Szybkość symulacji, domyślnie równa czasowi rzeczywistemu.
- **Linia 54:** Chwila startu w czasie rzeczywistym.
- **Linia 55:** Znacznik zakończenia. `volatile sig_atomic_t` to typ bezpieczny do zmiany w obsłudze sygnału.

```c
 57  static int uart_xon;
 58  static uint8_t pending[256];
 59  static size_t pending_len, pending_pos;
 60  static char tx_line[128];
 61  static size_t tx_len;
```

- **Linia 57:** Czy UART układu może przyjąć znak.
- **Linia 58:** Bufor znaków czekających na podanie do układu.
- **Linia 59:** Liczba znaków w buforze i pozycja odczytu.
- **Linia 60:** Bufor bieżącej linii wysyłanej przez układ.
- **Linia 61:** Długość tej linii.

```c
 63  static avr_irq_t *uart_in;
 64  static avr_irq_t *button_irq;
 65  static avr_irq_t *adc_irq[3];
 66  static uint32_t adc_mv[3] = { 2500, 2500, 220 };
 67
 68  static double led_duty = -1;
 69  static uint64_t led_reported_ns;
```

- **Linia 63:** Wirtualny przewód: wejście UART.
- **Linia 64:** Wirtualny przewód: pin przycisku.
- **Linia 65:** Wirtualne przewody: trzy kanały ADC.
- **Linia 66:** Napięcia na kanałach w miliwoltach.
- **Linia 68:** Ostatnio zgłoszone wypełnienie. `-1` oznacza brak zgłoszenia.
- **Linia 69:** Chwila ostatniego zgłoszenia.

### Funkcje pomocnicze

```c
 71  static uint64_t now_ns(void)
 72  {
 73      struct timespec ts;
 74
 75      clock_gettime(CLOCK_MONOTONIC, &ts);
 76      return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
 77  }
 78
 79  static double sim_time(void)
 80  {
 81      return (double)avr->cycle / F_CPU;
 82  }
```

- **Linia 71:** Funkcja zwracająca bieżący czas w nanosekundach.
- **Linia 72:** Początek funkcji.
- **Linia 73:** Struktura z sekundami i nanosekundami.
- **Linia 75:** Odczytaj zegar, który nie cofa się przy zmianie godziny.
- **Linia 76:** Przelicz wszystko na nanosekundy.
- **Linia 77:** Koniec funkcji.
- **Linia 79:** Funkcja zwracająca czas symulacji w sekundach.
- **Linia 80:** Początek funkcji.
- **Linia 81:** Liczba wykonanych taktów podzielona przez częstotliwość.
- **Linia 82:** Koniec funkcji.

```c
 84  static void emit(const char *fmt, ...)
 85  {
 86      va_list ap;
 87
 88      va_start(ap, fmt);
 89      vprintf(fmt, ap);
 90      va_end(ap);
 91      putchar('\n');
 92      fflush(stdout);
 93  }
```

- **Linia 84:** Funkcja wypisująca jedną linię zdarzenia. Wielokropek oznacza dowolną liczbę parametrów, jak w `printf`.
- **Linia 85:** Początek funkcji.
- **Linia 86:** Zmienna do obsługi tych parametrów.
- **Linia 88:** Rozpocznij ich odczyt.
- **Linia 89:** Wypisz sformatowany tekst.
- **Linia 90:** Zakończ ich odczyt.
- **Linia 91:** Wypisz koniec linii.
- **Linia 92:** Wymuś natychmiastowe wysłanie, bo czeka na to serwer.
- **Linia 93:** Koniec funkcji.

```c
 95  // Logi simavr idą na stderr, żeby nie mieszały się z JSON na stdout
 96  static void logger(avr_t *a, const int level, const char *fmt, va_list ap)
 97  {
 98      (void)a;
 99      if (level <= LOG_WARNING)
100          vfprintf(stderr, fmt, ap);
101  }
102
103  static void sleep_noop(avr_t *a, avr_cycle_count_t how_long)
104  {
105      (void)a;
106      (void)how_long;
107  }
```

- **Linia 95:** Komentarz: komunikaty simavr idą na `stderr`, by nie mieszały się z JSON.
- **Linia 96:** Funkcja, którą simavr wywoła, by coś zapisać.
- **Linia 97:** Początek funkcji.
- **Linia 98:** `(void)a` oznacza „wiem, że ten parametr jest nieużywany”, co ucisza ostrzeżenie.
- **Linia 99:** Jeśli komunikat jest co najmniej ostrzeżeniem...
- **Linia 100:** ...wypisz go.
- **Linia 101:** Koniec funkcji.
- **Linia 103:** Funkcja zastępująca usypianie w simavr. Tempo pilnujemy sami w pętli głównej.
- **Linia 104:** Początek funkcji.
- **Linia 105:** Parametr nieużywany.
- **Linia 106:** Parametr nieużywany.
- **Linia 107:** Koniec funkcji. Celowo nic nie robi.

### UART i pseudoterminal

```c
109  // --- UART <-> pseudoterminal -------------------------------------------------
110
111  static void uart_out_hook(avr_irq_t *irq, uint32_t value, void *param)
112  {
113      uint8_t c = (uint8_t)value;
114      (void)irq;
115      (void)param;
116
117      // Gdy nikt nie czyta pseudoterminala, nadmiar znaków przepada jak na prawdziwym łączu
118      if (write(pty_master, &c, 1) < 0 && errno != EAGAIN)
119          perror("pty write");
120
121      if (c == '\n') {
122          tx_line[tx_len] = '\0';
123          emit("{\"ev\":\"tx\",\"t\":%.3f,\"line\":\"%s\"}", sim_time(), tx_line);
124          tx_len = 0;
125      } else if (c >= 0x20 && c < 0x7F && c != '"' && c != '\\' && tx_len < sizeof(tx_line) - 1) {
126          tx_line[tx_len++] = (char)c;
127      }
128  }
```

- **Linia 109:** Komentarz oddzielający tę część.
- **Linia 111:** Funkcja wywoływana, gdy układ wyśle znak.
- **Linia 112:** Początek funkcji.
- **Linia 113:** Zapamiętaj wysłany bajt.
- **Linia 114:** Parametr nieużywany.
- **Linia 115:** Parametr nieużywany.
- **Linia 117:** Komentarz: gdy nikt nie czyta, nadmiar znaków przepada, jak na prawdziwym łączu.
- **Linia 118:** Przekaż znak do pseudoterminala. Jeśli wystąpił błąd inny niż „bufor pełny”...
- **Linia 119:** ...wypisz go.
- **Linia 121:** Jeśli to koniec linii...
- **Linia 122:** ...zakończ napis...
- **Linia 123:** ...wypisz zdarzenie `tx` z całą linią...
- **Linia 124:** ...i wyzeruj licznik.
- **Linia 125:** W przeciwnym razie, jeśli to zwykły znak bez znaków psujących JSON, a w buforze jest miejsce...
- **Linia 126:** ...dopisz go do linii.
- **Linia 127:** Koniec warunku.
- **Linia 128:** Koniec funkcji.

```c
130  static void uart_xon_hook(avr_irq_t *irq, uint32_t value, void *param)
131  {
132      (void)irq;
133      (void)value;
134      (void)param;
135      uart_xon = 1;
136  }
137
138  static void uart_xoff_hook(avr_irq_t *irq, uint32_t value, void *param)
139  {
140      (void)irq;
141      (void)param;
142      if (value)
143          uart_xon = 0;
144  }
```

- **Linia 130:** Funkcja wywoływana, gdy UART układu jest gotowy przyjmować znaki.
- **Linia 131:** Początek funkcji.
- **Linia 132:** Parametr nieużywany.
- **Linia 133:** Parametr nieużywany.
- **Linia 134:** Parametr nieużywany.
- **Linia 135:** Zapamiętaj, że można podawać znaki.
- **Linia 136:** Koniec funkcji.
- **Linia 138:** Funkcja wywoływana, gdy UART układu nie może przyjmować znaków.
- **Linia 139:** Początek funkcji.
- **Linia 140:** Parametr nieużywany.
- **Linia 141:** Parametr nieużywany.
- **Linia 142:** Jeśli sygnał jest aktywny...
- **Linia 143:** ...zapamiętaj, by wstrzymać podawanie.
- **Linia 144:** Koniec funkcji.

```c
146  static void feed_uart(void)
147  {
148      while (uart_xon && pending_pos < pending_len)
149          avr_raise_irq(uart_in, pending[pending_pos++]);
150      if (pending_pos == pending_len)
151          pending_pos = pending_len = 0;
152  }
153
154  static void read_pty(void)
155  {
156      ssize_t n;
157
158      if (pending_len > 0)
159          return;
160      n = read(pty_master, pending, sizeof(pending));
161      if (n > 0)
162          pending_len = (size_t)n;
163  }
```

- **Linia 146:** Funkcja podająca znaki do układu.
- **Linia 147:** Początek funkcji.
- **Linia 148:** Dopóki układ jest gotowy i są znaki w buforze...
- **Linia 149:** ...podaj kolejny znak na wirtualny przewód.
- **Linia 150:** Jeśli wszystko podano...
- **Linia 151:** ...wyzeruj bufor.
- **Linia 152:** Koniec funkcji.
- **Linia 154:** Funkcja czytająca znaki z pseudoterminala.
- **Linia 155:** Początek funkcji.
- **Linia 156:** Zmienna na liczbę odczytanych bajtów.
- **Linia 158:** Jeśli w buforze wciąż coś jest...
- **Linia 159:** ...zakończ, poczekamy, aż się opróżni.
- **Linia 160:** Odczytaj znaki do bufora.
- **Linia 161:** Jeśli coś odczytano...
- **Linia 162:** ...zapisz ich liczbę.
- **Linia 163:** Koniec funkcji.

```c
165  static void open_pty(void)
166  {
167      struct termios t;
168      const char *name;
169
170      pty_master = posix_openpt(O_RDWR | O_NOCTTY);
171      if (pty_master < 0 || grantpt(pty_master) < 0 || unlockpt(pty_master) < 0) {
172          perror("pty");
173          exit(1);
174      }
175      name = ptsname(pty_master);
176      // Trzymamy stronę slave otwartą, żeby odłączenie klienta nie zamykało łącza
177      pty_slave = open(name, O_RDWR | O_NOCTTY);
178      if (pty_slave < 0 || tcgetattr(pty_slave, &t) < 0) {
179          perror("pty slave");
180          exit(1);
181      }
182      cfmakeraw(&t);
183      cfsetspeed(&t, B9600);
184      tcsetattr(pty_slave, TCSANOW, &t);
185      fcntl(pty_master, F_SETFL, fcntl(pty_master, F_GETFL) | O_NONBLOCK);
186
187      if (link_path) {
188          unlink(link_path);
189          if (symlink(name, link_path) < 0)
190              perror("symlink");
191      }
192      emit("{\"ev\":\"ready\",\"pty\":\"%s\",\"link\":\"%s\"}", name, link_path ? link_path : "");
193  }
```

- **Linia 165:** Funkcja tworząca pseudoterminal.
- **Linia 166:** Początek funkcji.
- **Linia 167:** Struktura z ustawieniami terminala.
- **Linia 168:** Ścieżka pseudoterminala.
- **Linia 170:** Utwórz stronę główną pseudoterminala.
- **Linia 171:** Jeśli wystąpił błąd lub nie udało się nadać uprawnień albo odblokować strony podrzędnej...
- **Linia 172:** ...wypisz błąd...
- **Linia 173:** ...i zakończ program.
- **Linia 174:** Koniec warunku.
- **Linia 175:** Pobierz ścieżkę strony podrzędnej, np. `/dev/pts/3`.
- **Linia 176:** Komentarz: stronę podrzędną trzymamy otwartą, by odłączenie klienta nie zamykało łącza.
- **Linia 177:** Otwórz stronę podrzędną.
- **Linia 178:** Jeśli wystąpił błąd lub nie można odczytać ustawień...
- **Linia 179:** ...wypisz błąd...
- **Linia 180:** ...i zakończ program.
- **Linia 181:** Koniec warunku.
- **Linia 182:** Ustaw tryb surowy, czyli bajty bez żadnych przeróbek.
- **Linia 183:** Ustaw prędkość 9600 bitów na sekundę.
- **Linia 184:** Zastosuj ustawienia od razu.
- **Linia 185:** Ustaw odczyt bez czekania, by program się nie blokował.
- **Linia 187:** Jeśli podano opcję `--link`...
- **Linia 188:** ...usuń stare dowiązanie...
- **Linia 189:** ...i utwórz nowe. Jeśli się nie uda...
- **Linia 190:** ...wypisz błąd.
- **Linia 191:** Koniec warunku.
- **Linia 192:** Wypisz zdarzenie gotowości ze ścieżką. Na to czeka serwer.
- **Linia 193:** Koniec funkcji.

### Diody

```c
195  // --- wirtualny sprzęt ----------------------------------------------------------
196
197  static void status_hook(avr_irq_t *irq, uint32_t value, void *param)
198  {
199      (void)irq;
200      (void)param;
201      emit("{\"ev\":\"status\",\"t\":%.3f,\"on\":%u}", sim_time(), value ? 1 : 0);
202  }
203
204  static double read_led_duty(void)
205  {
206      uint16_t ocr;
207
208      if (!(avr->data[REG_TCCR1A] & (1 << BIT_COM1A1)))
209          return (avr->data[REG_PORTB] >> 1) & 1;
210      ocr = (uint16_t)(avr->data[REG_OCR1AL] | (avr->data[REG_OCR1AH] << 8));
211      return ocr >= 1023 ? 1.0 : ocr / 1023.0;
212  }
```

- **Linia 195:** Komentarz oddzielający część „wirtualny sprzęt”.
- **Linia 197:** Funkcja wywoływana, gdy zmieni się pin diody statusu.
- **Linia 198:** Początek funkcji.
- **Linia 199:** Parametr nieużywany.
- **Linia 200:** Parametr nieużywany.
- **Linia 201:** Wypisz zdarzenie `status`.
- **Linia 202:** Koniec funkcji.
- **Linia 204:** Funkcja odczytująca wypełnienie PWM z rejestrów układu.
- **Linia 205:** Początek funkcji.
- **Linia 206:** Zmienna na wartość rejestru.
- **Linia 208:** Jeśli timer nie steruje pinem...
- **Linia 209:** ...zwróć stan samego pinu `PB1`: 0 lub 1.
- **Linia 210:** Złóż 16-bitową wartość z dwóch bajtów.
- **Linia 211:** Zwróć wypełnienie od 0 do 1.
- **Linia 212:** Koniec funkcji.

```c
214  static void report_led(int force)
215  {
216      double duty = read_led_duty();
217      uint64_t now = now_ns();
218
219      if (!force && (fabs(duty - led_duty) < 1e-9 || now - led_reported_ns < LED_REPORT_NS))
220          return;
221      led_duty = duty;
222      led_reported_ns = now;
223      emit("{\"ev\":\"led\",\"t\":%.3f,\"duty\":%.4f}", sim_time(), duty);
224  }
```

- **Linia 214:** Funkcja zgłaszająca stan diody. Parametr `force` wymusza zgłoszenie.
- **Linia 215:** Początek funkcji.
- **Linia 216:** Odczytaj wypełnienie.
- **Linia 217:** Odczytaj czas.
- **Linia 219:** Jeśli nie ma wymuszenia, a wartość się nie zmieniła lub minęło za mało czasu...
- **Linia 220:** ...zakończ.
- **Linia 221:** Zapamiętaj wartość.
- **Linia 222:** Zapamiętaj czas.
- **Linia 223:** Wypisz zdarzenie `led`.
- **Linia 224:** Koniec funkcji.

### Przycisk

```c
226  // Przycisk to zewnętrzne wymuszenie stanu pinu; samo podniesienie IRQ nadpisałby pull-up
227  // przy każdym zapisie PORTD (dioda statusu jest zapisywana co 1 ms)
228  static void drive_button(int pressed)
229  {
230      avr_ioport_external_t ext = { .name = 'D', .mask = 1 << 2, .value = pressed ? 0 : 1 << 2 };
231
232      avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL('D'), &ext);
233      avr_raise_irq(button_irq, pressed ? 0 : 1);
234  }
235
236  static void set_button(int pressed)
237  {
238      drive_button(pressed);
239      emit("{\"ev\":\"btn\",\"t\":%.3f,\"pressed\":%d}", sim_time(), pressed);
240  }
241
242  static avr_cycle_count_t release_button(avr_t *a, avr_cycle_count_t when, void *param)
243  {
244      (void)a;
245      (void)when;
246      (void)param;
247      set_button(0);
248      return 0;
249  }
```

- **Linia 226:** Komentarz: przycisk to zewnętrzne wymuszenie stanu pinu, bo samo podniesienie IRQ nadpisałby pull-up...
- **Linia 227:** ...przy każdym zapisie `PORTD`, a dioda statusu jest zapisywana co 1 ms.
- **Linia 228:** Funkcja ustawiająca pin przycisku.
- **Linia 229:** Początek funkcji.
- **Linia 230:** Opis wymuszenia: port D, bit 2, a wartość 0 przy wciśnięciu.
- **Linia 232:** Przekaż wymuszenie do emulatora.
- **Linia 233:** Podaj też nową wartość na wirtualny przewód.
- **Linia 234:** Koniec funkcji.
- **Linia 236:** Funkcja naciskająca lub puszczająca przycisk.
- **Linia 237:** Początek funkcji.
- **Linia 238:** Ustaw pin.
- **Linia 239:** Wypisz zdarzenie `btn`.
- **Linia 240:** Koniec funkcji.
- **Linia 242:** Funkcja wywoływana po upływie czasu naciśnięcia.
- **Linia 243:** Początek funkcji.
- **Linia 244:** Parametr nieużywany.
- **Linia 245:** Parametr nieużywany.
- **Linia 246:** Parametr nieużywany.
- **Linia 247:** Puść przycisk.
- **Linia 248:** Zwróć 0, czyli nie powtarzaj tego wywołania.
- **Linia 249:** Koniec funkcji.

### Wejścia analogowe

```c
251  static void set_adc(int ch, uint32_t mv)
252  {
253      if (mv > VREF_MV)
254          mv = VREF_MV;
255      adc_mv[ch] = mv;
256      avr_raise_irq(adc_irq[ch], mv);
257  }
258
259  static uint32_t raw_to_mv(long raw)
260  {
261      if (raw < 0)
262          raw = 0;
263      if (raw > 1023)
264          raw = 1023;
265      // ADC w simavr liczy mV * 1023 / VREF; środek przedziału daje dokładnie tę wartość
266      return (uint32_t)(((double)raw + 0.5) * VREF_MV / 1023.0);
267  }
268
269  static uint32_t temp_to_mv(double c)
270  {
271      if (sensor_ntc) {
272          // NTC 10 kΩ (B = 3950) od VCC, 10 kΩ do GND – jak w firmware/include/config.h
273          double r_ntc = 10000.0 * exp(3950.0 * (1.0 / (c + 273.15) - 1.0 / 298.15));
274          return (uint32_t)(VREF_MV * 10000.0 / (10000.0 + r_ntc));
275      }
276      return c <= 0 ? 0 : (uint32_t)(c * 10.0 + 0.5);
277  }
```

- **Linia 251:** Funkcja ustawiająca napięcie na kanale.
- **Linia 252:** Początek funkcji.
- **Linia 253:** Jeśli napięcie przekracza napięcie odniesienia...
- **Linia 254:** ...przytnij je.
- **Linia 255:** Zapamiętaj wartość.
- **Linia 256:** Podaj ją na wirtualny przewód.
- **Linia 257:** Koniec funkcji.
- **Linia 259:** Funkcja przeliczająca odczyt ADC na miliwolty.
- **Linia 260:** Początek funkcji.
- **Linia 261:** Jeśli odczyt jest ujemny...
- **Linia 262:** ...ustaw 0.
- **Linia 263:** Jeśli przekracza 1023...
- **Linia 264:** ...ustaw 1023.
- **Linia 265:** Komentarz: dodanie 0,5 daje środek przedziału, czyli dokładnie żądany odczyt.
- **Linia 266:** Przelicz na miliwolty.
- **Linia 267:** Koniec funkcji.
- **Linia 269:** Funkcja przeliczająca temperaturę na napięcie z czujnika.
- **Linia 270:** Początek funkcji.
- **Linia 271:** Jeśli czujnikiem jest NTC...
- **Linia 272:** Komentarz: termistor 10 kΩ od zasilania i 10 kΩ do masy.
- **Linia 273:** Oblicz opór termistora wzorem z parametrem B (3950). 273,15 przelicza stopnie Celsjusza na kelwiny, a 298,15 K to 25 °C.
- **Linia 274:** Oblicz napięcie z dzielnika napięcia.
- **Linia 275:** Koniec warunku.
- **Linia 276:** Dla LM35: 10 mV na stopień. Dla temperatury ujemnej 0 mV.
- **Linia 277:** Koniec funkcji.

```c
279  static void apply_inputs(void)
280  {
281      int ch;
282
283      drive_button(0);
284      for (ch = 0; ch < 3; ch++)
285          avr_raise_irq(adc_irq[ch], adc_mv[ch]);
286  }
```

- **Linia 279:** Funkcja ustawiająca wejścia po starcie lub resecie.
- **Linia 280:** Początek funkcji.
- **Linia 281:** Licznik pętli.
- **Linia 283:** Przycisk puszczony.
- **Linia 284:** Dla każdego kanału...
- **Linia 285:** ...podaj jego napięcie.
- **Linia 286:** Koniec funkcji.

### Reset

```c
288  static void do_reset(int bit, const char *reason)
289  {
290      avr_reset(avr);
291      // avr_reset() zeruje licznik cykli, więc czas rzeczywisty liczymy od nowa
292      t0 = now_ns();
293      avr->data[REG_MCUCSR] |= (uint8_t)(1 << bit);
294      apply_inputs();
295      emit("{\"ev\":\"reset\",\"t\":%.3f,\"reason\":\"%s\"}", sim_time(), reason);
296  }
```

- **Linia 288:** Funkcja resetująca układ. Dostaje bit przyczyny i nazwę.
- **Linia 289:** Początek funkcji.
- **Linia 290:** Zresetuj układ w emulatorze.
- **Linia 291:** Komentarz: reset zeruje licznik taktów, więc czas rzeczywisty liczymy od nowa.
- **Linia 292:** Zapamiętaj nowy czas startu.
- **Linia 293:** Ustaw w rejestrze bit przyczyny resetu. Firmware go odczyta.
- **Linia 294:** Podaj wejścia ponownie.
- **Linia 295:** Wypisz zdarzenie `reset`.
- **Linia 296:** Koniec funkcji.

### Pamięć EEPROM w pliku

Dzięki temu zawartość EEPROM przetrwa „zanik zasilania”, czyli zamknięcie programu.

```c
298  // --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------
299
300  static void eeprom_io(int save)
301  {
302      uint8_t buf[512];
303      avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
304      FILE *f;
305
306      if (!eeprom_path)
307          return;
308      if (save) {
309          if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
310              return;
311          fwrite(d.ee, 1, d.size, f);
312          fclose(f);
313      } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
314          d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
315          fclose(f);
316          if (d.size > 0)
317              avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
318      }
319  }
```

- **Linia 298:** Komentarz oddzielający tę część.
- **Linia 300:** Funkcja zapisująca lub wczytująca EEPROM. Parametr `save` wybiera działanie.
- **Linia 301:** Początek funkcji.
- **Linia 302:** Bufor o rozmiarze pamięci EEPROM układu.
- **Linia 303:** Opis obszaru pamięci dla emulatora.
- **Linia 304:** Wskaźnik na plik.
- **Linia 306:** Jeśli nie podano pliku...
- **Linia 307:** ...zakończ.
- **Linia 308:** Przy zapisie:
- **Linia 309:** Pobierz zawartość EEPROM i otwórz plik do zapisu. Jeśli się nie uda...
- **Linia 310:** ...zakończ.
- **Linia 311:** Zapisz dane do pliku.
- **Linia 312:** Zamknij plik.
- **Linia 313:** Przy odczycie, jeśli plik da się otworzyć:
- **Linia 314:** Wczytaj dane i zapisz ich rozmiar.
- **Linia 315:** Zamknij plik.
- **Linia 316:** Jeśli coś wczytano...
- **Linia 317:** ...przekaż to do emulatora.
- **Linia 318:** Koniec warunku.
- **Linia 319:** Koniec funkcji.

### Polecenia ze standardowego wejścia

```c
321  // --- sterowanie z stdin ---------------------------------------------------------
322
323  static void command(char *line)
324  {
325      char cmd[16] = "", arg[16] = "";
326      double v1 = 0, v2 = 0;
327      int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);
328
329      if (n <= 0)
330          return;
```

- **Linia 321:** Komentarz oddzielający tę część.
- **Linia 323:** Funkcja wykonująca jedno polecenie.
- **Linia 324:** Początek funkcji.
- **Linia 325:** Bufory na nazwę polecenia i jego argument.
- **Linia 326:** Zmienne na liczby.
- **Linia 327:** Odczytaj z linii słowo i dwie liczby. `n` to liczba odczytanych elementów.
- **Linia 329:** Jeśli niczego nie odczytano...
- **Linia 330:** ...zakończ.

```c
331      if (strcmp(cmd, "btn") == 0) {
332          sscanf(line, "%*s %15s %lf", arg, &v1);
333          if (strcmp(arg, "down") == 0) {
334              set_button(1);
335          } else if (strcmp(arg, "up") == 0) {
336              set_button(0);
337          } else {
338              double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
339              set_button(1);
340              avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
341                                       release_button, NULL);
342          }
```

- **Linia 331:** Polecenie `btn`:
- **Linia 332:** Odczytaj jeszcze raz: pomiń pierwsze słowo (`%*s`), weź drugie i liczbę.
- **Linia 333:** Dla `down`...
- **Linia 334:** ...naciśnij przycisk.
- **Linia 335:** Dla `up`...
- **Linia 336:** ...puść przycisk.
- **Linia 337:** W pozostałych przypadkach:
- **Linia 338:** Czas naciśnięcia: podany dla `press`, a w przeciwnym razie 100 ms.
- **Linia 339:** Naciśnij przycisk.
- **Linia 340:** Zaplanuj puszczenie po upływie tego czasu symulacji, przeliczonego na takty.
- **Linia 341:** Funkcja do wywołania i brak dodatkowych danych.
- **Linia 342:** Koniec warunku.

```c
343      } else if (strcmp(cmd, "pot") == 0 && n >= 2) {
344          set_adc(0, raw_to_mv((long)v1));
345      } else if (strcmp(cmd, "ldr") == 0 && n >= 2) {
346          set_adc(1, raw_to_mv((long)v1));
347      } else if (strcmp(cmd, "temp") == 0 && n >= 2) {
348          set_adc(2, temp_to_mv(v1));
349      } else if (strcmp(cmd, "mv") == 0 && n >= 3 && v1 >= 0 && v1 < 3) {
350          set_adc((int)v1, (uint32_t)v2);
351      } else if (strcmp(cmd, "reset") == 0) {
352          do_reset(BIT_EXTRF, "EXT");
353      } else if (strcmp(cmd, "power") == 0) {
354          eeprom_io(1);
355          do_reset(BIT_PORF, "POR");
356      } else if (strcmp(cmd, "state") == 0) {
357          emit("{\"ev\":\"state\",\"t\":%.3f,\"duty\":%.4f,\"pot_mv\":%u,\"ldr_mv\":%u,\"temp_mv\":%u}",
358               sim_time(), read_led_duty(), adc_mv[0], adc_mv[1], adc_mv[2]);
359      } else if (strcmp(cmd, "quit") == 0) {
360          stop_requested = 1;
361      } else {
362          fprintf(stderr, "avrsim: nieznana komenda: %s\n", line);
363      }
364  }
```

- **Linia 343:** Polecenie `pot` z liczbą:
- **Linia 344:** Ustaw napięcie na kanale 0.
- **Linia 345:** Polecenie `ldr` z liczbą:
- **Linia 346:** Ustaw napięcie na kanale 1.
- **Linia 347:** Polecenie `temp` z liczbą:
- **Linia 348:** Ustaw napięcie na kanale 2.
- **Linia 349:** Polecenie `mv` z numerem kanału i napięciem:
- **Linia 350:** Ustaw napięcie na wskazanym kanale.
- **Linia 351:** Polecenie `reset`:
- **Linia 352:** Wykonaj reset zewnętrzny.
- **Linia 353:** Polecenie `power`:
- **Linia 354:** Zapisz EEPROM do pliku.
- **Linia 355:** Wykonaj reset jak po włączeniu zasilania.
- **Linia 356:** Polecenie `state`:
- **Linia 357:** Wypisz zdarzenie ze stanem...
- **Linia 358:** ...podając czas, wypełnienie i napięcia.
- **Linia 359:** Polecenie `quit`:
- **Linia 360:** Ustaw znacznik zakończenia.
- **Linia 361:** Nieznane polecenie:
- **Linia 362:** Wypisz błąd.
- **Linia 363:** Koniec łańcucha warunków.
- **Linia 364:** Koniec funkcji.

```c
366  static void read_stdin(void)
367  {
368      static char buf[256];
369      static size_t len;
370      ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
371      char *start, *nl;
372
373      if (n == 0) {
374          stop_requested = 1;
375          return;
376      }
377      if (n < 0)
378          return;
379      len += (size_t)n;
380      buf[len] = '\0';
381      start = buf;
382      while ((nl = strchr(start, '\n')) != NULL) {
383          *nl = '\0';
384          command(start);
385          start = nl + 1;
386      }
387      len = strlen(start);
388      memmove(buf, start, len);
389      if (len == sizeof(buf) - 1)
390          len = 0;
391  }
```

- **Linia 366:** Funkcja czytająca ze standardowego wejścia i dzieląca na linie.
- **Linia 367:** Początek funkcji.
- **Linia 368:** Bufor. `static` wewnątrz funkcji: zachowuje zawartość między wywołaniami.
- **Linia 369:** Liczba znaków w buforze. Też zachowuje wartość.
- **Linia 370:** Dopisz do bufora to, co jest na wejściu.
- **Linia 371:** Wskaźniki: początek linii i koniec linii.
- **Linia 373:** Jeśli odczytano 0 bajtów, wejście zostało zamknięte...
- **Linia 374:** ...ustaw znacznik zakończenia...
- **Linia 375:** ...i zakończ funkcję.
- **Linia 376:** Koniec warunku.
- **Linia 377:** Jeśli wystąpił błąd lub nie ma danych...
- **Linia 378:** ...zakończ funkcję.
- **Linia 379:** Powiększ licznik.
- **Linia 380:** Zakończ napis.
- **Linia 381:** Zacznij od początku bufora.
- **Linia 382:** Dopóki jest znak końca linii...
- **Linia 383:** ...zamień go na koniec napisu...
- **Linia 384:** ...wykonaj polecenie...
- **Linia 385:** ...i przejdź za niego.
- **Linia 386:** Koniec pętli.
- **Linia 387:** Zapisz długość reszty, czyli niepełnej linii.
- **Linia 388:** Przesuń resztę na początek bufora.
- **Linia 389:** Jeśli bufor jest pełny, a końca linii wciąż nie ma...
- **Linia 390:** ...porzuć tę linię.
- **Linia 391:** Koniec funkcji.

```c
393  static void on_signal(int sig)
394  {
395      (void)sig;
396      stop_requested = 1;
397  }
398
399  static void usage(const char *prog)
400  {
401      fprintf(stderr, "użycie: %s [--speed X] [--link ŚCIEŻKA] [--eeprom PLIK] "
402                      "[--sensor lm35|ntc] firmware.hex\n", prog);
403      exit(2);
404  }
```

- **Linia 393:** Funkcja obsługi sygnału, np. Ctrl+C.
- **Linia 394:** Początek funkcji.
- **Linia 395:** Parametr nieużywany.
- **Linia 396:** Ustaw znacznik zakończenia.
- **Linia 397:** Koniec funkcji.
- **Linia 399:** Funkcja wypisująca instrukcję użycia.
- **Linia 400:** Początek funkcji.
- **Linia 401:** Wypisz pierwszą część...
- **Linia 402:** ...i drugą część.
- **Linia 403:** Zakończ program z kodem błędu 2.
- **Linia 404:** Koniec funkcji.

### Funkcja główna: start

```c
406  int main(int argc, char *argv[])
407  {
408      elf_firmware_t fw;
409      const char *hex = NULL;
410      int i, status = 0;
411
412      for (i = 1; i < argc; i++) {
413          if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
414              speed = atof(argv[++i]);
415          else if (strcmp(argv[i], "--link") == 0 && i + 1 < argc)
416              link_path = argv[++i];
417          else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc)
418              eeprom_path = argv[++i];
419          else if (strcmp(argv[i], "--sensor") == 0 && i + 1 < argc)
420              sensor_ntc = strcmp(argv[++i], "ntc") == 0;
421          else if (argv[i][0] != '-' && !hex)
422              hex = argv[i];
423          else
424              usage(argv[0]);
425      }
426      if (!hex)
427          usage(argv[0]);
```

- **Linia 406:** Początek programu. `argc` to liczba argumentów, a `argv` to ich lista.
- **Linia 407:** Początek funkcji.
- **Linia 408:** Opis firmware.
- **Linia 409:** Ścieżka pliku `.hex`, na razie brak.
- **Linia 410:** Licznik i kod zakończenia.
- **Linia 412:** Przejdź po argumentach, zaczynając od pierwszego prawdziwego.
- **Linia 413:** Jeśli to `--speed` i jest następny argument...
- **Linia 414:** ...odczytaj szybkość i przesuń się dalej.
- **Linia 415:** Jeśli to `--link`...
- **Linia 416:** ...zapisz ścieżkę dowiązania.
- **Linia 417:** Jeśli to `--eeprom`...
- **Linia 418:** ...zapisz ścieżkę pliku.
- **Linia 419:** Jeśli to `--sensor`...
- **Linia 420:** ...zapisz, czy to NTC.
- **Linia 421:** Jeśli argument nie zaczyna się od minusa i nie ma jeszcze pliku...
- **Linia 422:** ...to jest to plik firmware.
- **Linia 423:** W pozostałych przypadkach:
- **Linia 424:** Wypisz instrukcję i zakończ program.
- **Linia 425:** Koniec pętli.
- **Linia 426:** Jeśli nie podano pliku...
- **Linia 427:** ...wypisz instrukcję i zakończ program.

```c
429      avr_global_logger_set(logger);
430      memset(&fw, 0, sizeof(fw));
431      strcpy(fw.mmcu, "atmega8");
432      fw.frequency = F_CPU;
433      sim_setup_firmware(hex, 0, &fw, argv[0]);
434
435      avr = avr_make_mcu_by_name(fw.mmcu);
436      if (!avr) {
437          fprintf(stderr, "avrsim: brak rdzenia atmega8 w simavr\n");
438          return 1;
439      }
440      avr_init(avr);
441      avr_load_firmware(avr, &fw);
442      avr->vcc = avr->avcc = avr->aref = VREF_MV;
443      avr->sleep = sleep_noop;
444      avr->data[REG_MCUCSR] |= 1 << BIT_PORF;
445      eeprom_io(0);
```

- **Linia 429:** Ustaw naszą funkcję zapisywania komunikatów.
- **Linia 430:** Wyzeruj opis firmware.
- **Linia 431:** Wpisz nazwę układu.
- **Linia 432:** Wpisz częstotliwość.
- **Linia 433:** Wczytaj plik firmware.
- **Linia 435:** Utwórz układ o podanej nazwie.
- **Linia 436:** Jeśli się nie udało...
- **Linia 437:** ...wypisz błąd...
- **Linia 438:** ...i zakończ z kodem 1.
- **Linia 439:** Koniec warunku.
- **Linia 440:** Przygotuj układ.
- **Linia 441:** Załaduj do niego firmware.
- **Linia 442:** Ustaw napięcia zasilania i odniesienia.
- **Linia 443:** Zastąp usypianie pustą funkcją.
- **Linia 444:** Ustaw w rejestrze przyczynę resetu: włączenie zasilania.
- **Linia 445:** Wczytaj EEPROM z pliku.

### Podłączanie przewodów

```c
447      {
448          uint32_t flags = 0;
449          avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
450          flags &= ~(uint32_t)AVR_UART_FLAG_STDIO;
451          avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
452      }
453      uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
454      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
455                              uart_out_hook, NULL);
456      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XON),
457                              uart_xon_hook, NULL);
458      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XOFF),
459                              uart_xoff_hook, NULL);
460      button_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN2);
461      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN7),
462                              status_hook, NULL);
463      for (i = 0; i < 3; i++)
464          adc_irq[i] = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0 + i);
465      adc_mv[2] = temp_to_mv(22.0);
466      apply_inputs();
```

- **Linia 447:** Początek wewnętrznego bloku, by zmienna `flags` istniała tylko tutaj.
- **Linia 448:** Zmienna na flagi UART.
- **Linia 449:** Odczytaj flagi.
- **Linia 450:** Wyłącz flagę, która kazałaby simavr wypisywać znaki na ekran. Sami się tym zajmujemy.
- **Linia 451:** Zapisz flagi.
- **Linia 452:** Koniec bloku.
- **Linia 453:** Pobierz wirtualny przewód wejścia UART.
- **Linia 454:** Podłącz naszą funkcję do wyjścia UART...
- **Linia 455:** ...czyli funkcję `uart_out_hook`.
- **Linia 456:** Podłącz naszą funkcję do sygnału gotowości...
- **Linia 457:** ...czyli funkcję `uart_xon_hook`.
- **Linia 458:** Podłącz naszą funkcję do sygnału wstrzymania...
- **Linia 459:** ...czyli funkcję `uart_xoff_hook`.
- **Linia 460:** Pobierz wirtualny przewód pinu przycisku, czyli pinu 2 portu D.
- **Linia 461:** Podłącz naszą funkcję do pinu 7 portu D, czyli diody statusu...
- **Linia 462:** ...czyli funkcję `status_hook`.
- **Linia 463:** Dla trzech kanałów...
- **Linia 464:** ...pobierz wirtualny przewód ADC.
- **Linia 465:** Ustaw początkową temperaturę: 22 °C.
- **Linia 466:** Podaj wejścia.

```c
468      signal(SIGINT, on_signal);
469      signal(SIGTERM, on_signal);
470      signal(SIGPIPE, SIG_IGN);
471      fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
472      open_pty();
473      report_led(1);
474
475      t0 = now_ns();
```

- **Linia 468:** Przy sygnale Ctrl+C wywołaj `on_signal`.
- **Linia 469:** Tak samo przy prośbie o zakończenie.
- **Linia 470:** Ignoruj błąd pisania do zamkniętego wyjścia, by program nie kończył się nagle.
- **Linia 471:** Ustaw odczyt wejścia bez czekania.
- **Linia 472:** Utwórz pseudoterminal.
- **Linia 473:** Zgłoś początkowy stan diody.
- **Linia 475:** Zapisz czas startu.

### Pętla główna

```c
476      while (!stop_requested) {
477          struct pollfd fds[2] = {
478              { .fd = STDIN_FILENO, .events = POLLIN },
479              { .fd = pty_master, .events = POLLIN },
480          };
481          avr_cycle_count_t target = speed > 0
482              ? (avr_cycle_count_t)((double)(now_ns() - t0) * 1e-9 * speed * F_CPU)
483              : avr->cycle + 50000;
484          avr_cycle_count_t budget = avr->cycle + 100000;
```

- **Linia 476:** Dopóki nie ma polecenia końca...
- **Linia 477:** Lista źródeł, na które czekamy:
- **Linia 478:** Standardowe wejście.
- **Linia 479:** Pseudoterminal.
- **Linia 480:** Koniec listy.
- **Linia 481:** Oblicz, do którego taktu ma dojść symulacja. Jeśli szybkość jest dodatnia...
- **Linia 482:** ...to czas rzeczywisty razy szybkość razy częstotliwość...
- **Linia 483:** ...a w przeciwnym razie, czyli przy trybie bez limitu, o 50 000 taktów naprzód.
- **Linia 484:** Wykonaj najwyżej 100 000 taktów, by program mógł obsłużyć też inne rzeczy.

```c
486          while (avr->cycle < target && avr->cycle < budget) {
487              int state = avr_run(avr);
488              if (state == cpu_Done || state == cpu_Crashed) {
489                  emit("{\"ev\":\"halt\",\"t\":%.3f,\"state\":%d}", sim_time(), state);
490                  stop_requested = 1;
491                  status = 1;
492                  break;
493              }
494          }
```

- **Linia 486:** Dopóki nie doszliśmy do celu ani do limitu...
- **Linia 487:** ...wykonaj kolejny krok układu.
- **Linia 488:** Jeśli układ się zatrzymał lub wystąpił błąd...
- **Linia 489:** ...wypisz zdarzenie...
- **Linia 490:** ...ustaw znacznik zakończenia...
- **Linia 491:** ...ustaw kod błędu...
- **Linia 492:** ...i przerwij pętlę.
- **Linia 493:** Koniec warunku.
- **Linia 494:** Koniec pętli.

```c
495          feed_uart();
496          report_led(0);
497          if (poll(fds, 2, avr->cycle >= target ? 1 : 0) > 0) {
498              if (fds[0].revents & (POLLIN | POLLHUP))
499                  read_stdin();
500              if (fds[1].revents & POLLIN)
501                  read_pty();
502          }
503      }
```

- **Linia 495:** Podaj oczekujące znaki do układu.
- **Linia 496:** Zgłoś stan diody, jeśli się zmienił.
- **Linia 497:** Czekaj na dane. Jeśli symulacja doszła do celu, najwyżej 1 ms, a w przeciwnym razie wcale.
- **Linia 498:** Jeśli coś jest na wejściu lub je zamknięto...
- **Linia 499:** ...przeczytaj to.
- **Linia 500:** Jeśli coś jest na pseudoterminalu...
- **Linia 501:** ...przeczytaj to.
- **Linia 502:** Koniec warunku.
- **Linia 503:** Koniec pętli głównej.

### Zakończenie

```c
505      eeprom_io(1);
506      if (link_path)
507          unlink(link_path);
508      avr_terminate(avr);
509      return status;
510  }
```

- **Linia 505:** Zapisz EEPROM do pliku.
- **Linia 506:** Jeśli było dowiązanie...
- **Linia 507:** ...usuń je.
- **Linia 508:** Zakończ pracę emulatora.
- **Linia 509:** Zwróć kod zakończenia.
- **Linia 510:** Koniec programu.

---

[← Poprzedni](60-sim-makefile.md) · [Spis treści](README.md) · [Następny →](70-narzedzia-github-tymczasowo.md)
