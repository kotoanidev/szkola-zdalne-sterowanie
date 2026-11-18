---
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 2
nazwa: Komunikacja szeregowa (UART w ATmega32A na płytce ATB 1.05)
data: 18.11.2026
---

# Sprawozdanie – etap 2

<!-- Szkic. Wpisujemy tylko to, co faktycznie zrobiono i zmierzono. Konwersja: tools\md2docx.ps1 -->

## 1. Cel etapu

Celem etapu była konfiguracja interfejsu UART mikrokontrolera i nawiązanie dwukierunkowej komunikacji tekstowej z komputerem przez konwerter USB-UART, tak aby mikrokontroler reagował na komendy wysyłane z terminala (np. `1` – włącz, `0` – wyłącz).

Od tego etapu układ działa na płytce uruchomieniowej ATNEL ATB 1.05 z mikrokontrolerem ATmega32A zamiast ATmega8 na płytce stykowej z etapu 1. Płytka ma wbudowany konwerter USB-UART i programator, więc jeden kabel USB zasila układ, wgrywa program i przenosi komunikację szeregową. Rejestry UART, Timer1, Timer2, ADC i watchdoga mają w ATmega32A te same nazwy co w ATmega8, więc przeniesienie programu polegało głównie na zmianie pinów.

## 2. Wykaz zastosowanych urządzeń, narzędzi i oprogramowania

- **Sprzęt:**
  - płytka uruchomieniowa ATNEL ATB rev 1.05 z mikrokontrolerem ATmega32A-PU (DIP-40),
  - konwerter USB-UART FT232RL i programator USBasp wbudowane w płytkę (wspólne gniazdo USB-B),
  - na płytce: diody LED1 i LED8, przycisk K1, potencjometr P1 (20 kΩ), czujnik temperatury DS18B20 (1-Wire),
  - fotorezystor z rezystorem 10 kΩ [DO UZUPEŁNIENIA: czy dołożony],
  - przewody połączeniowe.
- **Środowisko:**
  - avr-gcc 7.3.0 (z Arduino IDE), avrdude 8.0, GNU Make – komputer z Linuxem; program główny `firmware/`, wgrany poleceniem `make clean flash F_CPU=11059200UL`, wersja firmware 0.10.0 (4728 B flash, 14,4 %),
  - terminal szeregowy: skrypt w Pythonie z biblioteką pyserial (port `/dev/ttyUSB0`) [DO UZUPEŁNIENIA: jeśli w szkole także Serial Monitor z Arduino IDE],
  - parametry transmisji: 9600 bps, 8N1, bez kontroli przepływu, U2X = 1,
  - zegar mikrokontrolera: kwarc 11,0592 MHz (odczyt fusebitów: lfuse `0xFF`, hfuse `0xDF`, lock `0xFF`), `F_CPU = 11059200UL`, UBRR = 143 – prędkość 9600 bps bez błędu.

<!-- Serial Monitor: dla znaków 1 / 0 wystarczy dowolne zakończenie linii; komendy tekstowe (PING, GET...) wymagają ustawienia „Nowa linia” (Newline). -->

## 3. Schemat połączeń / opis ideowy

<!-- Zdjęcie płytki z połączeniami wstawić w Wordzie. Tabela wynika z docs/sprzet.md i firmware/include/pinout.h. -->

[DO UZUPEŁNIENIA: zdjęcie płytki po wykonaniu połączeń]

Komputer łączy się z płytką jednym kablem USB. Układ FT232RL na płytce zamienia USB na UART i jest połączony z wyprowadzeniami RXD (PD0) i TXD (PD1) mikrokontrolera przez zworki JP7 i JP6. Zworka VCCIO jest w pozycji TTL, więc linie UART mają poziom 5 V. Pozostałe elementy płytki łączy się z mikrokontrolerem zworkami albo przewodami:

| Pin ATmega32A | Funkcja | Połączenie na płytce ATB |
|---|---|---|
| PD0 / RXD | odbiór UART | TXD FT232RL, zworka JP7 |
| PD1 / TXD | nadawanie UART | RXD FT232RL, zworka JP6 |
| PD5 / OC1A | dioda główna (PWM) | przewód do LED8, zworka J25 przy PC7 zdjęta |
| PC0 | dioda statusu | LED1, zworka J25 |
| PD2 | przycisk | przewód z J26 KEY1 (K1) |
| PA0 / ADC0 | potencjometr | przewód z pinu POT, zworka J30 nr 0 zdjęta |
| PA1 / ADC1 | fotorezystor | dzielnik poza płytką, zworka J30 nr 1 zdjęta |
| PD3 | czujnik DS18B20 | przewód z pinu 1Wire |

[DO UZUPEŁNIENIA: potwierdzenie, że połączenia na płytce są zgodne z tabelą; ewentualne odstępstwa]

## 4. Kluczowy fragment kodu / Konfiguracja

Inicjalizacja i odbiór UART (`firmware/src/uart.c`). Wartość UBRR liczy się z `F_CPU` podanego w Makefile. Odbiór działa na przerwaniach: każdy znak trafia do bufora kołowego, a pętla główna odczytuje go funkcją `uart_read()`.

```c
#define UBRR_VALUE ((F_CPU + 4 * UART_BAUD) / (8 * UART_BAUD) - 1)  // 143 przy 11,0592 MHz

void uart_init(void)
{
    UBRRH = (uint8_t)(UBRR_VALUE >> 8);
    UBRRL = (uint8_t)UBRR_VALUE;
    UCSRA = _BV(U2X);                                   // podwójna prędkość
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);       // 8N1
    UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);         // odbiór, nadawanie, przerwanie RX
}

ISR(USART_RXC_vect)
{
    char c = UDR;
    uint8_t next = (rx_head + 1) & RX_MASK;

    // Przy przepełnieniu znak przepada, komenda dostanie ERR
    if (next != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}
```

