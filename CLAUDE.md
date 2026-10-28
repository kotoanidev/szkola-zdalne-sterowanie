# CLAUDE.md – zasady pracy w repozytorium

Projekt szkolny (Pracownia Urządzeń Techniki Komputerowej): **zdalny system sterowania urządzeniem niskonapięciowym przez sieć LAN**. Łańcuch: przeglądarka → Flask na Raspberry Pi → konwerter USB-UART → ATmega8 → LED (PWM), przycisk, potencjometr, fotorezystor, czujnik temperatury. Realizujemy wariant zaawansowany: rozszerzenia R1–R11 opisane w `docs/PLAN_PROJEKTU.md`.

Zespół:
- **Mikołaj Milcarz** – implementacja całości (firmware, serwer, front-end, montaż),
- **Wiktor Selwon** – przegląd kodu, weryfikacja, pomiary, dokumentacja i sprawozdania.

| Etap | Termin |
|---|---|
| 1. ATmega8 + GPIO | śr. 04.11.2026 |
| 2. UART | śr. 18.11.2026 |
| 3. Raspberry Pi + Flask | śr. 09.12.2026 |
| 4. Interfejs WWW, testy, dokumentacja | śr. 13.01.2027 |

Źródła prawdy:
- plan i terminy – `docs/PLAN_PROJEKTU.md`,
- sprzęt i piny – `docs/sprzet.md`,
- protokół i API – `docs/protokol.md`.

## Zasada nr 1: nie zgaduj, tylko pytaj

Jeśli czegoś nie wiesz, zatrzymaj się i zapytaj użytkownika. Dotyczy to zwłaszcza:
- parametrów sprzętu: typu czujnika temperatury, wersji ATmega8, modelu konwertera i Raspberry Pi,
- przypisania pinów innego niż w `docs/sprzet.md`,
- adresów IP, nazw i zasad sieci szkolnej,
- terminów i wymagań prowadzącego,
- wyników pomiarów, testów, napotkanych problemów i wniosków.

W dokumentach brakujące dane oznaczaj `[DO UZUPEŁNIENIA]`. Otwarte kwestie są w `docs/PLAN_PROJEKTU.md`, sekcja 10.

## Bezpieczeństwo sprzętu

- **Nigdy nie zapisuj fusebitów ani lock bitów** bez wyraźnej zgody użytkownika w czacie. Makefile celowo ma tylko `make read-fuses`. Taktowanie to fabryczne 1 MHz (RC) – tak zostaje.
- `make flash` i `make check` uruchamiaj tylko na prośbę użytkownika, bo układ może być akurat przebudowywany.
- Układ zasilamy z jednego źródła 5 V. GPIO Raspberry Pi nie łączymy bezpośrednio z ATmega8.

## Komputer szkolny – zero śladów

Komputer z kontem Windows `mikrokontrolery` jest szkolny i współdzielony. Na nim:
- **nie zmieniaj** globalnej konfiguracji gita (`git config --global`), nie uruchamiaj `gh auth setup-git` i nie zapisuj poświadczeń w Menedżerze poświadczeń Windows,
- logowanie do GitHuba i push wyłącznie przez `tools/github-tymczasowo.ps1`: `-Akcja Zaloguj`, potem `-Akcja Wypchnij`,
- **po zakończeniu pracy zawsze** `-Akcja Wyloguj` i sprawdź, że nic nie zostało,
- autor commitów ustawiony jest tylko lokalnie w repozytorium (`git config --local`),
- nie zapisuj pamięci Claude (auto-memory); trwała wiedza trafia do repozytorium (ten plik, `docs/`).

Na prywatnym komputerze domowym te ograniczenia nie obowiązują.

## Konwencje kodu

Ogólne:
- komentarze są minimalne i po polsku, tylko tam, gdzie kod sam nie wyjaśnia sensu; nazwy w kodzie po angielsku,
- UTF-8 i LF (patrz `.editorconfig`, `.gitattributes`); pliki `.ps1` mają CRLF i **tylko znaki ASCII** (Windows PowerShell 5.1 czyta pliki bez BOM jako ANSI).

