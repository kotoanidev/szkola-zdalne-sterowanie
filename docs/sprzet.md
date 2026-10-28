# Sprzęt, połączenia i wyprowadzenia

> **Status:** projekt w wersji roboczej, do potwierdzenia przy montażu w etapie 1.
> Każda zmiana przypisania pinów wymaga aktualizacji tego pliku **i** `firmware/include/pinout.h` (powstanie w etapie 1) w tym samym commicie.

## 1. Lista elementów

| Element | Ilość | Uwagi |
|---|---|---|
| ATmega8 (DIP-28) | 1 | wersja 8 / 8A / 8L – **do ustalenia**; pracuje na fabrycznym RC 1 MHz |
| Programator USBasp + taśma ISP | 1 | 10-pin lub 6-pin |
| Konwerter USB-UART | 1 | dowolny z dostępnych (CH340 / CP2102 / FT232RL / PL2303) – patrz rozdz. 4 |
| Raspberry Pi 4 lub 5 + zasilacz + microSD | 1 | model – **do ustalenia**, system Raspberry Pi OS |
| Płytka stykowa + przewody | 1 | |
| Dioda LED (kanał główny) + rezystor 330 Ω | 1 | wyjście PWM |
| Dioda LED (status) + rezystor 330 Ω | 1 | sygnalizacja pracy / komunikacji |
| Przycisk (tact switch) | 1 | do GND, wewnętrzny pull-up |
| Potencjometr 10 kΩ | 1 | lokalna regulacja jasności (ADC0) |
| Fotorezystor + rezystor 10 kΩ | 1 | dzielnik napięcia, tryb AUTO (ADC1) |
| Czujnik temperatury | 1 | typ **do ustalenia**: LM35 / NTC 10 kΩ + 10 kΩ / DS18B20 + 4,7 kΩ |
| Kondensatory 100 nF | 3 | VCC–GND, AVCC–GND, AREF–GND |
| Rezystor 10 kΩ | 1 | RESET → VCC |
| Multimetr | 1 | pomiary w etapie 4 |

## 2. Przypisanie wyprowadzeń ATmega8 (DIP-28)

| Pin | Port / funkcja | Podłączenie | Etap |
|---|---|---|---|
| 1 | PC6 / RESET | 10 kΩ do VCC, ISP RESET | 1 |
| 2 | PD0 / RXD | ← TXD konwertera USB-UART | 2 |
| 3 | PD1 / TXD | → RXD konwertera USB-UART | 2 |
| 4 | PD2 / INT0 | przycisk do GND (pull-up wewnętrzny) | 1 |
| 5 | PD3 | rezerwa (DS18B20, jeśli wybrany taki czujnik) | – |
| 7 | VCC | +5 V, 100 nF do GND | 1 |
| 8, 22 | GND | masa wspólna | 1 |
| 13 | PD7 | dioda statusu + 330 Ω | 1 |
| 15 | PB1 / OC1A | dioda główna + 330 Ω (PWM Timer1) | 1 |
| 17 | PB3 / MOSI | tylko ISP | – |
| 18 | PB4 / MISO | tylko ISP | – |
| 19 | PB5 / SCK | tylko ISP | – |
| 20 | AVCC | +5 V, 100 nF do GND | 2 |
| 21 | AREF | 100 nF do GND (odniesienie = AVCC) | 2 |
| 23 | PC0 / ADC0 | suwak potencjometru | 2 |
| 24 | PC1 / ADC1 | dzielnik z fotorezystorem | 2 |
| 25 | PC2 / ADC2 | czujnik temperatury (analogowy) | 2 |

Zasada: piny ISP (PB3–PB5, RESET) **nie** są używane do peryferiów, dzięki czemu programowanie zawsze działa bez rozłączania układu.

Złącze ISP USBasp (10-pin): 1 MOSI, 2 VCC, 4/6/8/10 GND, 5 RESET, 7 SCK, 9 MISO.

## 3. Zasilanie

- Układ zasilamy napięciem **5 V z jednego źródła**: z USBasp (zworka zasilania) w trakcie programowania **albo** z pinu 5 V konwertera USB-UART. Nigdy z dwóch źródeł jednocześnie.
- Masa ATmega8, konwertera i Raspberry Pi musi być wspólna (przez USB jest wspólna automatycznie).
- GPIO Raspberry Pi (3,3 V) **nie** łączymy bezpośrednio z ATmega8. Komunikacja idzie wyłącznie przez konwerter USB-UART.

## 4. Konwerter USB-UART – wersja uniwersalna

Połączenie: konwerter **TXD → PD0 (pin 2)**, konwerter **RXD ← PD1 (pin 3)**, **GND ↔ GND**. Linie TX/RX łączymy na krzyż.

| Sytuacja | Postępowanie |
|---|---|
| Konwerter ma zworkę 3V3 / 5V | ustaw **5V** |
| Konwerter tylko 3,3 V | ATmega8 odczyta 3,3 V jako „1” (próg 0,6·VCC = 3,0 V, mały margines). Linia ATmega TXD (5 V) → RXD konwertera: sprawdź w karcie katalogowej, czy wejście toleruje 5 V; jeśli nie, dodaj dzielnik 1 kΩ / 2 kΩ |
| Konwerter zasila układ | tylko gdy USBasp jest odłączony lub ma zdjętą zworkę zasilania |

Rozpoznawanie przez serwer (Raspberry Pi): automatycznie po identyfikatorach USB (VID:PID) popularnych układów – CH340 `1a86:7523`, CP2102 `10c4:ea60`, FT232R `0403:6001`, PL2303 `067b:2303`. Port można też wskazać ręcznie zmienną środowiskową.

## 5. Taktowanie i fusebity

- Taktowanie: **fabryczne – wewnętrzny oscylator RC 1 MHz** (`F_CPU = 1000000UL`).
- Fabryczne fusebity ATmega8: `lfuse = 0xE1`, `hfuse = 0xD9`. W etapie 1 potwierdzamy je **odczytem** (`make read-fuses`).
- Fusebitów **nie zapisujemy** bez wyraźnej decyzji zespołu (zła wartość może zablokować układ).
- Skutki dla projektu: UART 9600 bps tylko z podwójną prędkością (U2X = 1, UBRR = 12, błąd +0,16 %); zegar ISP USBasp musi być < 250 kHz (`-B 8` w Makefile).
- Dokładność oscylatora RC to kilka procent. Jeśli w terminalu pojawią się „krzaki”, korygujemy rejestr `OSCCAL` programowo, bez zmiany fusebitów.
