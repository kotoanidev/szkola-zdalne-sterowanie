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
  1  // Symulacja firmware ATmega32A (płytka ATB 1.05) w simavr: UART na pseudoterminalu, wirtualny
  2  // przycisk, potencjometr, fotorezystor i czujnik DS18B20 na 1-Wire.
  3  //
  4  //   avrsim [--speed 1.0] [--freq 1000000] [--link ŚCIEŻKA] [--eeprom PLIK] firmware.hex
  5  //
  6  // Sterowanie liniami na stdin: btn press [ms] | btn down | btn up | pot <0-1023> | ldr <0-1023>
  7  // | temp <°C> | temp none | mv <kanał> <mV> | reset | power | state | quit
  8  // Zdarzenia jako JSON (po jednym w linii) na stdout: ready, led, status, tx, btn, reset, state.
```

⚠- **Linia 1:** Komentarz: symulacja firmware w simavr z UART na pseudoterminalu oraz wirtualnym przyciskiem i czujnikami.
- **Linia 3:** Pusty komentarz, odstęp.
⚠- **Linia 4:** Sposób uruchomienia. Nawiasy kwadratowe oznaczają opcje, które można pominąć.
- **Linia 5:** Pusty komentarz, odstęp.
- **Linia 6:** Polecenia przyjmowane ze standardowego wejścia: przycisk, potencjometr, fotorezystor...
⚠- **Linia 7:** ...temperatura, napięcie na kanale, reset, zanik zasilania, stan i wyjście.
- **Linia 8:** Na standardowe wyjście wychodzą zdarzenia w JSON, po jednym w linii.

### Biblioteki

```c
  9  #define _GNU_SOURCE
 10  #include <errno.h>
 11  #include <fcntl.h>
 12  #include <math.h>
 13  #include <poll.h>
 14  #include <signal.h>
 15  #include <stdarg.h>
 16  #include <stdio.h>
 17  #include <stdlib.h>
 18  #include <string.h>
 19  #include <termios.h>
 20  #include <time.h>
 21  #include <unistd.h>
```

- **Linia 9:** Włącza dodatkowe funkcje systemu Linux. Musi być przed wszystkimi `#include`.
- **Linia 10:** Kody błędów systemu, np. `errno`.
- **Linia 11:** Obsługa plików, np. `O_RDWR`.
- **Linia 12:** Funkcje matematyczne, np. `exp`.
- **Linia 13:** Czekanie na wiele źródeł danych: `poll`.
- **Linia 14:** Obsługa sygnałów, np. Ctrl+C.
- **Linia 15:** Funkcje ze zmienną liczbą parametrów.
- **Linia 16:** Wejście i wyjście, np. `printf`.
- **Linia 17:** Funkcje ogólne, np. `exit`.
- **Linia 18:** Praca z napisami.
- **Linia 19:** Ustawienia terminala.
- **Linia 20:** Odczyt czasu.
- **Linia 21:** Funkcje systemowe, np. `read`, `write`.

```c
 23  #include "sim_avr.h"
 24  #include "sim_elf.h"
 25  #include "sim_hex.h"
 26  #include "sim_io.h"
 27  #include "sim_time.h"
 28  #include "avr_adc.h"
 29  #include "avr_eeprom.h"
 30  #include "avr_ioport.h"
 31  #include "avr_uart.h"
```

- **Linia 23:** simavr: główny obiekt układu.
- **Linia 24:** simavr: opis firmware.
- **Linia 25:** simavr: odczyt plików `.hex`.
- **Linia 26:** simavr: wejścia i wyjścia.
- **Linia 27:** simavr: czas symulacji.
- **Linia 28:** simavr: przetwornik ADC.
- **Linia 29:** simavr: pamięć EEPROM.
- **Linia 30:** simavr: porty, czyli piny.
- **Linia 31:** simavr: UART.

### Stałe

```c
 32
 33  #define VREF_MV 5000
 34  #define ADC_INPUTS 2
 35
 36  // Diody na ATB świecą stanem niskim – jak LED_ACTIVE_LOW w firmware/include/pinout.h
 37  #define LED_ACTIVE_LOW 1
 38  #define LED_BIT    5   // PD5 / OC1A
 39  #define STATUS_BIT 0   // PC0
 40  #define BTN_BIT    2   // PD2
 41  #define OW_BIT     3   // PD3, DS18B20
 42
 43  // Rejestry ATmega32 w przestrzeni danych (adres I/O + 0x20)
 44  #define REG_OCR1AL 0x4A
 45  #define REG_OCR1AH 0x4B
 46  #define REG_TCCR1A 0x4F
 47  #define REG_PORTD  0x32
 48  #define REG_MCUCSR 0x54
 49  #define BIT_COM1A1 7
 50  #define BIT_COM1A0 6
 51  #define BIT_PORF   0
 52  #define BIT_EXTRF  1
 53
 54  #define LED_REPORT_NS 20000000ULL
```

⚠- **Linia 32:** Częstotliwość zegara układu: 1 MHz.
- **Linia 33:** Napięcie odniesienia w miliwoltach.
⚠- **Linia 35:** Komentarz: rejestry w przestrzeni danych. Adres w emulatorze to adres I/O plus `0x20`.
- **Linia 44:** Adres dolnego bajtu rejestru `OCR1A`, czyli wypełnienia PWM.
- **Linia 45:** Adres górnego bajtu rejestru `OCR1A`.
- **Linia 46:** Adres rejestru `TCCR1A`, sterującego Timer1.
⚠- **Linia 47:** Adres rejestru wyjściowego portu B.
- **Linia 48:** Adres rejestru z przyczyną resetu.
- **Linia 49:** Numer bitu `COM1A1`, czyli „timer steruje pinem”.
- **Linia 51:** Numer bitu `PORF`, czyli „reset po włączeniu zasilania”.
- **Linia 52:** Numer bitu `EXTRF`, czyli „reset zewnętrzny”.
- **Linia 54:** Stan diody zgłaszamy najczęściej co 20 ms, czyli 20 000 000 ns. Unikamy dzięki temu zalania wyjścia zdarzeniami.

### Zmienne globalne

```c
 52  #define BIT_EXTRF  1
 53
 54  #define LED_REPORT_NS 20000000ULL
 55
 56  static avr_t *avr;
 57  static int pty_master = -1;
 58  static int pty_slave = -1;
 59  static const char *link_path;
 60  static const char *eeprom_path;
 61  static double speed = 1.0;
 62  // Zegar jak F_CPU firmware: 1 MHz (RC) albo 11059200 (kwarc ATB)
 63  static uint32_t freq = 1000000;
 64  static uint64_t t0;
 65  static volatile sig_atomic_t stop_requested;
```

- **Linia 56:** Wskaźnik na symulowany układ.
- **Linia 57:** Numer strony głównej pseudoterminala. `-1` oznacza „jeszcze nieotwarty”.
- **Linia 58:** Numer strony podrzędnej pseudoterminala.
- **Linia 59:** Ścieżka dowiązania do pseudoterminala, z opcji `--link`.
- **Linia 60:** Ścieżka pliku EEPROM, z opcji `--eeprom`.
⚠- **Linia 52:** Czy czujnik to NTC.
- **Linia 61:** Szybkość symulacji, domyślnie równa czasowi rzeczywistemu.
- **Linia 64:** Chwila startu w czasie rzeczywistym.
- **Linia 65:** Znacznik zakończenia. `volatile sig_atomic_t` to typ bezpieczny do zmiany w obsłudze sygnału.

```c
 67  static int uart_xon;
 68  static uint8_t pending[256];
 69  static size_t pending_len, pending_pos;
 70  static char tx_line[128];
 71  static size_t tx_len;
```

- **Linia 67:** Czy UART układu może przyjąć znak.
- **Linia 68:** Bufor znaków czekających na podanie do układu.
- **Linia 69:** Liczba znaków w buforze i pozycja odczytu.
- **Linia 70:** Bufor bieżącej linii wysyłanej przez układ.
- **Linia 71:** Długość tej linii.

```c
 65  static volatile sig_atomic_t stop_requested;
 66
 67  static int uart_xon;
 68  static uint8_t pending[256];
 69  static size_t pending_len, pending_pos;
 70  static char tx_line[128];
 71  static size_t tx_len;
 72
 73  static avr_irq_t *uart_in;
 74  static avr_irq_t *button_irq;
 75  static avr_irq_t *adc_irq[ADC_INPUTS];
 76  static uint32_t adc_mv[ADC_INPUTS] = { 2500, 2500 };
 77  static int button_down;
 78
 79  // DS18B20: stan czujnika i automat protokołu 1-Wire
 80  enum { OW_IDLE, OW_ROM, OW_FUNC, OW_TX };
 81  static avr_irq_t *ow_irq;
 82  static int ow_present = 1;
 83  static int16_t ow_raw = 22 * 16;
 84  static int16_t ow_converted = 85 * 16;   // wartość po włączeniu zasilania
 85  static int ow_state = OW_IDLE;
 86  static int ow_master_low;
 87  static int ow_slave_low;
 88  static avr_cycle_count_t ow_fall;
 89  static uint8_t ow_in, ow_bits;
 90  static uint8_t ow_tx[9];
 91  static int ow_tx_bit;
 92
 93  static double led_duty = -1;
 94  static uint64_t led_reported_ns;
```

