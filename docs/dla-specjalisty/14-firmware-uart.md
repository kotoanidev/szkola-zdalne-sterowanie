# Firmware: `uart.c`, rozmowa z serwerem

[← Poprzedni](13-firmware-timer.md) · [Spis treści](README.md) · [Następny →](15-firmware-protocol.md)

**Po co ten plik:** UART przesyła znaki powoli: jeden znak trwa około 1 ms, czyli przy zegarze 1 MHz około 1000 taktów. Program nie może tak długo stać i czekać. Dlatego znaki odkłada do **buforów**, a przerwania przenoszą je z przewodu i na przewód w tle.

**Bufor kołowy** to tablica, do której pisze się w kółko, jak w karuzeli. Mamy dwa wskaźniki:

- `head` (głowa): tu dopiszemy następny znak,
- `tail` (ogon): stąd odczytamy następny znak.

Jeśli `head == tail`, bufor jest pusty. Jeśli głowa stoi tuż przed ogonem, czyli po dopisaniu znaku by go dogoniła, bufor jest pełny. Po dojściu do końca tablicy wskaźnik wraca na początek.

## `firmware/src/uart.c`

[Otwórz ten plik w repozytorium](../../firmware/src/uart.c)

### Stałe i bufory

```c
  1  // UART 9600 bps 8N1 (U2X) na przerwaniach, z buforami kołowymi
  2  #include <avr/io.h>
  3  #include <avr/interrupt.h>
  4  #include "uart.h"
  5
  6  // Rozmiary muszą być potęgami dwójki
  7  #define RX_SIZE 32
  8  #define TX_SIZE 128
  9  #define RX_MASK (RX_SIZE - 1)
 10  #define TX_MASK (TX_SIZE - 1)
 11
 12  #define UBRR_VALUE ((F_CPU + 4 * UART_BAUD) / (8 * UART_BAUD) - 1)
 13
 14  static volatile char rx_buf[RX_SIZE];
 15  static volatile uint8_t rx_head, rx_tail;
 16  static volatile char tx_buf[TX_SIZE];
 17  static volatile uint8_t tx_head, tx_tail;
```

- **Linia 1:** Komentarz: UART z prędkością 9600 bitów na sekundę i formatem 8N1, w trybie U2X (podwójna prędkość), na przerwaniach, z buforami kołowymi.
- **Linia 2:** Biblioteka z nazwami rejestrów.
- **Linia 3:** Biblioteka przerwań.
- **Linia 4:** Nasz nagłówek. Dostarcza m.in. stałą `UART_BAUD`.
- **Linia 6:** Komentarz: rozmiary buforów muszą być potęgami dwójki, bo tylko wtedy działa szybkie „zawijanie” maską.
- **Linia 7:** Bufor odbiorczy ma 32 bajty.
- **Linia 8:** Bufor nadawczy ma 128 bajtów. Jest większy, bo odpowiedź `STATE` jest długa.
- **Linia 9:** Maska odbioru: 32 - 1 = 31, czyli `00011111`. Operacja `& RX_MASK` zawija liczbę do zakresu 0–31.
- **Linia 10:** Maska nadawania: 127, czyli `01111111`.
- **Linia 12:** Wartość dla rejestru prędkości. W trybie U2X wzór to `F_CPU / (8 * prędkość) - 1`. Dodanie `4 * UART_BAUD` zaokrągla wynik zamiast obcinać. Wychodzi 12, co daje rzeczywistą prędkość 9615 bitów na sekundę.
- **Linia 14:** Tablica odbiorcza. `volatile`, bo zapisuje ją przerwanie.
- **Linia 15:** Głowa i ogon bufora odbiorczego.
- **Linia 16:** Tablica nadawcza.
- **Linia 17:** Głowa i ogon bufora nadawczego.

### Przerwanie: przyszedł znak

