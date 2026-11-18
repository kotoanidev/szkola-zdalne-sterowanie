# Sprzęt, połączenia i wyprowadzenia

> **Status:** od etapu 2 projekt działa na płytce uruchomieniowej **ATNEL ATB rev 1.05** z mikrokontrolerem **ATmega32A-PU** (ustalone 18.11.2026 na podstawie zdjęć płytki). Etap 1 (ATmega8 na płytce stykowej) opisuje rozdział 7 – bez zmian.
> Każda zmiana przypisania pinów wymaga aktualizacji tego pliku, `firmware/include/pinout.h` **i** `sim/avrsim.c` w tym samym commicie.

## 1. Płytka ATB 1.05 – elementy używane w projekcie

| Element na płytce | Oznaczenie | Uwagi |
|---|---|---|
| ATmega32A-PU (DIP-40, w podstawce) | – | sygnatura `1E 95 02` (ATmega32/32A) odczytana 18.11.2026 |
| Konwerter USB-UART FT232RL | gniazdo USB-B, listwa „UART” | wbudowany; RX/TX mikrokontrolera łączą zworki JP7 (RX) i JP6 (TX) |
| Programator USBasp | to samo gniazdo USB-B („FT232RL PROG + USBASP”) | `make flash` (`-c usbasp -p m32`) |
| Kwarc X1 | zworki JK1–JK3 | 11,0592 MHz; fusebity wybierają zegar z kwarcu (rozdz. 5) |
| Diody LED1–LED8 | zworki J25 ↔ PORTC | LED1 = status, LED8 = dioda główna (PWM, przewodem z PD5) |
| Przyciski K1–K5 | listwa J26 KEY1–KEY5 | K1 = przycisk projektu (przewodem do PD2) |
| Potencjometr P1 (20 kΩ, „203”) | pin „POT” na listwie peryferiów | przewodem do PA0 |
| Czujnik DS18B20 (U7, TO-92) | pin „1Wire” na listwie peryferiów | przewodem do PD3 |
| Fotorezystor | – | **brak na płytce**; do PA1 trzeba go dołożyć z rezystorem 10 kΩ (dzielnik jak w rozdz. 7.1) [DO UZUPEŁNIENIA: czy dokładamy] |
| Przycisk RESET, złącze ISP (KANDA) | – | bez zmian |

Nieużywane (zostają w stanie z płytki): LCD 16×2, wyświetlacz 4×7-segmentowy, enkoder (niewlutowany), buzzer, RS485 (SN75176), RTC PCF8563, EEPROM 24C04, L293D, ULN2803, odbiornik IR, złącze ATB-MICROPORT.

## 2. Przypisanie wyprowadzeń ATmega32A (DIP-40)

| Pin | Port / funkcja | Podłączenie na ATB | Uwagi |
|---|---|---|---|
| 14 | PD0 / RXD | TXD FT232RL przez zworkę JP7 | założona |
| 15 | PD1 / TXD | RXD FT232RL przez zworkę JP6 | założona |
| 16 | PD2 / INT0 | przewód z J26 KEY1 (przycisk K1) | wewnętrzny pull-up, naciśnięcie = GND |
| 17 | PD3 / INT1 | przewód z pinu „1Wire” (DS18B20) | pull-up linii na płytce (działa: odczyt 25,9 °C) |
| 19 | PD5 / OC1A | przewód do LED8 (styk J25 od strony diody) | PWM Timer1; zworka J25 przy PC7 zdjęta |
| 22 | PC0 | LED1 przez zworkę J25 | dioda statusu |
| 40 | PA0 / ADC0 | przewód z pinu „POT” | zworka J30 nr 0 (segment A) zdjęta |
| 39 | PA1 / ADC1 | fotorezystor (poza płytką) | zworka J30 nr 1 (segment B) zdjęta |
| 30, 32 | AVCC, AREF | filtr AVCC na płytce (L2, C24); AREF bez zworki | odniesienie ADC = AVCC |
| 6–8 | PB5–PB7 (MOSI, MISO, SCK) | tylko ISP / USBasp | – |
| 9 | RESET | przycisk RESET, ISP, zworka ARST (JP8) | patrz rozdz. 4 |
| 12, 13 | XTAL2, XTAL1 | kwarc X1 przez zworki JK1–JK3 | – |

Piny, których **nie używamy**, bo płytka ma na nich inne układy albo funkcje:
- **PC2–PC5** – piny JTAG; JTAG jest wyłączony (hfuse `0xDF`), więc są wolne, ale w projekcie ich nie używamy,
- **PC0, PC1** – także SCL/SDA magistrali I²C (RTC, EEPROM); jako zwykłe wyjście dla diody PC0 działa,
- **PD6** – odbiornik IR (zworka IR_ICP),
- **PD4, PD5** – możliwe połączenie z L293D przez zworki PWM1/PWM2 (J39/J40) [DO UZUPEŁNIENIA: sprawdzić; silników nie używamy, zworki można zdjąć],
- **PA2–PA7** – segmenty wyświetlacza (zworki J30).