- **Linia 73:** Wirtualny przewód: wejście UART.
- **Linia 74:** Wirtualny przewód: pin przycisku.
⚠- **Linia 65:** Wirtualne przewody: trzy kanały ADC.
⚠- **Linia 66:** Napięcia na kanałach w miliwoltach.
- **Linia 93:** Ostatnio zgłoszone wypełnienie. `-1` oznacza brak zgłoszenia.
- **Linia 94:** Chwila ostatniego zgłoszenia.

### Funkcje pomocnicze

```c
 96  static uint64_t now_ns(void)
 97  {
 98      struct timespec ts;
 99
100      clock_gettime(CLOCK_MONOTONIC, &ts);
101      return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
102  }
103
104  static double sim_time(void)
105  {
106      return (double)avr->cycle / freq;
107  }
```

- **Linia 96:** Funkcja zwracająca bieżący czas w nanosekundach.
- **Linia 97:** Początek funkcji.
- **Linia 98:** Struktura z sekundami i nanosekundami.
- **Linia 100:** Odczytaj zegar, który nie cofa się przy zmianie godziny.
- **Linia 101:** Przelicz wszystko na nanosekundy.
- **Linia 102:** Koniec funkcji.
- **Linia 104:** Funkcja zwracająca czas symulacji w sekundach.
- **Linia 105:** Początek funkcji.
⚠- **Linia 106:** Liczba wykonanych taktów podzielona przez częstotliwość.
- **Linia 107:** Koniec funkcji.

```c
109  static void emit(const char *fmt, ...)
110  {
111      va_list ap;
112
113      va_start(ap, fmt);
114      vprintf(fmt, ap);
115      va_end(ap);
116      putchar('\n');
117      fflush(stdout);
118  }
```

- **Linia 109:** Funkcja wypisująca jedną linię zdarzenia. Wielokropek oznacza dowolną liczbę parametrów, jak w `printf`.
- **Linia 110:** Początek funkcji.
- **Linia 111:** Zmienna do obsługi tych parametrów.
- **Linia 113:** Rozpocznij ich odczyt.
- **Linia 114:** Wypisz sformatowany tekst.
- **Linia 115:** Zakończ ich odczyt.
- **Linia 116:** Wypisz koniec linii.
- **Linia 117:** Wymuś natychmiastowe wysłanie, bo czeka na to serwer.
- **Linia 118:** Koniec funkcji.

```c
120  // Logi simavr idą na stderr, żeby nie mieszały się z JSON na stdout
121  static void logger(avr_t *a, const int level, const char *fmt, va_list ap)
122  {
123      (void)a;
124      if (level <= LOG_WARNING)
125          vfprintf(stderr, fmt, ap);
126  }
127
128  static void sleep_noop(avr_t *a, avr_cycle_count_t how_long)
129  {
130      (void)a;
131      (void)how_long;
132  }
```

- **Linia 120:** Komentarz: komunikaty simavr idą na `stderr`, by nie mieszały się z JSON.
- **Linia 121:** Funkcja, którą simavr wywoła, by coś zapisać.
- **Linia 122:** Początek funkcji.
- **Linia 123:** `(void)a` oznacza „wiem, że ten parametr jest nieużywany”, co ucisza ostrzeżenie.
- **Linia 124:** Jeśli komunikat jest co najmniej ostrzeżeniem...
- **Linia 125:** ...wypisz go.
- **Linia 126:** Koniec funkcji.
- **Linia 128:** Funkcja zastępująca usypianie w simavr. Tempo pilnujemy sami w pętli głównej.
- **Linia 129:** Początek funkcji.
- **Linia 130:** Parametr nieużywany.
- **Linia 131:** Parametr nieużywany.
- **Linia 132:** Koniec funkcji. Celowo nic nie robi.

### UART i pseudoterminal

```c
134  // --- UART <-> pseudoterminal -------------------------------------------------
135
136  static void uart_out_hook(avr_irq_t *irq, uint32_t value, void *param)
137  {
138      uint8_t c = (uint8_t)value;
139      (void)irq;
140      (void)param;
141
142      // Gdy nikt nie czyta pseudoterminala, nadmiar znaków przepada jak na prawdziwym łączu
143      if (write(pty_master, &c, 1) < 0 && errno != EAGAIN)
144          perror("pty write");
145
146      if (c == '\n') {
147          tx_line[tx_len] = '\0';
148          emit("{\"ev\":\"tx\",\"t\":%.3f,\"line\":\"%s\"}", sim_time(), tx_line);
149          tx_len = 0;
150      } else if (c >= 0x20 && c < 0x7F && c != '"' && c != '\\' && tx_len < sizeof(tx_line) - 1) {
151          tx_line[tx_len++] = (char)c;
152      }
153  }
```

- **Linia 134:** Komentarz oddzielający tę część.
- **Linia 136:** Funkcja wywoływana, gdy układ wyśle znak.
- **Linia 137:** Początek funkcji.
- **Linia 138:** Zapamiętaj wysłany bajt.
- **Linia 139:** Parametr nieużywany.
- **Linia 140:** Parametr nieużywany.
- **Linia 142:** Komentarz: gdy nikt nie czyta, nadmiar znaków przepada, jak na prawdziwym łączu.
- **Linia 143:** Przekaż znak do pseudoterminala. Jeśli wystąpił błąd inny niż „bufor pełny”...
- **Linia 144:** ...wypisz go.
- **Linia 146:** Jeśli to koniec linii...
- **Linia 147:** ...zakończ napis...
- **Linia 148:** ...wypisz zdarzenie `tx` z całą linią...
- **Linia 149:** ...i wyzeruj licznik.
- **Linia 150:** W przeciwnym razie, jeśli to zwykły znak bez znaków psujących JSON, a w buforze jest miejsce...
- **Linia 151:** ...dopisz go do linii.
- **Linia 152:** Koniec warunku.
- **Linia 153:** Koniec funkcji.

```c
155  static void uart_xon_hook(avr_irq_t *irq, uint32_t value, void *param)
156  {
157      (void)irq;
158      (void)value;
159      (void)param;
160      uart_xon = 1;
161  }
162
163  static void uart_xoff_hook(avr_irq_t *irq, uint32_t value, void *param)
164  {
165      (void)irq;
166      (void)param;
167      if (value)
168          uart_xon = 0;
169  }
```

- **Linia 155:** Funkcja wywoływana, gdy UART układu jest gotowy przyjmować znaki.
- **Linia 156:** Początek funkcji.
- **Linia 157:** Parametr nieużywany.
- **Linia 158:** Parametr nieużywany.
- **Linia 159:** Parametr nieużywany.
- **Linia 160:** Zapamiętaj, że można podawać znaki.
- **Linia 161:** Koniec funkcji.
- **Linia 163:** Funkcja wywoływana, gdy UART układu nie może przyjmować znaków.
- **Linia 164:** Początek funkcji.
- **Linia 165:** Parametr nieużywany.
- **Linia 166:** Parametr nieużywany.
- **Linia 167:** Jeśli sygnał jest aktywny...
- **Linia 168:** ...zapamiętaj, by wstrzymać podawanie.
- **Linia 169:** Koniec funkcji.

```c
171  static void feed_uart(void)
172  {
173      while (uart_xon && pending_pos < pending_len)
174          avr_raise_irq(uart_in, pending[pending_pos++]);
175      if (pending_pos == pending_len)
176          pending_pos = pending_len = 0;
177  }
178
179  static void read_pty(void)
180  {
181      ssize_t n;
182
183      if (pending_len > 0)
184          return;
185      n = read(pty_master, pending, sizeof(pending));
186      if (n > 0)
187          pending_len = (size_t)n;
188  }
```

- **Linia 171:** Funkcja podająca znaki do układu.
- **Linia 172:** Początek funkcji.
- **Linia 173:** Dopóki układ jest gotowy i są znaki w buforze...
- **Linia 174:** ...podaj kolejny znak na wirtualny przewód.
- **Linia 175:** Jeśli wszystko podano...
- **Linia 176:** ...wyzeruj bufor.
- **Linia 177:** Koniec funkcji.
- **Linia 179:** Funkcja czytająca znaki z pseudoterminala.
- **Linia 180:** Początek funkcji.
- **Linia 181:** Zmienna na liczbę odczytanych bajtów.
- **Linia 183:** Jeśli w buforze wciąż coś jest...
- **Linia 184:** ...zakończ, poczekamy, aż się opróżni.
- **Linia 185:** Odczytaj znaki do bufora.
- **Linia 186:** Jeśli coś odczytano...
- **Linia 187:** ...zapisz ich liczbę.
- **Linia 188:** Koniec funkcji.

```c
190  static void open_pty(void)
191  {
192      struct termios t;
193      const char *name;
194
195      pty_master = posix_openpt(O_RDWR | O_NOCTTY);
196      if (pty_master < 0 || grantpt(pty_master) < 0 || unlockpt(pty_master) < 0) {
197          perror("pty");
198          exit(1);
199      }
200      name = ptsname(pty_master);
201      // Trzymamy stronę slave otwartą, żeby odłączenie klienta nie zamykało łącza
202      pty_slave = open(name, O_RDWR | O_NOCTTY);
203      if (pty_slave < 0 || tcgetattr(pty_slave, &t) < 0) {
204          perror("pty slave");
205          exit(1);
206      }
207      cfmakeraw(&t);
208      cfsetspeed(&t, B9600);
209      tcsetattr(pty_slave, TCSANOW, &t);
210      fcntl(pty_master, F_SETFL, fcntl(pty_master, F_GETFL) | O_NONBLOCK);
211
212      if (link_path) {
213          unlink(link_path);
214          if (symlink(name, link_path) < 0)
215              perror("symlink");
216      }
217      emit("{\"ev\":\"ready\",\"pty\":\"%s\",\"link\":\"%s\"}", name, link_path ? link_path : "");
218  }
```

