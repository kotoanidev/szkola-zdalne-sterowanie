---
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 2
nazwa: Komunikacja szeregowa (UART w ATmega8)
data: 18.11.2026
---

# Sprawozdanie – etap 2

<!-- Szkic. Wpisujemy tylko to, co faktycznie zrobiono i zmierzono. Konwersja: tools\md2docx.ps1 -->

## 1. Cel etapu

Celem etapu była konfiguracja interfejsu UART w mikrokontrolerze ATmega8 i nawiązanie dwukierunkowej komunikacji tekstowej z komputerem przez konwerter USB-UART, tak aby mikrokontroler reagował na komendy wysyłane z terminala (np. `1` – włącz, `0` – wyłącz).

## 2. Wykaz zastosowanych urządzeń, narzędzi i oprogramowania

- **Sprzęt:**
  - układ z etapu 1 (ATmega8, USBasp, dioda główna PB1, dioda statusu PD7, przycisk PD2),
  - konwerter USB-UART [DO UZUPEŁNIENIA: model / układ (CH340, CP2102, FT232RL, PL2303), tryb 5 V lub 3,3 V],
  - potencjometr 10 kΩ (ADC0), fotorezystor z rezystorem 10 kΩ (ADC1), czujnik temperatury [DO UZUPEŁNIENIA: LM35 / NTC] (ADC2),
  - kondensatory 100 nF na AVCC i AREF.
- **Środowisko:**
  - avr-gcc, avrdude, GNU Make (jak w etapie 1), program główny `firmware/` (`make`, `make flash`), wersja firmware 0.9.0,
  - terminal szeregowy [DO UZUPEŁNIENIA: np. Serial Monitor z Arduino IDE],
  - parametry transmisji: 9600 bps, 8N1, bez kontroli przepływu; U2X = 1, UBRR = 12 (przy F_CPU = 1 MHz daje 9615 bps, błąd +0,16 %).

<!-- Serial Monitor: dla znaków 1 / 0 wystarczy dowolne zakończenie linii; komendy tekstowe (PING, GET...) wymagają ustawienia „Nowa linia” (Newline). -->

## 3. Schemat połączeń / opis ideowy

<!-- Schemat lub zdjęcie układu wstawić w Wordzie. Tabela poniżej wynika z docs/sprzet.md i firmware/include/pinout.h. -->

[DO UZUPEŁNIENIA: schemat lub zdjęcie układu]

Do układu z etapu 1 dołączono konwerter USB-UART. Linie danych łączy się na krzyż: wyjście TXD konwertera do wejścia RXD mikrokontrolera i odwrotnie. Masa jest wspólna. Układ zasilany jest z jednego źródła 5 V (z USBasp albo z konwertera, nigdy z obu jednocześnie).

| Pin | Port / funkcja | Podłączenie |
|---|---|---|
| 2 | PD0 / RXD | ← TXD konwertera USB-UART |
| 3 | PD1 / TXD | → RXD konwertera USB-UART |
| 8, 22 | GND | ↔ GND konwertera (masa wspólna) |
| 20 | AVCC | +5 V, 100 nF do GND |
| 21 | AREF | 100 nF do GND (odniesienie ADC = AVCC) |
| 23 | PC0 / ADC0 | suwak potencjometru |
| 24 | PC1 / ADC1 | dzielnik z fotorezystorem |
| 25 | PC2 / ADC2 | czujnik temperatury |

[DO UZUPEŁNIENIA: potwierdzenie, że rzeczywisty montaż jest zgodny z tabelą; źródło zasilania; ewentualne odstępstwa]

## 4. Kluczowy fragment kodu / Konfiguracja

Inicjalizacja i odbiór UART (`firmware/src/uart.c`). Odbiór działa na przerwaniach: każdy znak trafia do bufora kołowego, a pętla główna odczytuje go funkcją `uart_read()`.

```c
#define UBRR_VALUE ((F_CPU + 4 * UART_BAUD) / (8 * UART_BAUD) - 1)  // = 12

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
- **R4** – pomiary ADC (potencjometr, fotorezystor, temperatura, średnia z 16 próbek) zwracane komendą `GET`,
- **R6** – watchdog (500 ms) oraz zapis stanu w EEPROM 2 s po ostatniej zmianie i przywrócenie go po restarcie.

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** [DO UZUPEŁNIENIA]
- **Napotkane trudności i sposób ich rozwiązania:** [DO UZUPEŁNIENIA]

<!-- Wpisać tylko to, co faktycznie się wydarzyło, np. „krzaki” w terminalu (wtedy korekta OSCCAL, bez zmiany fusebitów), zamienione linie TX/RX, problemy ze sterownikiem konwertera. -->

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** dioda reaguje na znaki wysyłane z klawiatury komputera.

| Wysłano | Oczekiwana reakcja | Wynik |
|---|---|---|
| (włączenie zasilania) | `EVT BOOT reset=POR ver=0.9.0` | [DO UZUPEŁNIENIA] |
| `1` | dioda włączona, odpowiedź `OK on=1` | [DO UZUPEŁNIENIA] |
| `0` | dioda wyłączona, odpowiedź `OK on=0` | [DO UZUPEŁNIENIA] |
| `PING` | `PONG` | [DO UZUPEŁNIENIA] |
| `VER` | `VER 0.9.0` | [DO UZUPEŁNIENIA] |
| `ON`, potem `PWM 30` | jasność płynnie zmienia się na 30 %, `OK pwm=30` | [DO UZUPEŁNIENIA] |
| `GET` | linia `STATE on=... pwm=... pot=... ldr=... temp=...` | [DO UZUPEŁNIENIA] |
| obrót potencjometru | `EVT POT pwm=...`, zmiana jasności | [DO UZUPEŁNIENIA] |
| naciśnięcie przycisku | `EVT BTN on=...`, zmiana stanu diody | [DO UZUPEŁNIENIA] |
| `ABC` | `ERR UNKNOWN` | [DO UZUPEŁNIENIA] |
| `PWM 150` | `ERR ARG` | [DO UZUPEŁNIENIA] |
| `PWM 60`, odczekać 2 s, wyłączyć i włączyć zasilanie, `GET` | `EVT BOOT reset=POR ...`, w `STATE` `pwm=60` (stan z EEPROM) | [DO UZUPEŁNIENIA] |

**Wynik weryfikacji:** [DO UZUPEŁNIENIA]

**Wnioski:** [DO UZUPEŁNIENIA]