Firmware (`firmware/`, C):
- kompilacja **bez ostrzeżeń** (`-Wall -Wextra`) na avr-gcc 7.3 (Arduino IDE) **i** WinAVR 4.3.3: standard gnu99, bez cech C11 (np. `_Static_assert`),
- `F_CPU` definiuje tylko Makefile (1 MHz),
- piny wyłącznie według `docs/sprzet.md`, zdefiniowane w jednym miejscu (`firmware/include/pinout.h`, od etapu 1); zmiana pinu = zmiana obu plików,
- zmienne współdzielone z przerwaniami są `volatile`, a wielobajtowe czytamy w `ATOMIC_BLOCK`; w przerwaniach żadnych opóźnień, w pętli głównej żadnych blokujących opóźnień (wyjątek: demonstracja w etapie 1),
- obliczenia całkowitoliczbowe, bez `printf` z float (8 KB flash, 1 KB RAM),
- kod dzielimy na moduły według peryferiów (UART, PWM, ADC, przycisk, protokół).

Protokół: `docs/protokol.md` to jedyne źródło prawdy. Zmiana protokołu oznacza zmianę firmware, serwera, testów i dokumentu w jednym commicie.

Serwer (`server/`, Python):
- Python 3.11+, Flask i pyserial; na Raspberry Pi tylko w venv (bez `sudo pip`),
- port szeregowy obsługuje jeden obiekt z blokadą; Flask korzysta wyłącznie z niego,
- konfiguracja przez zmienne środowiskowe, bez wpisanych na sztywno portów i adresów,
- tryb symulacji musi działać bez sprzętu (Windows i Linux),
- testy `pytest` muszą przechodzić przed commitem dotyczącym serwera,
- wersje zależności przypinamy po pierwszym uruchomieniu na Raspberry Pi (etap 3).

Front-end:
- czysty HTML, CSS i JS, bez frameworków i bez CDN, bo strona musi działać w LAN bez internetu; pliki serwuje Flask,
- interfejs po polsku, czytelny także na telefonie.

## Dokumentacja i sprawozdania

- Sprawozdanie z etapu powstaje jako `docs/sprawozdania/etapN.md` (sekcje 1–6 jak w szablonie). Do `.docx` zamieniamy je poleceniem `tools/md2docx.ps1`; data w nagłówku = termin etapu.
- **Nigdy nie wymyślaj** wyników pomiarów, testów, zrzutów, problemów ani wniosków. Wpisuj tylko dane podane przez zespół.
- Po zmianie planu lub terminów aktualizuj razem `docs/PLAN_PROJEKTU.md`, ten plik i README, a potem wygeneruj ponownie `docs/PLAN_PROJEKTU.docx`.
- Materiały prowadzącego (`docs/materialy/`) są tylko lokalne (`.gitignore`) – nie publikujemy ich.
- Repozytorium jest publiczne: nie umieszczaj w nim adresów IP ani nazw sieci szkolnej, haseł, tokenów ani danych osobowych poza imionami i nazwiskami autorów.

## Git i GitHub

- Repozytorium: https://github.com/kotoanidev/szkola-zdalne-sterowanie (publiczne, wszelkie prawa zastrzeżone – `LICENSE`).
- Autor commitów: adres noreply GitHuba (`317747506+kotoanidev@users.noreply.github.com`), żeby prywatny e-mail nie był publiczny.
- `main` zawsze działa. Prace prowadzimy na gałęziach `etapN-krotki-opis`, następnie PR z listą kontrolną, przegląd Wiktora i merge.
- Commity po polsku, krótko, w trybie rozkazującym (np. „Dodaj obsługę UART”).
- Przed commitem: `make` w `firmware/` bez ostrzeżeń, a od etapu 3 także `pytest` w `server/`.
- Commit i push tylko na prośbę użytkownika.

## Polecenia

```
cd firmware
make             # kompilacja -> build/firmware.hex
make flash       # wgranie przez USBasp
make check       # test programatora (sygnatura)
make read-fuses  # odczyt fusebitów (tylko odczyt)
make toolchain   # który avr-gcc / avrdude jest używany

powershell -NoProfile -ExecutionPolicy Bypass -File tools\md2docx.ps1 docs\sprawozdania\etap1.md
powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Zaloguj
powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wypchnij
powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wyloguj
```

Komputer szkolny (stan na 28.10.2026):
- jest WinAVR 20100110 (make 3.81, avr-gcc 4.3.3, avrdude 5.10) w PATH, Arduino IDE (avr-gcc 7.3, avrdude 8.0) i Git,
- nie ma Pythona, Worda ani zainstalowanego `gh`; `gh` uruchamiamy tylko tymczasowo skryptem,
- Makefile sam wybiera toolchain z Arduino IDE, a gdy go nie ma, korzysta z PATH.