- **Linia 190:** Funkcja tworząca pseudoterminal.
- **Linia 191:** Początek funkcji.
- **Linia 192:** Struktura z ustawieniami terminala.
- **Linia 193:** Ścieżka pseudoterminala.
- **Linia 195:** Utwórz stronę główną pseudoterminala.
- **Linia 196:** Jeśli wystąpił błąd lub nie udało się nadać uprawnień albo odblokować strony podrzędnej...
- **Linia 197:** ...wypisz błąd...
- **Linia 198:** ...i zakończ program.
- **Linia 199:** Koniec warunku.
- **Linia 200:** Pobierz ścieżkę strony podrzędnej, np. `/dev/pts/3`.
- **Linia 201:** Komentarz: stronę podrzędną trzymamy otwartą, by odłączenie klienta nie zamykało łącza.
- **Linia 202:** Otwórz stronę podrzędną.
- **Linia 203:** Jeśli wystąpił błąd lub nie można odczytać ustawień...
- **Linia 204:** ...wypisz błąd...
- **Linia 205:** ...i zakończ program.
- **Linia 206:** Koniec warunku.
- **Linia 207:** Ustaw tryb surowy, czyli bajty bez żadnych przeróbek.
- **Linia 208:** Ustaw prędkość 9600 bitów na sekundę.
- **Linia 209:** Zastosuj ustawienia od razu.
- **Linia 210:** Ustaw odczyt bez czekania, by program się nie blokował.
- **Linia 212:** Jeśli podano opcję `--link`...
- **Linia 213:** ...usuń stare dowiązanie...
- **Linia 214:** ...i utwórz nowe. Jeśli się nie uda...
- **Linia 215:** ...wypisz błąd.
- **Linia 216:** Koniec warunku.
- **Linia 217:** Wypisz zdarzenie gotowości ze ścieżką. Na to czeka serwer.
- **Linia 218:** Koniec funkcji.

### Diody

```c
201      // Trzymamy stronę slave otwartą, żeby odłączenie klienta nie zamykało łącza
202      pty_slave = open(name, O_RDWR | O_NOCTTY);
203      if (pty_slave < 0 || tcgetattr(pty_slave, &t) < 0) {
204          perror("pty slave");
205          exit(1);
206      }
207      cfmakeraw(&t);
208      cfsetspeed(&t, B9600);
209      tcsetattr(pty_slave, TCSANOW, &t);
210      fcntl(pty_master, F_SETFL, fcntl(pty_master, F_GETFL) | O_NONBLOCK);
211
212      if (link_path) {
213          unlink(link_path);
214          if (symlink(name, link_path) < 0)
215              perror("symlink");
216      }
217      emit("{\"ev\":\"ready\",\"pty\":\"%s\",\"link\":\"%s\"}", name, link_path ? link_path : "");
218  }
219
220  // --- wirtualny sprzęt ----------------------------------------------------------
221
222  static void status_hook(avr_irq_t *irq, uint32_t value, void *param)
223  {
224      (void)irq;
225      (void)param;
226      emit("{\"ev\":\"status\",\"t\":%.3f,\"on\":%u}", sim_time(),
227           (value ? 1 : 0) ^ LED_ACTIVE_LOW);
228  }
229
230  // Jasność diody głównej = część okresu, w której dioda przewodzi
231  static double read_led_duty(void)
232  {
233      uint8_t tccr = avr->data[REG_TCCR1A];
234      double high;
235
236      if (!(tccr & (1 << BIT_COM1A1))) {
237          high = (avr->data[REG_PORTD] >> LED_BIT) & 1;
238      } else {
239          uint16_t ocr = (uint16_t)(avr->data[REG_OCR1AL] | (avr->data[REG_OCR1AH] << 8));
240          double d = ocr >= 1023 ? 1.0 : ocr / 1023.0;
241          high = (tccr & (1 << BIT_COM1A0)) ? 1.0 - d : d;
242      }
243      return LED_ACTIVE_LOW ? 1.0 - high : high;
244  }
```

- **Linia 220:** Komentarz oddzielający część „wirtualny sprzęt”.
- **Linia 222:** Funkcja wywoływana, gdy zmieni się pin diody statusu.
- **Linia 223:** Początek funkcji.
- **Linia 224:** Parametr nieużywany.
- **Linia 225:** Parametr nieużywany.
⚠- **Linia 201:** Wypisz zdarzenie `status`.
- **Linia 228:** Koniec funkcji.
- **Linia 231:** Funkcja odczytująca wypełnienie PWM z rejestrów układu.
- **Linia 232:** Początek funkcji.
⚠- **Linia 206:** Zmienna na wartość rejestru.
⚠- **Linia 208:** Jeśli timer nie steruje pinem...
⚠- **Linia 209:** ...zwróć stan samego pinu `PB1`: 0 lub 1.
⚠- **Linia 210:** Złóż 16-bitową wartość z dwóch bajtów.
⚠- **Linia 211:** Zwróć wypełnienie od 0 do 1.
- **Linia 244:** Koniec funkcji.

```c
246  static void report_led(int force)
247  {
248      double duty = read_led_duty();
249      uint64_t now = now_ns();
250
251      if (!force && (fabs(duty - led_duty) < 1e-9 || now - led_reported_ns < LED_REPORT_NS))
252          return;
253      led_duty = duty;
254      led_reported_ns = now;
255      emit("{\"ev\":\"led\",\"t\":%.3f,\"duty\":%.4f}", sim_time(), duty);
256  }
```

- **Linia 246:** Funkcja zgłaszająca stan diody. Parametr `force` wymusza zgłoszenie.
- **Linia 247:** Początek funkcji.
- **Linia 248:** Odczytaj wypełnienie.
- **Linia 249:** Odczytaj czas.
- **Linia 251:** Jeśli nie ma wymuszenia, a wartość się nie zmieniła lub minęło za mało czasu...
- **Linia 252:** ...zakończ.
- **Linia 253:** Zapamiętaj wartość.
- **Linia 254:** Zapamiętaj czas.
- **Linia 255:** Wypisz zdarzenie `led`.
- **Linia 256:** Koniec funkcji.

### Przycisk

```c
226      emit("{\"ev\":\"status\",\"t\":%.3f,\"on\":%u}", sim_time(),
227           (value ? 1 : 0) ^ LED_ACTIVE_LOW);
228  }
229
230  // Jasność diody głównej = część okresu, w której dioda przewodzi
231  static double read_led_duty(void)
232  {
233      uint8_t tccr = avr->data[REG_TCCR1A];
234      double high;
235
236      if (!(tccr & (1 << BIT_COM1A1))) {
237          high = (avr->data[REG_PORTD] >> LED_BIT) & 1;
238      } else {
239          uint16_t ocr = (uint16_t)(avr->data[REG_OCR1AL] | (avr->data[REG_OCR1AH] << 8));
240          double d = ocr >= 1023 ? 1.0 : ocr / 1023.0;
241          high = (tccr & (1 << BIT_COM1A0)) ? 1.0 - d : d;
242      }
243      return LED_ACTIVE_LOW ? 1.0 - high : high;
244  }
245
246  static void report_led(int force)
247  {
248      double duty = read_led_duty();
249      uint64_t now = now_ns();
250
251      if (!force && (fabs(duty - led_duty) < 1e-9 || now - led_reported_ns < LED_REPORT_NS))
252          return;
253      led_duty = duty;
254      led_reported_ns = now;
255      emit("{\"ev\":\"led\",\"t\":%.3f,\"duty\":%.4f}", sim_time(), duty);
256  }
257
258  // Przycisk (PD2) i linia 1-Wire (PD3) to zewnętrzne wymuszenie stanu pinów; samo podniesienie
259  // IRQ zostałoby nadpisane przy następnym zapisie PORTD. Obie linie mają pull-up.
260  static void portd_external(void)
261  {
262      avr_ioport_external_t ext = {
263          .name = 'D',
264          .mask = (1 << BTN_BIT) | (1 << OW_BIT),
265          .value = (button_down ? 0 : 1 << BTN_BIT) | (ow_slave_low ? 0 : 1 << OW_BIT),
266      };
267
268      avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL('D'), &ext);
269  }
270
271  static void drive_button(int pressed)
272  {
273      button_down = pressed;
274      portd_external();
275      avr_raise_irq(button_irq, pressed ? 0 : 1);
276  }
277
278  static void set_button(int pressed)
279  {
280      drive_button(pressed);
281      emit("{\"ev\":\"btn\",\"t\":%.3f,\"pressed\":%d}", sim_time(), pressed);
282  }
283
284  static avr_cycle_count_t release_button(avr_t *a, avr_cycle_count_t when, void *param)
285  {
286      (void)a;
287      (void)when;
288      (void)param;
289      set_button(0);
290      return 0;
291  }
```