```c
 19  ISR(USART_RXC_vect)
 20  {
 21      char c = UDR;
 22      uint8_t next = (rx_head + 1) & RX_MASK;
 23
 24      // Przy przepełnieniu znak przepada, komenda dostanie ERR
 25      if (next != rx_tail) {
 26          rx_buf[rx_head] = c;
 27          rx_head = next;
 28      }
 29  }
```

- **Linia 19:** Funkcja obsługi przerwania „odebrano znak”. Sprzęt uruchamia ją sam.
- **Linia 20:** Początek funkcji.
- **Linia 21:** Odczytaj znak z rejestru danych `UDR`. To zarazem kasuje zgłoszenie przerwania.
- **Linia 22:** Oblicz następną pozycję głowy. `& RX_MASK` sprawia, że po 31 następuje 0.
- **Linia 24:** Komentarz: gdy bufor jest pełny, znak przepada, a uszkodzona komenda dostanie odpowiedź `ERR`.
- **Linia 25:** Jeśli po dopisaniu głowa nie dogoni ogona, czyli jest miejsce...
- **Linia 26:** ...zapisz znak w tablicy...
- **Linia 27:** ...i przesuń głowę.
- **Linia 28:** Koniec warunku.
- **Linia 29:** Koniec funkcji.

### Przerwanie: można wysłać znak

```c
 31  ISR(USART_UDRE_vect)
 32  {
 33      if (tx_head != tx_tail) {
 34          UDR = tx_buf[tx_tail];
 35          tx_tail = (tx_tail + 1) & TX_MASK;
 36      } else {
 37          UCSRB &= ~_BV(UDRIE);
 38      }
 39  }
```

- **Linia 31:** Funkcja obsługi przerwania „rejestr danych jest pusty”, czyli sprzęt jest gotowy na kolejny znak.
- **Linia 32:** Początek funkcji.
- **Linia 33:** Jeśli w buforze nadawczym coś czeka...
- **Linia 34:** ...wpisz znak z ogona do `UDR`. Sprzęt sam wyśle go przewodem.
- **Linia 35:** Przesuń ogon, zawijając go maską.
- **Linia 36:** W przeciwnym razie, czyli bufor jest pusty:
- **Linia 37:** Wyłącz to przerwanie, kasując bit `UDRIE`. Inaczej wywoływałoby się bez przerwy, bo rejestr ciągle jest pusty.
- **Linia 38:** Koniec warunku.
- **Linia 39:** Koniec funkcji.

### Przygotowanie UART

```c
 41  void uart_init(void)
 42  {
 43      UBRRH = (uint8_t)(UBRR_VALUE >> 8);
 44      UBRRL = (uint8_t)UBRR_VALUE;
 45      UCSRA = _BV(U2X);
 46      UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
 47      UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
 48  }
```

- **Linia 41:** Początek funkcji `uart_init`.
- **Linia 42:** Początek treści funkcji.
- **Linia 43:** Wpisz górny bajt wartości prędkości do `UBRRH`. `>> 8` przesuwa liczbę o 8 bitów w prawo.
- **Linia 44:** Wpisz dolny bajt wartości prędkości do `UBRRL`. Zapisanie dolnego bajtu uaktywnia nową prędkość.
- **Linia 45:** Włącz bit `U2X`, czyli tryb podwójnej prędkości. Przy zegarze 1 MHz tylko on daje 9600 bitów na sekundę z małym błędem.
- **Linia 46:** Ustaw format. Bit `URSEL` wskazuje, że piszemy do `UCSRC` (ten rejestr dzieli adres z `UBRRH`). `UCSZ1` i `UCSZ0` wybierają 8 bitów danych. Reszta domyślnie: bez parzystości, 1 bit stopu.
- **Linia 47:** Włącz odbiornik (`RXEN`), nadajnik (`TXEN`) i przerwanie po odebraniu znaku (`RXCIE`).
- **Linia 48:** Koniec funkcji.

### Odczyt znaku z bufora