## 3. Lista połączeń do wykonania na płytce

1. Zdjąć zworkę J25 przy PC7 (LED8). Przewód: **PD5** (listwa PORT D) → styk J25 od strony LED8.
2. Przewód: **J26 KEY1** → **PD2**.
3. Przewód: pin **1Wire** (listwa peryferiów) → **PD3**.
4. Zdjąć zworki J30 nr 0 i 1. Przewód: pin **POT** → **PA0**. Pin POT jest w rzędzie 13 pinów pod wyświetlaczem 7-segmentowym: SW, A, B, BUZ, **POT** (5. od lewej), **1Wire** (6.), TX, DE, RX, GND, BAT, INT.
5. Fotorezystor z rezystorem 10 kΩ → **PA1** (jeśli dokładamy).
6. Zostawić założone: JP6, JP7 (UART), J25 przy PC0 (LED1), zworki JK1–JK3 (kwarc), VCCIO na „TTL”, ARST (JP8).
7. Przewody przekładać przy odłączonym kablu USB.

[DO UZUPEŁNIENIA: zdjęcie płytki po wykonaniu połączeń]

## 4. Zasilanie i USB

- Płytka jest zasilana 5 V z gniazda USB-B (zworka J2 PWR w pozycji „USB”, odczytane ze zdjęcia). Moduł ATB-PWR3 służy do zasilania z zewnętrznego zasilacza (pozycja „EXT”) – w projekcie nieużywany.
- Jeden kabel USB daje zasilanie, programator USBasp i port szeregowy FT232RL. Na Raspberry Pi FT232R ma identyfikator `0403:6001`, który serwer rozpoznaje automatycznie.
- Zworka VCCIO jest na „TTL”: linie UART mają poziom 5 V, zgodny z ATmega32A.
- Zworka ARST (JP8) łączy linie DTR/RTS z FT232 z wejściem RESET (reset jak w Arduino). Jest założona: każde otwarcie portu szeregowego resetuje mikrokontroler (`EVT BOOT reset=EXT`). Serwer to znosi, bo po otwarciu portu ponawia `PING` do 3 razy.
- GPIO Raspberry Pi nadal **nie** łączymy z mikrokontrolerem – tylko kabel USB.

## 5. Taktowanie i fusebity

Odczyt 18.11.2026 (tylko odczyt, `make read-fuses` oraz lock bity):

| Bajt | Wartość | Znaczenie |
|---|---|---|
| lfuse | `0xFF` | CKSEL = 1111, SUT = 11: zegar z zewnętrznego kwarcu (X1, 11,0592 MHz); BOD wyłączony |
| hfuse | `0xDF` | JTAG wyłączony, SPIEN włączony (ISP działa), CKOPT niezaprogramowany, EESAVE niezaprogramowany (wgranie programu kasuje EEPROM), start od adresu 0 |
| lock | `0xFF` | brak blokad |

- Układ pracuje szybciej niż 6 MHz: sygnatura odczytuje się poprawnie przy zegarze ISP 1,5 MHz (musi być poniżej 1/4 zegara układu), a UART przy `F_CPU = 11059200UL` działa bez błędów.
- **Na płytkę ATB wgrywamy zawsze:** `make clean flash F_CPU=11059200UL`. UART ma wtedy dokładnie 9600 bps (UBRR = 143), a takt 1 ms, ADC i 1-Wire dopasowują się same. `clean` jest potrzebne, bo zmiana `F_CPU` nie wymusza ponownej kompilacji.
- Domyślne `F_CPU = 1000000UL` w Makefile zostaje, bo zakładają je symulator (`sim/`) i testy. Samo `make flash` wgrałoby program z błędnym zegarem – UART by nie działał.
- Nota katalogowa przy kwarcu powyżej 8 MHz zaleca zaprogramowany CKOPT; oscylator pracuje stabilnie także bez tego. Fusebitów **nie zapisujemy** bez wyraźnej decyzji zespołu (CLAUDE.md).
- Zegar ISP USBasp: `-B 8` (ok. 94–125 kHz), z dużym zapasem.
- Przed pierwszym wgraniem na płytce był fabryczny program ATNEL (ok. 9 KB, nic nie wysyła przez UART). Jego kopia jest tylko lokalnie – to cudzy program, nie umieszczamy go w repozytorium.

## 6. Do potwierdzenia na płytce