⚠- **Linia 226:** Komentarz: przycisk to zewnętrzne wymuszenie stanu pinu, bo samo podniesienie IRQ nadpisałby pull-up...
⚠- **Linia 227:** ...przy każdym zapisie `PORTD`, a dioda statusu jest zapisywana co 1 ms.
- **Linia 271:** Funkcja ustawiająca pin przycisku.
- **Linia 272:** Początek funkcji.
⚠- **Linia 230:** Opis wymuszenia: port D, bit 2, a wartość 0 przy wciśnięciu.
⚠- **Linia 232:** Przekaż wymuszenie do emulatora.
- **Linia 275:** Podaj też nową wartość na wirtualny przewód.
- **Linia 276:** Koniec funkcji.
- **Linia 278:** Funkcja naciskająca lub puszczająca przycisk.
- **Linia 279:** Początek funkcji.
- **Linia 280:** Ustaw pin.
- **Linia 281:** Wypisz zdarzenie `btn`.
- **Linia 282:** Koniec funkcji.
- **Linia 284:** Funkcja wywoływana po upływie czasu naciśnięcia.
- **Linia 285:** Początek funkcji.
- **Linia 286:** Parametr nieużywany.
- **Linia 287:** Parametr nieużywany.
- **Linia 288:** Parametr nieużywany.
- **Linia 289:** Puść przycisk.
- **Linia 290:** Zwróć 0, czyli nie powtarzaj tego wywołania.
- **Linia 291:** Koniec funkcji.

### Wejścia analogowe

```c
269  }
270
271  static void drive_button(int pressed)
272  {
273      button_down = pressed;
274      portd_external();
275      avr_raise_irq(button_irq, pressed ? 0 : 1);
276  }
277
278  static void set_button(int pressed)
279  {
280      drive_button(pressed);
281      emit("{\"ev\":\"btn\",\"t\":%.3f,\"pressed\":%d}", sim_time(), pressed);
282  }
283
284  static avr_cycle_count_t release_button(avr_t *a, avr_cycle_count_t when, void *param)
285  {
286      (void)a;
287      (void)when;
288      (void)param;
289      set_button(0);
290      return 0;
291  }
292
293  static void set_adc(int ch, uint32_t mv)
294  {
295      if (mv > VREF_MV)
296          mv = VREF_MV;
297      adc_mv[ch] = mv;
298      avr_raise_irq(adc_irq[ch], mv);
299  }
300
301  static uint32_t raw_to_mv(long raw)
302  {
303      if (raw < 0)
304          raw = 0;
305      if (raw > 1023)
306          raw = 1023;
307      // ADC w simavr liczy mV * 1023 / VREF; środek przedziału daje dokładnie tę wartość
308      return (uint32_t)(((double)raw + 0.5) * VREF_MV / 1023.0);
309  }
310
311  // --- DS18B20 na 1-Wire --------------------------------------------------------------
312  // Czasy z karty katalogowej w wariancie najmniej korzystnym dla mikrokontrolera: bit 0 trzyma
313  // linię tylko 15 µs, obecność trwa od 60 do 120 µs po zwolnieniu linii przez reset.
314
315  static void ow_drive(int low)
316  {
317      ow_slave_low = low;
318      portd_external();
319      avr_raise_irq(ow_irq, low ? 0 : 1);
320  }
321
322  static avr_cycle_count_t ow_drive_timer(avr_t *a, avr_cycle_count_t when, void *param)
323  {
324      (void)a;
325      (void)when;
326      ow_drive(param != NULL);
327      return 0;
328  }
329
330  static void ow_schedule(uint32_t us, int low)
331  {
332      avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, us), ow_drive_timer,
333                               low ? (void *)1 : NULL);
334  }
335
336  static uint8_t crc8(const uint8_t *data, int n)
337  {
338      uint8_t crc = 0;
339      int i, b;
340
341      for (i = 0; i < n; i++) {
342          crc ^= data[i];
343          for (b = 0; b < 8; b++)
344              crc = (crc & 1) ? (uint8_t)((crc >> 1) ^ 0x8C) : (uint8_t)(crc >> 1);
345      }
346      return crc;
347  }
348
349  static void ow_command(uint8_t cmd)
350  {
351      if (ow_state == OW_ROM) {
352          ow_state = cmd == 0xCC ? OW_FUNC : OW_IDLE;   // tylko SKIP ROM
353      } else if (cmd == 0x44) {
354          ow_converted = ow_raw;
355          ow_state = OW_IDLE;
356      } else if (cmd == 0xBE) {
357          uint8_t pad[9] = { (uint8_t)ow_converted, (uint8_t)((uint16_t)ow_converted >> 8),
358                             0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0 };
359          pad[8] = crc8(pad, 8);
360          memcpy(ow_tx, pad, sizeof(ow_tx));
361          ow_tx_bit = 0;
362          ow_state = OW_TX;
363      } else {
364          ow_state = OW_IDLE;
365      }
366  }
367
368  static void ow_violation(const char *what, double us)
369  {
370      fprintf(stderr, "avrsim: 1-Wire: %s (%.1f us)\n", what, us);
371      ow_state = OW_IDLE;
372  }
373
374  static void ow_master_edge(int low)
375  {
376      double us = (double)(avr->cycle - ow_fall) * 1e6 / freq;
377
378      if (low) {
379          ow_fall = avr->cycle;
380          if (ow_present && ow_state == OW_TX) {
381              if (!((ow_tx[ow_tx_bit / 8] >> (ow_tx_bit % 8)) & 1)) {
382                  ow_drive(1);
383                  ow_schedule(15, 0);
384              }
385              if (++ow_tx_bit == (int)sizeof(ow_tx) * 8)
386                  ow_state = OW_IDLE;
387          }
388          return;
389      }
390      if (!ow_present)
391          return;
392      if (us >= 480) {
393          ow_state = OW_ROM;
394          ow_bits = 0;
395          ow_schedule(60, 1);
396          ow_schedule(120, 0);
397      } else if (ow_state == OW_ROM || ow_state == OW_FUNC) {
398          if (us >= 15 && us < 60) {
399              ow_violation("bit zapisu poza dozwolonym czasem", us);
400              return;
401          }
402          ow_in = (uint8_t)((ow_in >> 1) | (us < 15 ? 0x80 : 0));
403          if (++ow_bits == 8) {
404              ow_bits = 0;
405              ow_command(ow_in);
406          }
407      } else if (us >= 15) {
408          ow_violation("szczelina odczytu dłuższa niż 15 us", us);
409      }
410  }
411
412  // Mikrokontroler ściąga linię, ustawiając pin jako wyjście (PORT = 0)
413  static void ddrd_hook(avr_irq_t *irq, uint32_t value, void *param)
414  {
415      int low = (value >> OW_BIT) & 1;
416      (void)irq;
417      (void)param;
418
419      if (low != ow_master_low) {
420          ow_master_low = low;
421          ow_master_edge(low);
422      }
423  }
424
425  static void set_temp(const char *arg, double c)
426  {
427      if (strcmp(arg, "none") == 0) {
428          ow_present = 0;
429          return;
430      }
431      if (c < -55)
432          c = -55;
433      if (c > 125)
434          c = 125;
435      ow_present = 1;
436      ow_raw = (int16_t)lround(c * 16);
437  }
```

- **Linia 293:** Funkcja ustawiająca napięcie na kanale.
- **Linia 294:** Początek funkcji.
- **Linia 295:** Jeśli napięcie przekracza napięcie odniesienia...
- **Linia 296:** ...przytnij je.
- **Linia 297:** Zapamiętaj wartość.
- **Linia 298:** Podaj ją na wirtualny przewód.
- **Linia 299:** Koniec funkcji.
- **Linia 301:** Funkcja przeliczająca odczyt ADC na miliwolty.
- **Linia 302:** Początek funkcji.
- **Linia 303:** Jeśli odczyt jest ujemny...
- **Linia 304:** ...ustaw 0.
- **Linia 305:** Jeśli przekracza 1023...
- **Linia 306:** ...ustaw 1023.
- **Linia 307:** Komentarz: dodanie 0,5 daje środek przedziału, czyli dokładnie żądany odczyt.
- **Linia 308:** Przelicz na miliwolty.
- **Linia 309:** Koniec funkcji.
⚠- **Linia 269:** Funkcja przeliczająca temperaturę na napięcie z czujnika.
- **Linia 316:** Początek funkcji.
⚠- **Linia 271:** Jeśli czujnikiem jest NTC...
⚠- **Linia 272:** Komentarz: termistor 10 kΩ od zasilania i 10 kΩ do masy.
⚠- **Linia 273:** Oblicz opór termistora wzorem z parametrem B (3950). 273,15 przelicza stopnie Celsjusza na kelwiny, a 298,15 K to 25 °C.
⚠- **Linia 274:** Oblicz napięcie z dzielnika napięcia.
- **Linia 345:** Koniec warunku.
⚠- **Linia 276:** Dla LM35: 10 mV na stopień. Dla temperatury ujemnej 0 mV.
- **Linia 437:** Koniec funkcji.

```c
439  static void apply_inputs(void)
440  {
441      int ch;
442
443      ow_state = OW_IDLE;
444      ow_master_low = 0;
445      ow_slave_low = 0;
446      drive_button(0);
447      for (ch = 0; ch < ADC_INPUTS; ch++)
448          avr_raise_irq(adc_irq[ch], adc_mv[ch]);
449  }
```

