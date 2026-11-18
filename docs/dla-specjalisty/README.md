# Dla specjalisty: cały kod wytłumaczony linijka po linijce

Ten katalog tłumaczy każdą linijkę kodu projektu tak, by zrozumiała go osoba, która **nigdy wcześniej nie widziała kodu**. Nie trzeba niczego instalować, wystarczy czytać.

Projekt to zdalne sterowanie urządzeniem niskonapięciowym przez sieć LAN: strona WWW → serwer na Raspberry Pi → układ ATmega8 → dioda LED, przycisk i czujniki.

## Jak czytać

1. Zacznij od [podstaw ogólnych](00-podstawy-ogolne.md). Tłumaczą, czym są bit, rejestr, przerwanie czy PWM.
2. Przeczytaj [`demo/etap1.c`](21-firmware-demo.md). To najprostszy kod w projekcie, ma tylko 43 linie.
3. Gdy trafisz na nieznany zapis, zajrzyj do podstaw danego języka.
4. Dalej idź według spisu albo wybierz to, co Cię interesuje.

Każdy dokument pokazuje prawdziwy kod z numerami linii, a pod nim objaśnia każdą linię. Numer nie jest częścią kodu. Puste linie nie mają opisu, bo komputer je pomija.

## Spis treści

### Podstawy (bez kodu projektu)

| Dokument | O czym |
|---|---|
| [Podstawy ogólne](00-podstawy-ogolne.md) | jak działa projekt, bity, pamięć, rejestry, przerwania, PWM, ADC, UART |
| [Podstawy języka C](01-podstawy-jezyka-c.md) | zmienne, funkcje, pętle, wskaźniki, preprocesor |
| [Podstawy Pythona](02-podstawy-pythona.md) | wcięcia, klasy, dekoratory, wątki, testy |
| [Podstawy WWW](03-podstawy-www.md) | HTTP, JSON, HTML, CSS, JavaScript |
| [Podstawy narzędzi](04-podstawy-narzedzi.md) | terminal, kompilacja, Makefile, PowerShell, Git |

### Firmware: program w układzie ATmega8 (język C)

| Dokument | Plik | Co robi |
|---|---|---|
| [Makefile](10-firmware-makefile.md) | `firmware/Makefile` | przepis na budowę i wgrywanie programu |
| [Nagłówki](11-firmware-naglowki.md) | `firmware/include/*.h` | ustawienia, piny, spisy funkcji |
| [main](12-firmware-main.md) | `firmware/src/main.c` | start programu i pętla główna |
| [timer](13-firmware-timer.md) | `firmware/src/timer.c` | zegar milisekundowy |
| [uart](14-firmware-uart.md) | `firmware/src/uart.c` | wysyłanie i odbiór znaków |
| [protocol](15-firmware-protocol.md) | `firmware/src/protocol.c` | rozumienie komend tekstowych |
| [pwm](16-firmware-pwm.md) | `firmware/src/pwm.c` | jasność diody |
| [adc](17-firmware-adc.md) | `firmware/src/adc.c` | pomiary napięć |
| [sensors](18-firmware-sensors.md) | `firmware/src/sensors.c` | temperatura, potencjometr, tryb AUTO |
| [button](19-firmware-button.md) | `firmware/src/button.c` | przycisk, krótkie i długie naciśnięcie |
| [state](20-firmware-state.md) | `firmware/src/state.c` | stan urządzenia i pamięć EEPROM |
| [demo](21-firmware-demo.md) | `firmware/demo/etap1.c` | prosty program pokazowy |

### Serwer: program na Raspberry Pi (Python)

| Dokument | Plik | Co robi |
|---|---|---|
| [Start i konfiguracja](30-serwer-start.md) | `server/app/__main__.py`, `config.py`, `__init__.py` | uruchomienie i ustawienia |
| [protocol](31-serwer-protocol.md) | `server/app/protocol.py` | rozkładanie odpowiedzi układu |
| [transport](32-serwer-transport.md) | `server/app/transport.py` | łącze: port szeregowy albo symulator |
| [device](33-serwer-device.md) | `server/app/device.py` | główny obiekt, wątki, komendy |
| [routes](34-serwer-routes.md) | `server/app/routes.py` | adresy strony i API |
| [mcu_sim](35-serwer-mcu-sim.md) | `server/app/mcu_sim.py` | układ udawany w Pythonie |
| [avrsim](36-serwer-avrsim.md) | `server/app/avrsim.py` | sterowanie emulatorem firmware |
| [Uruchamianie](37-serwer-wdrozenie.md) | `server/deploy/...`, `pytest.ini`, `requirements*.txt` | autostart i zależności |

### Strona WWW

| Dokument | Plik | Co robi |
|---|---|---|
| [HTML](40-www-index-html.md) | `server/app/static/index.html` | zawartość strony |
| [CSS](41-www-app-css.md) | `server/app/static/app.css` | wygląd |
| [JavaScript](42-www-app-js.md) | `server/app/static/app.js` | zachowanie |

### Testy

| Dokument | Plik | Co sprawdza |
|---|---|---|
| [Przygotowanie i parser](50-testy-conftest-protocol.md) | `server/tests/conftest.py`, `test_protocol.py` | wspólne przygotowania, rozkładanie tekstu |
| [API](51-testy-api.md) | `server/tests/test_api.py` | serwer od strony przeglądarki |
| [Symulator](52-testy-mcu-sim.md) | `server/tests/test_mcu_sim.py` | zgodność symulatora z firmware |
| [Firmware](53-testy-firmware-avr.md) | `server/tests/test_firmware_avr.py` | prawdziwy firmware w emulatorze |

### Symulator firmware

| Dokument | Plik | Co robi |
|---|---|---|
| [Makefile](60-sim-makefile.md) | `sim/Makefile` | pobiera i buduje simavr |
| [avrsim](61-sim-avrsim-c.md) | `sim/avrsim.c` | wirtualna płytka z prawdziwym firmware |

### Narzędzia i konfiguracja

| Dokument | Plik | Co robi |
|---|---|---|
| [github-tymczasowo](70-narzedzia-github-tymczasowo.md) | `tools/github-tymczasowo.ps1` | logowanie do GitHuba bez śladów |
| [md2docx](71-narzedzia-md2docx.md) | `tools/md2docx.ps1` | zamiana Markdowna na Worda |
| [Konfiguracja](80-repo-konfiguracja.md) | `.editorconfig`, `.gitattributes`, `.gitignore` | ustawienia edytorów i gita |

## Co obejmuje ten opis

Opisany jest cały kod i wszystkie pliki konfiguracyjne. Pominięte są dokumenty (`docs/`, `README.md`, `CLAUDE.md`, `LICENSE`, szablon PR w `.github/`), bo nie są kodem.

Kod w tych dokumentach odpowiada stanowi repozytorium z commita `6d200b0`. Jeśli kod się później zmieni, numery linii i opisy trzeba zaktualizować. W razie rozbieżności prawdziwy jest zawsze kod, a dla sprzętu i protokołu `docs/sprzet.md` oraz `docs/protokol.md`.

Ten katalog tłumaczy, **jak działa kod**. Nie zawiera wyników pomiarów ani testów na sprzęcie. Rzeczy jeszcze nieustalone, np. typ czujnika temperatury, są opisane jako nieustalone.
