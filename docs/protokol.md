# Protokół UART i API HTTP

> **Status:** wersja robocza 0.1. Protokół UART finalizujemy w etapie 2, a API HTTP w etapie 3.
> Ten plik jest jedynym źródłem prawdy: każda zmiana oznacza aktualizację firmware, serwera i testów w tym samym commicie.

## 1. Warstwa fizyczna UART

| Parametr | Wartość |
|---|---|
| Prędkość | 9600 bps |
| Format | 8N1, bez kontroli przepływu |
| Konfiguracja ATmega8 | U2X = 1, UBRR = 12 (F_CPU 1 MHz → 9615 bps, błąd +0,16 %) |
| Kodowanie | ASCII |
| Koniec linii | `\n` (znak `\r` jest ignorowany) |

## 2. Tryb zgodności z instrukcją (etap 2)

Gdy bufor linii jest pusty, pojedynczy znak działa od razu, bez Entera. Dzięki temu działa zwykły terminal:

| Znak | Działanie | Odpowiedź |
|---|---|---|
| `1` | włącz wyjście | `OK on=1` |
| `0` | wyłącz wyjście | `OK on=0` |

## 3. Komendy tekstowe (rozszerzenie)

Komenda to jedna linia o długości do 32 znaków, wielkość liter nie ma znaczenia. Każda komenda dostaje **dokładnie jedną** linię odpowiedzi: `OK ...`, `PONG`, `VER ...`, `STATE ...` albo `ERR <kod>`.

| Komenda | Opis | Odpowiedź (przykład) |
|---|---|---|
| `PING` | test łącza | `PONG` |
| `VER` | wersja firmware | `VER 0.1.0` |
| `ON` / `OFF` / `TOGGLE` | stan wyjścia | `OK on=1` |
| `PWM <0-100>` | jasność w % | `OK pwm=60` |
| `MODE MANUAL` / `MODE AUTO` | tryb (AUTO = jasność wg fotorezystora) | `OK mode=AUTO` |
| `GET` | pełny stan i pomiary | `STATE on=1 pwm=60 mode=MANUAL pot=512 ldr=301 temp=23.5 up=1234` |

Kody błędów: `ERR UNKNOWN` (nieznana komenda), `ERR ARG` (zły argument), `ERR TOOLONG` (za długa linia).

## 4. Zdarzenia asynchroniczne (MCU → serwer)

Mikrokontroler sam wysyła linię zaczynającą się od `EVT`, gdy stan zmieni się lokalnie. Dzięki temu strona WWW widzi zmiany z przycisku i potencjometru.

| Zdarzenie | Kiedy |
|---|---|
| `EVT BOOT reset=POR\|EXT\|WDT\|BOR` | start po resecie (z przyczyną) |
| `EVT BTN on=0` | przełączenie przyciskiem |
| `EVT POT pwm=40` | zmiana jasności potencjometrem |
| `EVT TEMP alarm=1 temp=51.0` | przekroczenie progu temperatury |

Zasady: serwer wysyła jedną komendę naraz i czeka na odpowiedź (limit 500 ms). Linie `EVT` mogą przyjść w dowolnej chwili i nie są odpowiedzią na komendę.

## 5. API HTTP serwera (Raspberry Pi, port 5000)

| Metoda | Ścieżka | Opis |
|---|---|---|
| GET | `/` | interfejs WWW |
| GET | `/on`, `/off` | zgodność z instrukcją (etap 3); zwraca stan w JSON |
| GET | `/api/state` | stan i pomiary |
| POST | `/api/state` | zmiana stanu, np. `{"on": true, "pwm": 60, "mode": "AUTO"}` (pola opcjonalne) |
| GET | `/api/events` | strumień Server-Sent Events ze zmianami stanu |
| GET | `/api/health` | stan serwera i połączenia z mikrokontrolerem |

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
  "uptime_s": 1234
}
```

Błędy zwracamy jako JSON `{"error": "..."}` z kodem HTTP 400 (zły argument) lub 503 (brak połączenia z mikrokontrolerem).