- **Linia 439:** Funkcja ustawiająca wejścia po starcie lub resecie.
- **Linia 440:** Początek funkcji.
- **Linia 441:** Licznik pętli.
- **Linia 446:** Przycisk puszczony.
⚠- **Linia 447:** Dla każdego kanału...
- **Linia 448:** ...podaj jego napięcie.
- **Linia 449:** Koniec funkcji.

### Reset

```c
451  static void do_reset(int bit, const char *reason)
452  {
453      avr_reset(avr);
454      // avr_reset() zeruje licznik cykli, więc czas rzeczywisty liczymy od nowa
455      t0 = now_ns();
456      avr->data[REG_MCUCSR] |= (uint8_t)(1 << bit);
457      apply_inputs();
458      emit("{\"ev\":\"reset\",\"t\":%.3f,\"reason\":\"%s\"}", sim_time(), reason);
459  }
```

- **Linia 451:** Funkcja resetująca układ. Dostaje bit przyczyny i nazwę.
- **Linia 452:** Początek funkcji.
- **Linia 453:** Zresetuj układ w emulatorze.
- **Linia 454:** Komentarz: reset zeruje licznik taktów, więc czas rzeczywisty liczymy od nowa.
- **Linia 455:** Zapamiętaj nowy czas startu.
- **Linia 456:** Ustaw w rejestrze bit przyczyny resetu. Firmware go odczyta.
- **Linia 457:** Podaj wejścia ponownie.
- **Linia 458:** Wypisz zdarzenie `reset`.
- **Linia 459:** Koniec funkcji.

### Pamięć EEPROM w pliku

Dzięki temu zawartość EEPROM przetrwa „zanik zasilania”, czyli zamknięcie programu.

```c
461  // --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------
462
463  static void eeprom_io(int save)
464  {
465      uint8_t buf[512];
466      avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
467      FILE *f;
468
469      if (!eeprom_path)
470          return;
471      if (save) {
472          if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
473              return;
474          fwrite(d.ee, 1, d.size, f);
475          fclose(f);
476      } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
477          d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
478          fclose(f);
479          if (d.size > 0)
480              avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
481      }
482  }
```

- **Linia 461:** Komentarz oddzielający tę część.
- **Linia 463:** Funkcja zapisująca lub wczytująca EEPROM. Parametr `save` wybiera działanie.
- **Linia 464:** Początek funkcji.
- **Linia 465:** Bufor o rozmiarze pamięci EEPROM układu.
- **Linia 466:** Opis obszaru pamięci dla emulatora.
- **Linia 467:** Wskaźnik na plik.
- **Linia 469:** Jeśli nie podano pliku...
- **Linia 470:** ...zakończ.
- **Linia 471:** Przy zapisie:
- **Linia 472:** Pobierz zawartość EEPROM i otwórz plik do zapisu. Jeśli się nie uda...
- **Linia 473:** ...zakończ.
- **Linia 474:** Zapisz dane do pliku.
- **Linia 475:** Zamknij plik.
- **Linia 476:** Przy odczycie, jeśli plik da się otworzyć:
- **Linia 477:** Wczytaj dane i zapisz ich rozmiar.
- **Linia 478:** Zamknij plik.
- **Linia 479:** Jeśli coś wczytano...
- **Linia 480:** ...przekaż to do emulatora.
- **Linia 481:** Koniec warunku.
- **Linia 482:** Koniec funkcji.

### Polecenia ze standardowego wejścia

```c
484  // --- sterowanie z stdin ---------------------------------------------------------
485
486  static void command(char *line)
487  {
488      char cmd[16] = "", arg[16] = "";
489      double v1 = 0, v2 = 0;
490      int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);
491
492      if (n <= 0)
493          return;
```

- **Linia 484:** Komentarz oddzielający tę część.
- **Linia 486:** Funkcja wykonująca jedno polecenie.
- **Linia 487:** Początek funkcji.
- **Linia 488:** Bufory na nazwę polecenia i jego argument.
- **Linia 489:** Zmienne na liczby.
- **Linia 490:** Odczytaj z linii słowo i dwie liczby. `n` to liczba odczytanych elementów.
- **Linia 492:** Jeśli niczego nie odczytano...
- **Linia 493:** ...zakończ.

```c
494      if (strcmp(cmd, "btn") == 0) {
495          sscanf(line, "%*s %15s %lf", arg, &v1);
496          if (strcmp(arg, "down") == 0) {
497              set_button(1);
498          } else if (strcmp(arg, "up") == 0) {
499              set_button(0);
500          } else {
501              double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
502              set_button(1);
503              avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
504                                       release_button, NULL);
505          }
```

- **Linia 494:** Polecenie `btn`:
- **Linia 495:** Odczytaj jeszcze raz: pomiń pierwsze słowo (`%*s`), weź drugie i liczbę.
- **Linia 496:** Dla `down`...
- **Linia 497:** ...naciśnij przycisk.
- **Linia 498:** Dla `up`...
- **Linia 499:** ...puść przycisk.
- **Linia 500:** W pozostałych przypadkach:
- **Linia 501:** Czas naciśnięcia: podany dla `press`, a w przeciwnym razie 100 ms.
- **Linia 502:** Naciśnij przycisk.
- **Linia 503:** Zaplanuj puszczenie po upływie tego czasu symulacji, przeliczonego na takty.
- **Linia 504:** Funkcja do wywołania i brak dodatkowych danych.
- **Linia 505:** Koniec warunku.

```c
357          uint8_t pad[9] = { (uint8_t)ow_converted, (uint8_t)((uint16_t)ow_converted >> 8),
358                             0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0 };
359          pad[8] = crc8(pad, 8);
360          memcpy(ow_tx, pad, sizeof(ow_tx));
361          ow_tx_bit = 0;
362          ow_state = OW_TX;
363      } else {
364          ow_state = OW_IDLE;
365      }
366  }
367
368  static void ow_violation(const char *what, double us)
369  {
370      fprintf(stderr, "avrsim: 1-Wire: %s (%.1f us)\n", what, us);
371      ow_state = OW_IDLE;
372  }
373
374  static void ow_master_edge(int low)
375  {
376      double us = (double)(avr->cycle - ow_fall) * 1e6 / freq;
377
378      if (low) {
379          ow_fall = avr->cycle;
380          if (ow_present && ow_state == OW_TX) {
381              if (!((ow_tx[ow_tx_bit / 8] >> (ow_tx_bit % 8)) & 1)) {
382                  ow_drive(1);
383                  ow_schedule(15, 0);
384              }
385              if (++ow_tx_bit == (int)sizeof(ow_tx) * 8)
386                  ow_state = OW_IDLE;
387          }
388          return;
389      }
390      if (!ow_present)
391          return;
392      if (us >= 480) {
393          ow_state = OW_ROM;
394          ow_bits = 0;
395          ow_schedule(60, 1);
396          ow_schedule(120, 0);
397      } else if (ow_state == OW_ROM || ow_state == OW_FUNC) {
398          if (us >= 15 && us < 60) {
399              ow_violation("bit zapisu poza dozwolonym czasem", us);
400              return;
401          }
402          ow_in = (uint8_t)((ow_in >> 1) | (us < 15 ? 0x80 : 0));
403          if (++ow_bits == 8) {
404              ow_bits = 0;
405              ow_command(ow_in);
406          }
407      } else if (us >= 15) {
408          ow_violation("szczelina odczytu dłuższa niż 15 us", us);
409      }
410  }
411
412  // Mikrokontroler ściąga linię, ustawiając pin jako wyjście (PORT = 0)
413  static void ddrd_hook(avr_irq_t *irq, uint32_t value, void *param)
414  {
415      int low = (value >> OW_BIT) & 1;
416      (void)irq;
417      (void)param;
418
419      if (low != ow_master_low) {
420          ow_master_low = low;
421          ow_master_edge(low);
422      }
423  }
424
425  static void set_temp(const char *arg, double c)
426  {
427      if (strcmp(arg, "none") == 0) {
428          ow_present = 0;
429          return;
430      }
431      if (c < -55)
432          c = -55;
433      if (c > 125)
434          c = 125;
435      ow_present = 1;
436      ow_raw = (int16_t)lround(c * 16);
437  }
438
439  static void apply_inputs(void)
440  {
441      int ch;
442
443      ow_state = OW_IDLE;
444      ow_master_low = 0;
445      ow_slave_low = 0;
446      drive_button(0);
447      for (ch = 0; ch < ADC_INPUTS; ch++)
448          avr_raise_irq(adc_irq[ch], adc_mv[ch]);
449  }
450
451  static void do_reset(int bit, const char *reason)
452  {
453      avr_reset(avr);
454      // avr_reset() zeruje licznik cykli, więc czas rzeczywisty liczymy od nowa
455      t0 = now_ns();
456      avr->data[REG_MCUCSR] |= (uint8_t)(1 << bit);
457      apply_inputs();
458      emit("{\"ev\":\"reset\",\"t\":%.3f,\"reason\":\"%s\"}", sim_time(), reason);
459  }
460
461  // --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------
462
463  static void eeprom_io(int save)
464  {
465      uint8_t buf[512];
466      avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
467      FILE *f;
468
469      if (!eeprom_path)
470          return;
471      if (save) {
472          if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
473              return;
474          fwrite(d.ee, 1, d.size, f);
475          fclose(f);
476      } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
477          d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
478          fclose(f);
479          if (d.size > 0)
480              avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
481      }
482  }
483
484  // --- sterowanie z stdin ---------------------------------------------------------
485
486  static void command(char *line)
487  {
488      char cmd[16] = "", arg[16] = "";
489      double v1 = 0, v2 = 0;
490      int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);
491
492      if (n <= 0)
493          return;
494      if (strcmp(cmd, "btn") == 0) {
495          sscanf(line, "%*s %15s %lf", arg, &v1);
496          if (strcmp(arg, "down") == 0) {
497              set_button(1);
498          } else if (strcmp(arg, "up") == 0) {
499              set_button(0);
500          } else {
501              double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
502              set_button(1);
503              avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
504                                       release_button, NULL);
505          }
506      } else if (strcmp(cmd, "pot") == 0 && n >= 2) {
507          set_adc(0, raw_to_mv((long)v1));
508      } else if (strcmp(cmd, "ldr") == 0 && n >= 2) {
509          set_adc(1, raw_to_mv((long)v1));
510      } else if (strcmp(cmd, "temp") == 0 && sscanf(line, "%*s %15s", arg) == 1) {
511          set_temp(arg, v1);
512      } else if (strcmp(cmd, "mv") == 0 && n >= 3 && v1 >= 0 && v1 < ADC_INPUTS) {
513          set_adc((int)v1, (uint32_t)v2);
514      } else if (strcmp(cmd, "reset") == 0) {
515          do_reset(BIT_EXTRF, "EXT");
516      } else if (strcmp(cmd, "power") == 0) {
517          eeprom_io(1);
518          ow_converted = 85 * 16;
519          do_reset(BIT_PORF, "POR");
520      } else if (strcmp(cmd, "state") == 0) {
521          emit("{\"ev\":\"state\",\"t\":%.3f,\"duty\":%.4f,\"pot_mv\":%u,\"ldr_mv\":%u,"
522               "\"temp_c\":%.4f,\"ds18b20\":%d}",
523               sim_time(), read_led_duty(), adc_mv[0], adc_mv[1], ow_raw / 16.0, ow_present);
524      } else if (strcmp(cmd, "quit") == 0) {
525          stop_requested = 1;
526      } else {
527          fprintf(stderr, "avrsim: nieznana komenda: %s\n", line);
528      }
529  }
```