```c
 50  uint8_t uart_read(char *c)
 51  {
 52      if (rx_head == rx_tail)
 53          return 0;
 54      *c = rx_buf[rx_tail];
 55      rx_tail = (rx_tail + 1) & RX_MASK;
 56      return 1;
 57  }
```

- **Linia 50:** Funkcja dostaje adres zmiennej, do której ma wpisać znak. Zwraca 1, jeśli znak był, a 0, jeśli nie.
- **Linia 51:** Początek funkcji.
- **Linia 52:** Jeśli głowa i ogon są równe, bufor jest pusty...
- **Linia 53:** ...zwróć 0.
- **Linia 54:** Wpisz znak z ogona pod podany adres. `*c` oznacza „miejsce, na które wskazuje `c`”.
- **Linia 55:** Przesuń ogon, zawijając go maską.
- **Linia 56:** Zwróć 1: znak został odczytany.
- **Linia 57:** Koniec funkcji.

### Wysłanie jednego znaku

```c
 59  void uart_putc(char c)
 60  {
 61      uint8_t next = (tx_head + 1) & TX_MASK;
 62
 63      // Pełny bufor: czekamy, aż przerwanie wyśle znak (ok. 1 ms)
 64      while (next == tx_tail) {
 65      }
 66      tx_buf[tx_head] = c;
 67      tx_head = next;
 68      UCSRB |= _BV(UDRIE);
 69  }
```

- **Linia 59:** Funkcja wysyłająca jeden znak.
- **Linia 60:** Początek funkcji.
- **Linia 61:** Oblicz następną pozycję głowy.
- **Linia 63:** Komentarz: gdy bufor jest pełny, czekamy, aż przerwanie wyśle znak, co trwa około 1 ms.
- **Linia 64:** Dopóki po dopisaniu głowa zrównałaby się z ogonem...
- **Linia 65:** ...czekaj. Pętla jest pusta, nic nie robi. Ogon przesuwa przerwanie, dlatego zmienne są `volatile`.
- **Linia 66:** Wstaw znak do bufora.
- **Linia 67:** Przesuń głowę.
- **Linia 68:** Włącz przerwanie nadawania (`UDRIE`), które zacznie opróżniać bufor.
- **Linia 69:** Koniec funkcji.

### Wysłanie napisu z pamięci flash

```c
 71  void uart_puts_P(PGM_P s)
 72  {
 73      char c;
 74
 75      while ((c = pgm_read_byte(s++)) != 0)
 76          uart_putc(c);
 77  }
```

- **Linia 71:** Funkcja dostaje wskaźnik na napis we flash.
- **Linia 72:** Początek funkcji.
- **Linia 73:** Zmienna na bieżący znak.
- **Linia 75:** Dopóki odczytany z flash znak nie jest zerem, czyli końcem napisu. `s++` przesuwa wskaźnik na następny znak.
- **Linia 76:** Wyślij znak.
- **Linia 77:** Koniec funkcji.

### Wysłanie liczby jako tekstu

Układ przechowuje liczbę 123 jako wartość, a nie jako trzy znaki. Tutaj odcinamy kolejne cyfry przez dzielenie przez 10. Cyfry wychodzą od końca (3, 2, 1), więc odkładamy je do tablicy i wysyłamy w odwrotnej kolejności.

```c
 79  void uart_put_uint(uint32_t value)
 80  {
 81      char digits[10];
 82      uint8_t n = 0;
 83      uint16_t small;
 84
 85      // Dzielenie 32-bitowe jest przy 1 MHz wolne, więc używamy go tylko dla dużych liczb
 86      while (value > 0xFFFF) {
 87          digits[n++] = (char)('0' + value % 10);
 88          value /= 10;
 89      }
 90      small = (uint16_t)value;
 91      do {
 92          digits[n++] = (char)('0' + small % 10);
 93          small /= 10;
 94      } while (small > 0);
 95      while (n > 0)
 96          uart_putc(digits[--n]);
 97  }
```

