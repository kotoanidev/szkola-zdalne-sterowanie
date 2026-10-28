# Protokół UART i API HTTP

> **Status:** wersja 0.9 – zaimplementowana w firmware 0.9.0 i serwerze, sprawdzona w symulatorze simavr; czeka na test na sprzęcie.
> Ten plik jest jedynym źródłem prawdy: każda zmiana oznacza aktualizację firmware, serwera i testów w tym samym commicie.

## 1. Warstwa fizyczna UART

| Parametr | Wartość |
|---|---|
| Prędkość | 9600 bps |
| Format | 8N1, bez kontroli przepływu |
| Konfiguracja ATmega8 | U2X = 1, UBRR = 12 (F_CPU 1 MHz → 9615 bps, błąd +0,16 %) |
| Kodowanie | ASCII |
| Koniec linii | do MCU: `\n` (znak `\r` jest ignorowany); z MCU: `\r\n` (czytelne w każdym terminalu) |

## 2. Tryb zgodności z instrukcją (etap 2)

Gdy bufor linii jest pusty, pojedynczy znak działa od razu, bez Entera. Dzięki temu działa zwykły terminal:

| Znak | Działanie | Odpowiedź |
|---|---|---|
| `1` | włącz wyjście | `OK on=1` |
| `0` | wyłącz wyjście | `OK on=0` |

## 3. Komendy tekstowe (rozszerzenie)

Komenda to jedna linia o długości do 32 znaków, wielkość liter nie ma znaczenia, a nadmiarowe spacje na początku i końcu są pomijane. Każda komenda dostaje **dokładnie jedną** linię odpowiedzi: `OK ...`, `PONG`, `VER ...`, `STATE ...` albo `ERR <kod>`.

| Komenda | Opis | Odpowiedź (przykład) |
|---|---|---|
| `PING` | test łącza | `PONG` |
| `VER` | wersja firmware | `VER 0.9.0` |
| `ON` / `OFF` / `TOGGLE` | stan wyjścia | `OK on=1` |
| `PWM <0-100>` | jasność w %; w trybie AUTO przełącza na MANUAL | `OK pwm=60` lub `OK pwm=60 mode=MANUAL` |
| `MODE MANUAL` / `MODE AUTO` | tryb (AUTO = jasność wg fotorezystora) | `OK mode=AUTO` |
| `TLIM <0-125>` | próg alarmu temperatury w °C | `OK tlim=30` |
| `GET` | pełny stan i pomiary | `STATE on=1 pwm=60 mode=MANUAL pot=512 ldr=301 temp=23.5 alarm=0 tlim=50 up=1234` |

Kody błędów: `ERR UNKNOWN` (nieznana komenda), `ERR ARG` (zły, brakujący lub nadmiarowy argument), `ERR TOOLONG` (linia dłuższa niż 32 znaki).

Odpowiedzi `OK`, `STATE` i zdarzenia `EVT` mają postać `klucz=wartość` oddzielonych spacjami, więc serwer może je czytać jednym parserem.

### Pola stanu

| Pole | Znaczenie |
|---|---|
| `on` | wyjście włączone (1) / wyłączone (0) |
| `pwm` | jasność 0–100 %; w trybie AUTO wyliczona z fotorezystora |
| `mode` | `MANUAL` albo `AUTO` |
| `pot`, `ldr` | surowy odczyt ADC0 i ADC1 (0–1023, średnia z 16 próbek) |
| `temp` | temperatura w °C z jedną cyfrą po przecinku (typ czujnika w `firmware/include/config.h`) |
| `alarm` | 1 – temperatura ≥ `tlim`; gaśnie po spadku o 2 °C poniżej progu |
| `tlim` | próg alarmu w °C |
| `up` | czas pracy od resetu w sekundach |

Jasność `pwm` to jasność odczuwana: wypełnienie PWM = (pwm/100)², np. `pwm=50` daje 25 % wypełnienia (korekcja gamma). Zmiany jasności i włączanie/wyłączanie są płynne (ok. 0,5 s). Stan `on`, `pwm` (tylko z trybu MANUAL), `mode` i `tlim` jest zapisywany w EEPROM 2 s po ostatniej zmianie i przywracany po restarcie.

## 4. Zdarzenia asynchroniczne (MCU → serwer)

Mikrokontroler sam wysyła linię zaczynającą się od `EVT`, gdy stan zmieni się lokalnie. Dzięki temu strona WWW widzi zmiany z przycisku i potencjometru.