- **Linia 506:** Polecenie `pot` z liczbą:
- **Linia 507:** Ustaw napięcie na kanale 0.
- **Linia 508:** Polecenie `ldr` z liczbą:
- **Linia 509:** Ustaw napięcie na kanale 1.
⚠- **Linia 510:** Polecenie `temp` z liczbą:
⚠- **Linia 511:** Ustaw napięcie na kanale 2.
⚠- **Linia 512:** Polecenie `mv` z numerem kanału i napięciem:
- **Linia 513:** Ustaw napięcie na wskazanym kanale.
- **Linia 514:** Polecenie `reset`:
- **Linia 515:** Wykonaj reset zewnętrzny.
- **Linia 516:** Polecenie `power`:
- **Linia 517:** Zapisz EEPROM do pliku.
- **Linia 519:** Wykonaj reset jak po włączeniu zasilania.
- **Linia 520:** Polecenie `state`:
⚠- **Linia 357:** Wypisz zdarzenie ze stanem...
⚠- **Linia 358:** ...podając czas, wypełnienie i napięcia.
- **Linia 524:** Polecenie `quit`:
- **Linia 525:** Ustaw znacznik zakończenia.
- **Linia 526:** Nieznane polecenie:
- **Linia 527:** Wypisz błąd.
- **Linia 528:** Koniec łańcucha warunków.
- **Linia 529:** Koniec funkcji.

```c
531  static void read_stdin(void)
532  {
533      static char buf[256];
534      static size_t len;
535      ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
536      char *start, *nl;
537
538      if (n == 0) {
539          stop_requested = 1;
540          return;
541      }
542      if (n < 0)
543          return;
544      len += (size_t)n;
545      buf[len] = '\0';
546      start = buf;
547      while ((nl = strchr(start, '\n')) != NULL) {
548          *nl = '\0';
549          command(start);
550          start = nl + 1;
551      }
552      len = strlen(start);
553      memmove(buf, start, len);
554      if (len == sizeof(buf) - 1)
555          len = 0;
556  }
```

- **Linia 531:** Funkcja czytająca ze standardowego wejścia i dzieląca na linie.
- **Linia 532:** Początek funkcji.
- **Linia 533:** Bufor. `static` wewnątrz funkcji: zachowuje zawartość między wywołaniami.
- **Linia 534:** Liczba znaków w buforze. Też zachowuje wartość.
- **Linia 535:** Dopisz do bufora to, co jest na wejściu.
- **Linia 536:** Wskaźniki: początek linii i koniec linii.
- **Linia 538:** Jeśli odczytano 0 bajtów, wejście zostało zamknięte...
- **Linia 539:** ...ustaw znacznik zakończenia...
- **Linia 540:** ...i zakończ funkcję.
- **Linia 541:** Koniec warunku.
- **Linia 542:** Jeśli wystąpił błąd lub nie ma danych...
- **Linia 543:** ...zakończ funkcję.
- **Linia 544:** Powiększ licznik.
- **Linia 545:** Zakończ napis.
- **Linia 546:** Zacznij od początku bufora.
- **Linia 547:** Dopóki jest znak końca linii...
- **Linia 548:** ...zamień go na koniec napisu...
- **Linia 549:** ...wykonaj polecenie...
- **Linia 550:** ...i przejdź za niego.
- **Linia 551:** Koniec pętli.
- **Linia 552:** Zapisz długość reszty, czyli niepełnej linii.
- **Linia 553:** Przesuń resztę na początek bufora.
- **Linia 554:** Jeśli bufor jest pełny, a końca linii wciąż nie ma...
- **Linia 555:** ...porzuć tę linię.
- **Linia 556:** Koniec funkcji.

```c
558  static void on_signal(int sig)
559  {
560      (void)sig;
561      stop_requested = 1;
562  }
563
564  static void usage(const char *prog)
565  {
566      fprintf(stderr, "użycie: %s [--speed X] [--freq HZ] [--link ŚCIEŻKA] [--eeprom PLIK] "
567                      "firmware.hex\n", prog);
568      exit(2);
569  }
```

- **Linia 558:** Funkcja obsługi sygnału, np. Ctrl+C.
- **Linia 559:** Początek funkcji.
- **Linia 560:** Parametr nieużywany.
- **Linia 561:** Ustaw znacznik zakończenia.
- **Linia 562:** Koniec funkcji.
- **Linia 564:** Funkcja wypisująca instrukcję użycia.
- **Linia 565:** Początek funkcji.
⚠- **Linia 566:** Wypisz pierwszą część...
⚠- **Linia 567:** ...i drugą część.
- **Linia 568:** Zakończ program z kodem błędu 2.
- **Linia 569:** Koniec funkcji.

### Funkcja główna: start