| Sprawdzenie | Jak | Wynik |
|---|---|---|
| Układ i sygnatura | `make check` → `1E 95 02` (ATmega32/32A) | ✅ `1E 95 02` (18.11.2026) |
| Fusebity | `make read-fuses` | ✅ lfuse `0xFF`, hfuse `0xDF`, lock `0xFF` – kwarc, `F_CPU = 11059200UL` (rozdz. 5) |
| Polaryzacja diod | dioda statusu ma krótko błyskać co ok. 1 s (`LED_ACTIVE_LOW 1` w `pinout.h`) | ✅ LED1 błyska poprawnie – diody świecą stanem niskim |
| Przyciski | K1 → `EVT BTN` | ❌ brak reakcji na K1 (18.11.2026) – do wyjaśnienia: przewód KEY1 → PD2, czy K1 zwiera do GND |
| USB | FT232R i USBasp jednocześnie | ✅ oba widoczne na jednym kablu (Linux: `/dev/ttyUSB0` i USBasp) [DO UZUPEŁNIENIA: Windows, Menedżer urządzeń] |
| Pull-up linii 1-Wire | odpowiedź `GET` bez `temp=ERR` | ✅ `temp=25.9` |
| Zworka ARST | otwarcie portu → `EVT BOOT reset=EXT` | ✅ założona |
| Zworki J39/J40 | oględziny | [DO UZUPEŁNIENIA] |

## 7. Etap 1 – płytka stykowa z ATmega8 (bez zmian)

Ten rozdział opisuje układ z etapu 1 (`firmware/demo/etap1.c`, piny w `firmware/demo/pinout.h`).

### 7.1. Lista elementów

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
| Fotorezystor + rezystor 10 kΩ | 1 | tryb AUTO (ADC1): fotorezystor od +5 V do ADC1, 10 kΩ od ADC1 do GND (więcej światła = wyższe napięcie); połączenie odwrotne – `LDR_INVERT 1` w `config.h` |
| Czujnik temperatury | 1 | typ **do ustalenia**: LM35 (wyjście na ADC2) albo NTC 10 kΩ od +5 V do ADC2 i 10 kΩ od ADC2 do GND; wybór stałą `TEMP_SENSOR` w `firmware/include/config.h` (domyślnie LM35) |
| Kondensatory 100 nF | 3 | VCC–GND, AVCC–GND, AREF–GND |
| Rezystor 10 kΩ | 1 | RESET → VCC |
| Multimetr | 1 | pomiary w etapie 4 |

### 7.2. Przypisanie wyprowadzeń ATmega8 (DIP-28)

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

### 7.3. Zasilanie

- Układ zasilamy napięciem **5 V z jednego źródła**: z USBasp (zworka zasilania) w trakcie programowania **albo** z pinu 5 V konwertera USB-UART. Nigdy z dwóch źródeł jednocześnie.
- Masa ATmega8, konwertera i Raspberry Pi musi być wspólna (przez USB jest wspólna automatycznie).
- GPIO Raspberry Pi (3,3 V) **nie** łączymy bezpośrednio z ATmega8. Komunikacja idzie wyłącznie przez konwerter USB-UART.

### 7.4. Konwerter USB-UART – wersja uniwersalna

Połączenie: konwerter **TXD → PD0 (pin 2)**, konwerter **RXD ← PD1 (pin 3)**, **GND ↔ GND**. Linie TX/RX łączymy na krzyż.

| Sytuacja | Postępowanie |
|---|---|
| Konwerter ma zworkę 3V3 / 5V | ustaw **5V** |
| Konwerter tylko 3,3 V | ATmega8 odczyta 3,3 V jako „1” (próg 0,6·VCC = 3,0 V, mały margines). Linia ATmega TXD (5 V) → RXD konwertera: sprawdź w karcie katalogowej, czy wejście toleruje 5 V; jeśli nie, dodaj dzielnik 1 kΩ / 2 kΩ |
| Konwerter zasila układ | tylko gdy USBasp jest odłączony lub ma zdjętą zworkę zasilania |

Rozpoznawanie przez serwer (Raspberry Pi): automatycznie po identyfikatorach USB (VID:PID) popularnych układów – CH340 `1a86:7523`, CP2102 `10c4:ea60`, FT232R `0403:6001`, PL2303 `067b:2303`. Port można też wskazać ręcznie zmienną środowiskową.

### 7.5. Taktowanie i fusebity

- Taktowanie: **fabryczne – wewnętrzny oscylator RC 1 MHz** (`F_CPU = 1000000UL`).
- Fabryczne fusebity ATmega8: `lfuse = 0xE1`, `hfuse = 0xD9`. W etapie 1 potwierdzamy je **odczytem** (`make read-fuses`).
- Fusebitów **nie zapisujemy** bez wyraźnej decyzji zespołu (zła wartość może zablokować układ).
- Skutki dla projektu: UART 9600 bps tylko z podwójną prędkością (U2X = 1, UBRR = 12, błąd +0,16 %); zegar ISP USBasp musi być < 250 kHz (`-B 8` w Makefile).
- Dokładność oscylatora RC to kilka procent. Jeśli w terminalu pojawią się „krzaki”, korygujemy rejestr `OSCCAL` programowo, bez zmiany fusebitów.