- **Linia 79:** Funkcja dostaje liczbę 32-bitową.
- **Linia 80:** Początek funkcji.
- **Linia 81:** Tablica na cyfry. Największa liczba 32-bitowa ma 10 cyfr.
- **Linia 82:** Licznik zebranych cyfr.
- **Linia 83:** Mniejsza kopia liczby.
- **Linia 85:** Komentarz: dzielenie 32-bitowe jest przy zegarze 1 MHz wolne, więc używamy go tylko, gdy to konieczne.
- **Linia 86:** Dopóki liczba nie mieści się w 16 bitach (czyli przekracza `0xFFFF`, 65 535)...
- **Linia 87:** ...weź ostatnią cyfrę. `value % 10` to reszta z dzielenia, a dodanie `'0'` zamienia cyfrę na jej znak ASCII. `n++` przesuwa licznik.
- **Linia 88:** Odetnij tę cyfrę, dzieląc przez 10.
- **Linia 89:** Koniec pętli.
- **Linia 90:** Teraz liczba mieści się w 16 bitach, więc skopiuj ją do mniejszej zmiennej.
- **Linia 91:** Pętla `do`: wykona się co najmniej raz, dzięki czemu liczba 0 da cyfrę `0`.
- **Linia 92:** Weź ostatnią cyfrę, tak samo jak w linii 87.
- **Linia 93:** Odetnij ją.
- **Linia 94:** Powtarzaj, dopóki coś zostało.
- **Linia 95:** Dopóki są zebrane cyfry...
- **Linia 96:** ...wysyłaj je od ostatnio zebranej. `--n` najpierw zmniejsza licznik. W ten sposób cyfry idą w dobrej kolejności.
- **Linia 97:** Koniec funkcji.

### Wysłanie liczby z ułamkiem

Temperatura jest trzymana jako dziesiąte części stopnia, np. 235. Ta funkcja wypisuje ją jako `23.5`. Dzięki temu firmware w ogóle nie liczy na ułamkach.

```c
 99  void uart_put_tenths(int16_t value)
100  {
101      uint16_t v;
102
103      if (value < 0) {
104          uart_putc('-');
105          v = (uint16_t)(-value);
106      } else {
107          v = (uint16_t)value;
108      }
109      uart_put_uint(v / 10);
110      uart_putc('.');
111      uart_putc((char)('0' + v % 10));
112  }
```

- **Linia 99:** Funkcja dostaje liczbę ze znakiem, np. -35 oznacza -3,5.
- **Linia 100:** Początek funkcji.
- **Linia 101:** Zmienna na wartość bez znaku.
- **Linia 103:** Jeśli liczba jest ujemna...
- **Linia 104:** ...wyślij znak minus...
- **Linia 105:** ...i weź jej wartość przeciwną, czyli dodatnią.
- **Linia 106:** W przeciwnym razie:
- **Linia 107:** Weź ją bez zmian.
- **Linia 108:** Koniec warunku.
- **Linia 109:** Wyślij część całkowitą: 235 / 10 to 23.
- **Linia 110:** Wyślij kropkę.
- **Linia 111:** Wyślij cyfrę dziesiątych: 235 % 10 to 5, a dodanie `'0'` zamienia ją na znak.
- **Linia 112:** Koniec funkcji.

### Koniec linii

```c
114  void uart_endl(void)
115  {
116      uart_putc('\r');
117      uart_putc('\n');
118  }
```

- **Linia 114:** Funkcja wysyłająca koniec linii.
- **Linia 115:** Początek funkcji.
- **Linia 116:** Znak powrotu karetki.
- **Linia 117:** Znak nowej linii. Dzięki parze `\r\n` tekst czytelnie wyświetli się w każdym terminalu.
- **Linia 118:** Koniec funkcji.

---

[← Poprzedni](13-firmware-timer.md) · [Spis treści](README.md) · [Następny →](15-firmware-protocol.md)