```c
419      if (low != ow_master_low) {
420          ow_master_low = low;
421          ow_master_edge(low);
422      }
423  }
424
425  static void set_temp(const char *arg, double c)
426  {
427      if (strcmp(arg, "none") == 0) {
428          ow_present = 0;
429          return;
430      }
431      if (c < -55)
432          c = -55;
433      if (c > 125)
434          c = 125;
435      ow_present = 1;
436      ow_raw = (int16_t)lround(c * 16);
437  }
438
439  static void apply_inputs(void)
440  {
441      int ch;
442
443      ow_state = OW_IDLE;
444      ow_master_low = 0;
445      ow_slave_low = 0;
446      drive_button(0);
447      for (ch = 0; ch < ADC_INPUTS; ch++)
448          avr_raise_irq(adc_irq[ch], adc_mv[ch]);
449  }
450
451  static void do_reset(int bit, const char *reason)
452  {
453      avr_reset(avr);
454      // avr_reset() zeruje licznik cykli, więc czas rzeczywisty liczymy od nowa
455      t0 = now_ns();
456      avr->data[REG_MCUCSR] |= (uint8_t)(1 << bit);
457      apply_inputs();
458      emit("{\"ev\":\"reset\",\"t\":%.3f,\"reason\":\"%s\"}", sim_time(), reason);
459  }
460
461  // --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------
462
463  static void eeprom_io(int save)
464  {
465      uint8_t buf[512];
466      avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
467      FILE *f;
468
469      if (!eeprom_path)
470          return;
471      if (save) {
472          if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
473              return;
474          fwrite(d.ee, 1, d.size, f);
475          fclose(f);
476      } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
477          d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
478          fclose(f);
479          if (d.size > 0)
480              avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
481      }
482  }
483
484  // --- sterowanie z stdin ---------------------------------------------------------
485
486  static void command(char *line)
487  {
488      char cmd[16] = "", arg[16] = "";
489      double v1 = 0, v2 = 0;
490      int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);
491
492      if (n <= 0)
493          return;
494      if (strcmp(cmd, "btn") == 0) {
495          sscanf(line, "%*s %15s %lf", arg, &v1);
496          if (strcmp(arg, "down") == 0) {
497              set_button(1);
498          } else if (strcmp(arg, "up") == 0) {
499              set_button(0);
500          } else {
501              double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
502              set_button(1);
503              avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
504                                       release_button, NULL);
505          }
506      } else if (strcmp(cmd, "pot") == 0 && n >= 2) {
507          set_adc(0, raw_to_mv((long)v1));
508      } else if (strcmp(cmd, "ldr") == 0 && n >= 2) {
509          set_adc(1, raw_to_mv((long)v1));
510      } else if (strcmp(cmd, "temp") == 0 && sscanf(line, "%*s %15s", arg) == 1) {
511          set_temp(arg, v1);
512      } else if (strcmp(cmd, "mv") == 0 && n >= 3 && v1 >= 0 && v1 < ADC_INPUTS) {
513          set_adc((int)v1, (uint32_t)v2);
514      } else if (strcmp(cmd, "reset") == 0) {
515          do_reset(BIT_EXTRF, "EXT");
516      } else if (strcmp(cmd, "power") == 0) {
517          eeprom_io(1);
518          ow_converted = 85 * 16;
519          do_reset(BIT_PORF, "POR");
520      } else if (strcmp(cmd, "state") == 0) {
521          emit("{\"ev\":\"state\",\"t\":%.3f,\"duty\":%.4f,\"pot_mv\":%u,\"ldr_mv\":%u,"
522               "\"temp_c\":%.4f,\"ds18b20\":%d}",
523               sim_time(), read_led_duty(), adc_mv[0], adc_mv[1], ow_raw / 16.0, ow_present);
524      } else if (strcmp(cmd, "quit") == 0) {
525          stop_requested = 1;
526      } else {
527          fprintf(stderr, "avrsim: nieznana komenda: %s\n", line);
528      }
529  }
530
531  static void read_stdin(void)
532  {
533      static char buf[256];
534      static size_t len;
535      ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
536      char *start, *nl;
537
538      if (n == 0) {
539          stop_requested = 1;
540          return;
541      }
542      if (n < 0)
543          return;
544      len += (size_t)n;
545      buf[len] = '\0';
546      start = buf;
547      while ((nl = strchr(start, '\n')) != NULL) {
548          *nl = '\0';
549          command(start);
550          start = nl + 1;
551      }
552      len = strlen(start);
553      memmove(buf, start, len);
554      if (len == sizeof(buf) - 1)
555          len = 0;
556  }
557
558  static void on_signal(int sig)
559  {
560      (void)sig;
561      stop_requested = 1;
562  }
563
564  static void usage(const char *prog)
565  {
566      fprintf(stderr, "użycie: %s [--speed X] [--freq HZ] [--link ŚCIEŻKA] [--eeprom PLIK] "
567                      "firmware.hex\n", prog);
568      exit(2);
569  }
570
571  int main(int argc, char *argv[])
572  {
573      elf_firmware_t fw;
574      const char *hex = NULL;
575      int i, status = 0;
576
577      for (i = 1; i < argc; i++) {
578          if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
579              speed = atof(argv[++i]);
580          else if (strcmp(argv[i], "--freq") == 0 && i + 1 < argc)
581              freq = (uint32_t)atol(argv[++i]);
582          else if (strcmp(argv[i], "--link") == 0 && i + 1 < argc)
583              link_path = argv[++i];
584          else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc)
585              eeprom_path = argv[++i];
586          else if (argv[i][0] != '-' && !hex)
587              hex = argv[i];
588          else
589              usage(argv[0]);
590      }
591      if (!hex)
592          usage(argv[0]);
```

- **Linia 571:** Początek programu. `argc` to liczba argumentów, a `argv` to ich lista.
- **Linia 572:** Początek funkcji.
- **Linia 573:** Opis firmware.
- **Linia 574:** Ścieżka pliku `.hex`, na razie brak.
- **Linia 575:** Licznik i kod zakończenia.
- **Linia 577:** Przejdź po argumentach, zaczynając od pierwszego prawdziwego.
- **Linia 578:** Jeśli to `--speed` i jest następny argument...
- **Linia 579:** ...odczytaj szybkość i przesuń się dalej.
- **Linia 582:** Jeśli to `--link`...
- **Linia 583:** ...zapisz ścieżkę dowiązania.
- **Linia 584:** Jeśli to `--eeprom`...
- **Linia 585:** ...zapisz ścieżkę pliku.
⚠- **Linia 419:** Jeśli to `--sensor`...
⚠- **Linia 420:** ...zapisz, czy to NTC.
- **Linia 586:** Jeśli argument nie zaczyna się od minusa i nie ma jeszcze pliku...
- **Linia 587:** ...to jest to plik firmware.
- **Linia 588:** W pozostałych przypadkach:
- **Linia 589:** Wypisz instrukcję i zakończ program.
- **Linia 590:** Koniec pętli.
- **Linia 591:** Jeśli nie podano pliku...
- **Linia 592:** ...wypisz instrukcję i zakończ program.

```c
594      avr_global_logger_set(logger);
595      memset(&fw, 0, sizeof(fw));
596      strcpy(fw.mmcu, "atmega32");
597      fw.frequency = freq;
598      sim_setup_firmware(hex, 0, &fw, argv[0]);
599
600      avr = avr_make_mcu_by_name(fw.mmcu);
601      if (!avr) {
602          fprintf(stderr, "avrsim: brak rdzenia atmega32 w simavr\n");
603          return 1;
604      }
605      avr_init(avr);
606      avr_load_firmware(avr, &fw);
607      avr->vcc = avr->avcc = avr->aref = VREF_MV;
608      avr->sleep = sleep_noop;
609      avr->data[REG_MCUCSR] |= 1 << BIT_PORF;
610      eeprom_io(0);
```

- **Linia 594:** Ustaw naszą funkcję zapisywania komunikatów.
- **Linia 595:** Wyzeruj opis firmware.
⚠- **Linia 596:** Wpisz nazwę układu.
⚠- **Linia 597:** Wpisz częstotliwość.
- **Linia 598:** Wczytaj plik firmware.
- **Linia 600:** Utwórz układ o podanej nazwie.
- **Linia 601:** Jeśli się nie udało...
⚠- **Linia 602:** ...wypisz błąd...
- **Linia 603:** ...i zakończ z kodem 1.
- **Linia 604:** Koniec warunku.
- **Linia 605:** Przygotuj układ.
- **Linia 606:** Załaduj do niego firmware.
- **Linia 607:** Ustaw napięcia zasilania i odniesienia.
- **Linia 608:** Zastąp usypianie pustą funkcją.
- **Linia 609:** Ustaw w rejestrze przyczynę resetu: włączenie zasilania.
- **Linia 610:** Wczytaj EEPROM z pliku.

### Podłączanie przewodów