Obsługa znaków `1` / `0` z instrukcji (`firmware/src/protocol.c`). Gdy bufor linii jest pusty, pojedynczy znak działa od razu, bez Entera:

```c
if (len == 0 && !too_long && (c == '1' || c == '0')) {
    state_set_on(c == '1');
    reply_on();        // odpowiedź "OK on=1" lub "OK on=0"
    return 1;
}
```

Rozszerzenia zrealizowane w etapie 2 (opis w `docs/protokol.md`):
- **R3** – odbiór i nadawanie na przerwaniach z buforami kołowymi, komendy tekstowe (`PING`, `VER`, `ON`, `OFF`, `TOGGLE`, `PWM`, `MODE`, `TLIM`, `GET`), jedna linia odpowiedzi na komendę, zdarzenia `EVT` (start, przycisk, potencjometr, alarm temperatury),
- **R4** – pomiary ADC (potencjometr, fotorezystor, średnia z 16 próbek) oraz temperatura z czujnika DS18B20 (1-Wire, pomiar co 1 s, suma kontrolna CRC; `temp=ERR` przy braku odczytu), zwracane komendą `GET`,
- **R6** – watchdog (500 ms) oraz zapis stanu w EEPROM 2 s po ostatniej zmianie i przywrócenie go po restarcie.

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** płytka zgłosiła się w systemie jako dwa urządzenia na jednym kablu USB (FT232R i USBasp). Odczytana sygnatura `1E 95 02` potwierdziła ATmega32A. Program wgrał się i przeszedł weryfikację (4728 B). Komunikacja 9600 bps działała bez błędów od pierwszego uruchomienia, czujnik DS18B20 podał 25,9 °C, a obrót potencjometru P1 płynnie zmieniał jasność LED8.
- **Napotkane trudności i sposób ich rozwiązania:**
  - Fusebity były inne, niż zakładaliśmy: zamiast fabrycznego oscylatora RC 1 MHz układ pracuje na kwarcu 11,0592 MHz (lfuse `0xFF`), a JTAG jest wyłączony (hfuse `0xDF`). Przy programie zbudowanym dla 1 MHz UART nie działałby. Częstotliwość potwierdziliśmy bez zmiany fusebitów: sygnatura odczytuje się przy zegarze ISP 1,5 MHz, a to wymaga zegara układu powyżej 6 MHz. Program budujemy z `F_CPU=11059200UL`.
  - Przed przełożeniem przewodów układ wysyłał ciągle zdarzenia `EVT POT` z jasnością skaczącą między 7 a 16 %. Wejście PA0 było wtedy połączone zworką J30 z wyświetlaczem 7-segmentowym, a potencjometr nie był podłączony. Po zdjęciu zworki J30 nr 0 i podłączeniu pinu POT do PA0 zdarzenia ustały.
  - Przycisk K1 nie wywołuje reakcji (brak zmiany stanu LED8). [DO UZUPEŁNIENIA: przyczyna i rozwiązanie]

<!-- Wpisać tylko to, co faktycznie się wydarzyło, np. wynik make check i make read-fuses, polaryzacja diod, „krzaki” w terminalu (zły F_CPU), temp=ERR (połączenie 1Wire). -->

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** dioda reaguje na znaki wysyłane z klawiatury komputera.

| Wysłano | Oczekiwana reakcja | Wynik |
|---|---|---|
| (podłączenie USB / reset) | `EVT BOOT reset=... ver=0.10.0` | ✅ `EVT BOOT reset=EXT ver=0.10.0` – otwarcie portu resetuje układ przez zworkę ARST |
| `1` | dioda LED8 włączona, odpowiedź `OK on=1` | [DO UZUPEŁNIENIA] |
| `0` | dioda LED8 wyłączona, odpowiedź `OK on=0` | [DO UZUPEŁNIENIA] |
| `PING` | `PONG` | ✅ `PONG` |
| `VER` | `VER 0.10.0` | ✅ `VER 0.10.0` |
| `ON`, potem `PWM 30` | jasność płynnie zmienia się na 30 %, `OK pwm=30` | ✅ `OK on=1`, `OK pwm=30`; po `PWM 100` LED8 świeci pełną jasnością [DO UZUPEŁNIENIA: obserwacja przy 30 %] |
| `GET` | linia `STATE on=... pwm=... pot=... ldr=... temp=...` z temperaturą z DS18B20 | ✅ `STATE on=0 pwm=50 mode=MANUAL pot=0 ldr=0 temp=25.9 alarm=0 tlim=50 up=4` |
| obrót potencjometru P1 | `EVT POT pwm=...`, zmiana jasności | ✅ jasność LED8 zmienia się płynnie [DO UZUPEŁNIENIA: zapis zdarzeń `EVT POT`] |
| naciśnięcie K1 | `EVT BTN on=...`, zmiana stanu diody | ❌ brak reakcji [DO UZUPEŁNIENIA: po naprawie] |
| `ABC` | `ERR UNKNOWN` | ✅ `ERR UNKNOWN` |
| `PWM 150` | `ERR ARG` | ✅ `ERR ARG` |
| `PWM 60`, odczekać 2 s, odłączyć i podłączyć USB, `GET` | `EVT BOOT reset=POR ...`, w `STATE` `pwm=60` (stan z EEPROM) | po resecie z portu (`reset=EXT`) wrócił zapisany stan `on=1 pwm=30` [DO UZUPEŁNIENIA: test z odłączeniem USB] |

**Wynik weryfikacji:** [DO UZUPEŁNIENIA]

**Wnioski:** [DO UZUPEŁNIENIA]