| Zdarzenie | Kiedy |
|---|---|
| `EVT BOOT reset=POR\|EXT\|WDT\|BOR ver=0.9.0` | start po resecie (z przyczyną i wersją firmware) |
| `EVT BTN on=0` | krótkie naciśnięcie przycisku – przełączenie wyjścia |
| `EVT BTN mode=AUTO` | przytrzymanie przycisku ≥ 1 s – zmiana trybu |
| `EVT POT pwm=40` | zmiana jasności potencjometrem (o ≥ 2 %); w trybie AUTO dodatkowo ` mode=MANUAL` |
| `EVT TEMP alarm=1 temp=51.0` | włączenie lub wyłączenie alarmu temperatury |

Zasady: serwer wysyła jedną komendę naraz i czeka na odpowiedź (limit 500 ms). Linie `EVT` mogą przyjść w dowolnej chwili i nie są odpowiedzią na komendę. Pomiary (`pot`, `ldr`, `temp`) i jasność w trybie AUTO serwer odczytuje okresowo komendą `GET`.

## 5. API HTTP serwera (Raspberry Pi, port 5000)

| Metoda | Ścieżka | Opis |
|---|---|---|
| GET | `/` | interfejs WWW |
| GET | `/on`, `/off` | zgodność z instrukcją (etap 3); zwraca stan w JSON |
| GET | `/api/state` | stan i pomiary z ostatniego odczytu; `?refresh=1` – odczyt od razu (`GET` do MCU) |
| POST | `/api/state` | zmiana stanu, np. `{"on": true, "pwm": 60, "mode": "AUTO", "tlim": 45}` (pola opcjonalne, wykonywane w kolejności on, pwm, mode, tlim) |
| GET | `/api/events` | strumień Server-Sent Events |
| GET | `/api/health` | stan serwera i połączenia z mikrokontrolerem |
| GET | `/api/log` | ostatnie 50 wpisów dziennika (zdarzenia MCU, komendy ze strony, połączenia) |
| GET / POST | `/api/sim` | tylko w trybie symulacji: wirtualny sprzęt, np. `{"pot": 800, "ldr": 300, "temp": 31.5, "button": "short"\|"long", "reset": true\|"power"}` |

Przykładowa odpowiedź `GET /api/state`:

```json
{
  "connected": true,
  "on": true,
  "pwm": 60,
  "mode": "MANUAL",
  "pot": 512,
  "ldr": 301,
  "temp": 23.5,
  "alarm": false,
  "tlim": 50,
  "uptime_s": 1234
}
```

Przykładowa odpowiedź `GET /api/health`:

```json
{
  "ok": true,
  "connected": true,
  "simulated": false,
  "simulator": null,
  "port": "/dev/ttyUSB0",
  "firmware": "0.9.0",
  "server_uptime_s": 3600
}
```

Zdarzenia SSE (`/api/events`):

| Zdarzenie | Dane |
|---|---|
| `init` | zaraz po połączeniu: `state`, `log`, `simulated`, `simulator`, `firmware`, `sim` |
| `state` | pełny stan po każdej zmianie (jak `GET /api/state`) |
| `log` | nowy wpis dziennika: `{"t": czas_unix, "text": "EVT BTN on=1", "source": "mcu"\|"web"\|"server"}` |
| `sim` | tylko simavr: dioda z emulowanego układu `{"led": %, "duty": wypełnienie 0–1, "status": true\|false, ...}` |

Co 15 s serwer wysyła komentarz podtrzymujący połączenie.

Błędy zwracamy jako JSON `{"error": "..."}` z kodem HTTP 400 (zły argument), 502 (mikrokontroler odpowiedział `ERR`), 503 (brak połączenia lub odpowiedzi mikrokontrolera) albo 404 (`/api/sim` poza trybem symulacji).

Serwer wysyła do mikrokontrolera jedną komendę naraz i przyjmuje tylko pasującą odpowiedź (`PING` → `PONG`, `VER` → `VER`, `GET` → `STATE`, pozostałe → `OK`, zawsze także `ERR`); inne linie, np. urwane po otwarciu portu, pomija. Po połączeniu wysyła `PING`, `VER` i `GET`, a potem odczytuje stan co `POLL_INTERVAL` sekund oraz od razu po każdym zdarzeniu `EVT`.