```c
460
461  // --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------
462
463  static void eeprom_io(int save)
464  {
465      uint8_t buf[512];
466      avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
467      FILE *f;
468
469      if (!eeprom_path)
470          return;
471      if (save) {
472          if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
473              return;
474          fwrite(d.ee, 1, d.size, f);
475          fclose(f);
476      } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
477          d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
478          fclose(f);
479          if (d.size > 0)
480              avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
481      }
482  }
483
484  // --- sterowanie z stdin ---------------------------------------------------------
485
486  static void command(char *line)
487  {
488      char cmd[16] = "", arg[16] = "";
489      double v1 = 0, v2 = 0;
490      int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);
491
492      if (n <= 0)
493          return;
494      if (strcmp(cmd, "btn") == 0) {
495          sscanf(line, "%*s %15s %lf", arg, &v1);
496          if (strcmp(arg, "down") == 0) {
497              set_button(1);
498          } else if (strcmp(arg, "up") == 0) {
499              set_button(0);
500          } else {
501              double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
502              set_button(1);
503              avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
504                                       release_button, NULL);
505          }
506      } else if (strcmp(cmd, "pot") == 0 && n >= 2) {
507          set_adc(0, raw_to_mv((long)v1));
508      } else if (strcmp(cmd, "ldr") == 0 && n >= 2) {
509          set_adc(1, raw_to_mv((long)v1));
510      } else if (strcmp(cmd, "temp") == 0 && sscanf(line, "%*s %15s", arg) == 1) {
511          set_temp(arg, v1);
512      } else if (strcmp(cmd, "mv") == 0 && n >= 3 && v1 >= 0 && v1 < ADC_INPUTS) {
513          set_adc((int)v1, (uint32_t)v2);
514      } else if (strcmp(cmd, "reset") == 0) {
515          do_reset(BIT_EXTRF, "EXT");
516      } else if (strcmp(cmd, "power") == 0) {
517          eeprom_io(1);
518          ow_converted = 85 * 16;
519          do_reset(BIT_PORF, "POR");
520      } else if (strcmp(cmd, "state") == 0) {
521          emit("{\"ev\":\"state\",\"t\":%.3f,\"duty\":%.4f,\"pot_mv\":%u,\"ldr_mv\":%u,"
522               "\"temp_c\":%.4f,\"ds18b20\":%d}",
523               sim_time(), read_led_duty(), adc_mv[0], adc_mv[1], ow_raw / 16.0, ow_present);
524      } else if (strcmp(cmd, "quit") == 0) {
525          stop_requested = 1;
526      } else {
527          fprintf(stderr, "avrsim: nieznana komenda: %s\n", line);
528      }
529  }
530
531  static void read_stdin(void)
532  {
533      static char buf[256];
534      static size_t len;
535      ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
536      char *start, *nl;
537
538      if (n == 0) {
539          stop_requested = 1;
540          return;
541      }
542      if (n < 0)
543          return;
544      len += (size_t)n;
545      buf[len] = '\0';
546      start = buf;
547      while ((nl = strchr(start, '\n')) != NULL) {
548          *nl = '\0';
549          command(start);
550          start = nl + 1;
551      }
552      len = strlen(start);
553      memmove(buf, start, len);
554      if (len == sizeof(buf) - 1)
555          len = 0;
556  }
557
558  static void on_signal(int sig)
559  {
560      (void)sig;
561      stop_requested = 1;
562  }
563
564  static void usage(const char *prog)
565  {
566      fprintf(stderr, "użycie: %s [--speed X] [--freq HZ] [--link ŚCIEŻKA] [--eeprom PLIK] "
567                      "firmware.hex\n", prog);
568      exit(2);
569  }
570
571  int main(int argc, char *argv[])
572  {
573      elf_firmware_t fw;
574      const char *hex = NULL;
575      int i, status = 0;
576
577      for (i = 1; i < argc; i++) {
578          if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
579              speed = atof(argv[++i]);
580          else if (strcmp(argv[i], "--freq") == 0 && i + 1 < argc)
581              freq = (uint32_t)atol(argv[++i]);
582          else if (strcmp(argv[i], "--link") == 0 && i + 1 < argc)
583              link_path = argv[++i];
584          else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc)
585              eeprom_path = argv[++i];
586          else if (argv[i][0] != '-' && !hex)
587              hex = argv[i];
588          else
589              usage(argv[0]);
590      }
591      if (!hex)
592          usage(argv[0]);
593
594      avr_global_logger_set(logger);
595      memset(&fw, 0, sizeof(fw));
596      strcpy(fw.mmcu, "atmega32");
597      fw.frequency = freq;
598      sim_setup_firmware(hex, 0, &fw, argv[0]);
599
600      avr = avr_make_mcu_by_name(fw.mmcu);
601      if (!avr) {
602          fprintf(stderr, "avrsim: brak rdzenia atmega32 w simavr\n");
603          return 1;
604      }
605      avr_init(avr);
606      avr_load_firmware(avr, &fw);
607      avr->vcc = avr->avcc = avr->aref = VREF_MV;
608      avr->sleep = sleep_noop;
609      avr->data[REG_MCUCSR] |= 1 << BIT_PORF;
610      eeprom_io(0);
611
612      {
613          uint32_t flags = 0;
614          avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
615          flags &= ~(uint32_t)AVR_UART_FLAG_STDIO;
616          avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
617      }
618      uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
619      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
620                              uart_out_hook, NULL);
621      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XON),
622                              uart_xon_hook, NULL);
623      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XOFF),
624                              uart_xoff_hook, NULL);
625      button_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN0 + BTN_BIT);
626      ow_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN0 + OW_BIT);
627      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_DIRECTION_ALL),
628                              ddrd_hook, NULL);
629      avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'),
630                                            IOPORT_IRQ_PIN0 + STATUS_BIT),
631                              status_hook, NULL);
632      for (i = 0; i < ADC_INPUTS; i++)
633          adc_irq[i] = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0 + i);
634      apply_inputs();
```

- **Linia 612:** Początek wewnętrznego bloku, by zmienna `flags` istniała tylko tutaj.
- **Linia 613:** Zmienna na flagi UART.
- **Linia 614:** Odczytaj flagi.
- **Linia 615:** Wyłącz flagę, która kazałaby simavr wypisywać znaki na ekran. Sami się tym zajmujemy.
- **Linia 616:** Zapisz flagi.
- **Linia 617:** Koniec bloku.
- **Linia 618:** Pobierz wirtualny przewód wejścia UART.
- **Linia 619:** Podłącz naszą funkcję do wyjścia UART...
- **Linia 620:** ...czyli funkcję `uart_out_hook`.
- **Linia 621:** Podłącz naszą funkcję do sygnału gotowości...
- **Linia 622:** ...czyli funkcję `uart_xon_hook`.
- **Linia 623:** Podłącz naszą funkcję do sygnału wstrzymania...
- **Linia 624:** ...czyli funkcję `uart_xoff_hook`.
⚠- **Linia 460:** Pobierz wirtualny przewód pinu przycisku, czyli pinu 2 portu D.
⚠- **Linia 461:** Podłącz naszą funkcję do pinu 7 portu D, czyli diody statusu...
- **Linia 631:** ...czyli funkcję `status_hook`.
⚠- **Linia 632:** Dla trzech kanałów...
- **Linia 633:** ...pobierz wirtualny przewód ADC.
⚠- **Linia 465:** Ustaw początkową temperaturę: 22 °C.
- **Linia 634:** Podaj wejścia.

```c
636      signal(SIGINT, on_signal);
637      signal(SIGTERM, on_signal);
638      signal(SIGPIPE, SIG_IGN);
639      fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
640      open_pty();
641      report_led(1);
642
643      t0 = now_ns();
```

- **Linia 636:** Przy sygnale Ctrl+C wywołaj `on_signal`.
- **Linia 637:** Tak samo przy prośbie o zakończenie.
- **Linia 638:** Ignoruj błąd pisania do zamkniętego wyjścia, by program nie kończył się nagle.
- **Linia 639:** Ustaw odczyt wejścia bez czekania.
- **Linia 640:** Utwórz pseudoterminal.
- **Linia 641:** Zgłoś początkowy stan diody.
- **Linia 643:** Zapisz czas startu.

### Pętla główna

```c
644      while (!stop_requested) {
645          struct pollfd fds[2] = {
646              { .fd = STDIN_FILENO, .events = POLLIN },
647              { .fd = pty_master, .events = POLLIN },
648          };
649          avr_cycle_count_t target = speed > 0
650              ? (avr_cycle_count_t)((double)(now_ns() - t0) * 1e-9 * speed * freq)
651              : avr->cycle + 50000;
652          avr_cycle_count_t budget = avr->cycle + 100000;
```

- **Linia 644:** Dopóki nie ma polecenia końca...
- **Linia 645:** Lista źródeł, na które czekamy:
- **Linia 646:** Standardowe wejście.
- **Linia 647:** Pseudoterminal.
- **Linia 648:** Koniec listy.
- **Linia 649:** Oblicz, do którego taktu ma dojść symulacja. Jeśli szybkość jest dodatnia...
⚠- **Linia 650:** ...to czas rzeczywisty razy szybkość razy częstotliwość...
- **Linia 651:** ...a w przeciwnym razie, czyli przy trybie bez limitu, o 50 000 taktów naprzód.
- **Linia 652:** Wykonaj najwyżej 100 000 taktów, by program mógł obsłużyć też inne rzeczy.

```c
654          while (avr->cycle < target && avr->cycle < budget) {
655              int state = avr_run(avr);
656              if (state == cpu_Done || state == cpu_Crashed) {
657                  emit("{\"ev\":\"halt\",\"t\":%.3f,\"state\":%d}", sim_time(), state);
658                  stop_requested = 1;
659                  status = 1;
660                  break;
661              }
662          }
```

- **Linia 654:** Dopóki nie doszliśmy do celu ani do limitu...
- **Linia 655:** ...wykonaj kolejny krok układu.
- **Linia 656:** Jeśli układ się zatrzymał lub wystąpił błąd...
- **Linia 657:** ...wypisz zdarzenie...
- **Linia 658:** ...ustaw znacznik zakończenia...
- **Linia 659:** ...ustaw kod błędu...
- **Linia 660:** ...i przerwij pętlę.
- **Linia 661:** Koniec warunku.
- **Linia 662:** Koniec pętli.

```c
663          feed_uart();
664          report_led(0);
665          if (poll(fds, 2, avr->cycle >= target ? 1 : 0) > 0) {
666              if (fds[0].revents & (POLLIN | POLLHUP))
667                  read_stdin();
668              if (fds[1].revents & POLLIN)
669                  read_pty();
670          }
671      }
```

- **Linia 663:** Podaj oczekujące znaki do układu.
- **Linia 664:** Zgłoś stan diody, jeśli się zmienił.
- **Linia 665:** Czekaj na dane. Jeśli symulacja doszła do celu, najwyżej 1 ms, a w przeciwnym razie wcale.
- **Linia 666:** Jeśli coś jest na wejściu lub je zamknięto...
- **Linia 667:** ...przeczytaj to.
- **Linia 668:** Jeśli coś jest na pseudoterminalu...
- **Linia 669:** ...przeczytaj to.
- **Linia 670:** Koniec warunku.
- **Linia 671:** Koniec pętli głównej.

### Zakończenie

```c
673      eeprom_io(1);
674      if (link_path)
675          unlink(link_path);
676      avr_terminate(avr);
677      return status;
678  }
```

- **Linia 673:** Zapisz EEPROM do pliku.
- **Linia 674:** Jeśli było dowiązanie...
- **Linia 675:** ...usuń je.
- **Linia 676:** Zakończ pracę emulatora.
- **Linia 677:** Zwróć kod zakończenia.
- **Linia 678:** Koniec programu.

---

[← Poprzedni](60-sim-makefile.md) · [Spis treści](README.md) · [Następny →](70-narzedzia-github-tymczasowo.md)
