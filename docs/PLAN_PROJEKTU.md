---
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 0 – przygotowanie
nazwa: Plan projektu i harmonogram prac
data: 28.10.2026
---

# Plan projektu i harmonogram prac

## 1. Temat i cel projektu

**Temat:** Zdalny system sterowania urządzeniem niskonapięciowym przez sieć LAN (mikrokontroler ATmega8 + serwer WWW na Raspberry Pi).

**Cel (wg instrukcji):** zaprojektować, uruchomić i przetestować układ zdalnego sterowania z poziomu przeglądarki internetowej w szkolnej sieci LAN.

**Nasz wariant – wersja zaawansowana:** inteligentne oświetlenie LED sterowane z przeglądarki. Oprócz wymaganego włączania i wyłączania system:
- reguluje jasność (PWM 0–100 %) z płynnym przejściem,
- ma sterowanie lokalne przyciskiem i potencjometrem, a zmiany lokalne widać od razu na stronie,
- w trybie AUTO dobiera jasność do oświetlenia otoczenia (fotorezystor),
- mierzy temperaturę i sygnalizuje przekroczenie progu,
- pokazuje stan na żywo, a po zaniku zasilania sam wraca do ostatniego stanu.

## 2. Zespół i podział ról

| Osoba | Rola | Zakres |
|---|---|---|
| Mikołaj Milcarz | Wykonawca – wszystkie role techniczne z instrukcji | firmware C (ATmega8), serwer Python/Flask na Raspberry Pi, interfejs WWW (HTML/JS), montaż układu |
| Wiktor Selwon | Przegląd, weryfikacja, dokumentacja | przegląd kodu (Pull Requesty), testy wg scenariuszy, pomiary, dokumentacja techniczna, sprawozdania z etapów |

Role z instrukcji: Programista Embedded i Specjalista Sieciowo-Serwerowy – Mikołaj; Front-End – Mikołaj; Dokumentator – Wiktor.

## 3. Harmonogram i terminy

Wszystkie terminy przypadają w środy. Przegląd Wiktora to proponowany wewnętrzny termin kontrolny, dwa dni przed oddaniem etapu.

| Etap | Okres prac | Termin oddania | Przegląd (propozycja) | Weryfikacja wg instrukcji |
|---|---|---|---|---|
| 1. Układ bazowy – ATmega8 + GPIO | 28.10 – 04.11.2026 (1 tydzień) | **śr. 04.11.2026** | pn. 02.11.2026 | miganie / włączanie diody na rozkaz z kodu |
| 2. Komunikacja szeregowa UART | 05.11 – 18.11.2026 (2 tygodnie) | **śr. 18.11.2026** | pn. 16.11.2026 | dioda reaguje na znaki wysyłane z klawiatury komputera |
| 3. Mostek sieciowy – Raspberry Pi + Flask | 19.11 – 09.12.2026 (3 tygodnie) | **śr. 09.12.2026** | pn. 07.12.2026 | `http://[IP_MALINY]:5000/on` zapala diodę |
| 4. Interfejs WWW, testy, dokumentacja | 10.12.2026 – 13.01.2027 (5 tygodni) | **śr. 13.01.2027** | pn. 11.01.2027 | pełna demonstracja działania dla prowadzącego |

Dni wolne w trakcie etapów: **11.11.2026** (Narodowe Święto Niepodległości – etap 2), **23–31.12.2026** (zimowa przerwa świąteczna), **01.01.2027** i **06.01.2027** (etap 4).

Po każdym etapie oddajemy osobne sprawozdanie według szkolnego szablonu (przygotowuje Wiktor). Data sprawozdania = termin etapu.

## 4. Zakres prac w etapach

### Etap 1 – układ bazowy (do 04.11.2026)

Minimum z instrukcji:
- program w C dla ATmega8, konfiguracja rejestrów DDRx i PORTx,
- dioda LED i przycisk, sterowanie lokalne, wgrywanie przez USBasp,
- weryfikacja: miganie / włączanie diody na rozkaz z kodu.

Zadania:
- [M] montaż na płytce stykowej: zasilanie, złącze ISP, dioda główna (PB1), dioda statusu (PD7), przycisk (PD2),
- [M] test programatora `make check` i odczyt fusebitów `make read-fuses` (potwierdzenie fabrycznego 1 MHz),
- [M] miganie diody, włączanie i wyłączanie przyciskiem,
- [M] rozszerzenia R1 (PWM + płynne ściemnianie) i R2 (debouncing na timerze) – po zrealizowaniu minimum,
- [W] przegląd kodu, test wg scenariusza, zdjęcie układu, sprawozdanie z etapu 1.

### Etap 2 – komunikacja szeregowa UART (do 18.11.2026)

Minimum z instrukcji:
- konfiguracja UART w ATmega8, odbiór znaków z zewnątrz,
- konwerter USB-UART i komendy z terminala: `1` – włącz, `0` – wyłącz,
- weryfikacja: dioda reaguje na znaki wysyłane z klawiatury komputera.

Zadania:
- [M] UART 9600 bps 8N1 (U2X), test echa, obsługa znaków `1` / `0`,
- [M] R3: odbiór na przerwaniach z buforem kołowym, komendy tekstowe, odpowiedzi i zdarzenia (docs/protokol.md),
- [M] R4: pomiary ADC (potencjometr, fotorezystor, temperatura) w odpowiedzi na komendę `GET`,
- [M] R6: watchdog oraz zapis ostatniego stanu w EEPROM,
- [W] testy komend z terminala (np. Serial Monitor z Arduino IDE), tabela wyników, sprawozdanie z etapu 2.

### Etap 3 – mostek sieciowy i serwer WWW (do 09.12.2026)

Minimum z instrukcji:
- Raspberry Pi w sieci LAN z ustalonym adresem IP,
- skrypt Python (Flask + pyserial) z adresami `/on` i `/off`,
- połączenie: Raspberry Pi (USB) → konwerter USB-UART → ATmega8,
- weryfikacja: adres `http://[IP_MALINY]:5000/on` w przeglądarce innego komputera zapala diodę.

Zadania:
- [M] Raspberry Pi: system, SSH, nazwa hosta, adres IP (zgodnie z zasadami sieci szkolnej), uprawnienia do portu szeregowego, środowisko venv,
- [M] serwer Flask: `/on`, `/off`, API JSON (`/api/state`, `/api/health`),
- [M] R7: automatyczne wykrywanie konwertera, ponowne łączenie po odłączeniu, osobny wątek odczytu zdarzeń,
- [M] R8: tryb symulacji (praca bez sprzętu) i testy automatyczne pytest,
- [M] R10: autostart serwera (systemd),
- [W] test z innego komputera w LAN, przegląd kodu, sprawozdanie z etapu 3.

### Etap 4 – interfejs użytkownika, testy i dokumentacja (do 13.01.2027)

Minimum z instrukcji:
- czytelna strona HTML z przyciskami (Włącz / Wyłącz / Stan urządzenia),
- testy stabilności, pomiary prądów i napięć, zgodność z dokumentacją techniczną,
- weryfikacja: pełna demonstracja działania systemu dla prowadzącego.

Zadania:
- [M] R9: strona WWW na żywo (SSE): przełącznik, suwak jasności, tryb AUTO, odczyty pomiarów, stan połączenia, dziennik zdarzeń; działa także na telefonie,
- [M] R5: tryb AUTO (jasność wg fotorezystora), R11: alarm temperatury,
- [W] testy stabilności: seria przełączeń, praca ciągła, odłączenie USB, reset mikrokontrolera, zanik zasilania,
- [W] pomiary: napięcie zasilania, prąd diody dla PWM 0 / 50 / 100 %, prąd całkowity układu, poziomy sygnałów UART,
- [W] dokumentacja techniczna, sprawozdanie z etapu 4, próba generalna pokazu.

Oznaczenia: [M] – Mikołaj, [W] – Wiktor.

## 5. Rozszerzenia (wariant zaawansowany)

| Nr | Rozszerzenie | Etap | Priorytet |
|---|---|---|---|
| R1 | PWM jasności 0–100 % z płynnym przejściem (Timer1) | 1 | wysoki |
| R2 | Programowy debouncing przycisku (takt 1 ms, Timer2) | 1 | wysoki |
| R3 | UART na przerwaniach, bufor kołowy, komendy z odpowiedziami i zdarzeniami | 2 | wysoki |
| R4 | Pomiary ADC: potencjometr, fotorezystor, temperatura | 2 | średni |
| R5 | Tryb AUTO – jasność zależna od oświetlenia | 4 | średni |
| R6 | Watchdog i przywracanie stanu z EEPROM po zaniku zasilania | 2 | wysoki |
| R7 | Serwer: wykrywanie konwertera, ponowne łączenie, API JSON | 3 | wysoki |
| R8 | Tryb symulacji bez sprzętu + testy automatyczne (pytest) | 3 | średni |
| R9 | Interfejs WWW na żywo (SSE), responsywny | 4 | wysoki |
| R10 | Autostart serwera na Raspberry Pi (systemd) | 3 | średni |
| R11 | Alarm przekroczenia temperatury | 4 | niski |

Zasada: najpierw minimum z instrukcji, potem rozszerzenia według priorytetu. Rozszerzenie, które zagraża terminowi, przesuwamy do kolejnego etapu.

## 6. Wykaz urządzeń, narzędzi i oprogramowania

**Sprzęt:** ATmega8 (DIP-28), programator USBasp, konwerter USB-UART (CH340 / CP2102 / FT232RL / PL2303 – wersja uniwersalna), Raspberry Pi 4 lub 5, płytka stykowa, diody LED z rezystorami 330 Ω, przycisk, potencjometr 10 kΩ, fotorezystor z rezystorem 10 kΩ, czujnik temperatury (typ do ustalenia), kondensatory 100 nF, multimetr. Szczegóły: docs/sprzet.md.

**Oprogramowanie:**
- avr-gcc 7.3 i avrdude 8.0 (z Arduino IDE); zapasowo WinAVR 20100110 (avr-gcc 4.3.3, avrdude 5.10),
- GNU Make (kompilacja i wgrywanie: `make`, `make flash`),
- Git i GitHub (repozytorium `szkola-zdalne-sterowanie`),
- Raspberry Pi OS, Python 3, Flask, pyserial, pytest,
- HTML, CSS i JavaScript bez zewnętrznych bibliotek (strona działa bez internetu),
- terminal szeregowy: Serial Monitor z Arduino IDE.

## 7. Architektura systemu (opis ideowy)

```
 Przeglądarka (komputer / telefon w szkolnej sieci LAN)
        |  HTTP :5000  - strona WWW, API JSON, zdarzenia SSE
        v
 Raspberry Pi 4/5  - Python + Flask + pyserial
        |  USB
        v
 Konwerter USB-UART  (CH340 / CP2102 / FT232 / PL2303)
        |  UART 9600 bps 8N1 (TXD/RXD na krzyż, wspólna masa)
        v
 ATmega8 @ 1 MHz (wewnętrzny RC)
        +-- PB1 (PWM) --> dioda LED - kanał główny
        +-- PD7       --> dioda statusu
        +-- PD2       <-- przycisk
        +-- ADC0..2   <-- potencjometr, fotorezystor, czujnik temperatury
```

Przepływ: kliknięcie na stronie → żądanie HTTP do Flaska → komenda tekstowa przez port szeregowy → ATmega8 zmienia wyjście i odsyła odpowiedź → serwer aktualizuje stan i rozsyła go do przeglądarek. Zmiany lokalne (przycisk, potencjometr) idą w drugą stronę jako zdarzenia `EVT`.

## 8. Ryzyka i sposoby ograniczenia

| Ryzyko | Skutek | Jak ograniczamy |
|---|---|---|
| Niedokładny oscylator RC 1 MHz | błędne znaki na UART | U2X i 9600 bps, w razie potrzeby korekta OSCCAL; bez zmiany fusebitów |
| Za szybki zegar ISP USBasp dla 1 MHz | programator nie widzi układu | `-B 8` w Makefile lub zworka slow SCK |
| Sterownik USBasp lub brak uprawnień administratora na komputerze szkolnym | nie da się wgrać programu | `make check` na pierwszych zajęciach, problem zgłaszamy prowadzącemu |
| Zasady sieci szkolnej (izolacja klientów, blokada portu 5000, zmienne IP) | brak dostępu do Raspberry Pi z innego komputera | ustalić z prowadzącym przed etapem 3; rezerwacja DHCP lub nazwa hosta |
| Różne poziomy logiczne konwerterów (3,3 V / 5 V) | błędy transmisji lub uszkodzenie konwertera | tryb 5 V albo dzielnik na linii TXD (docs/sprzet.md) |
| Dwa źródła zasilania jednocześnie | uszkodzenie portu USB | zasilanie zawsze z jednego źródła |
| Jedna osoba implementuje całość | opóźnienia | priorytety rozszerzeń, minimum ma pierwszeństwo, przegląd dwa dni przed terminem |
| Etap 1 trwa tylko 7 dni | niedotrzymanie terminu | najpierw miganie i przycisk, R1–R2 dopiero potem |
| Dni wolne w etapach 2 i 4 | mniej czasu na pracę ze sprzętem w szkole | w domu praca na symulatorze, sprzęt testowany w pracowni |

## 9. Zasady pracy zespołu

- Kod w repozytorium GitHub. Gałąź `main` zawsze działa, prace prowadzimy na gałęziach etapów.
- Każda większa zmiana przechodzi przez Pull Request z przeglądem Wiktora (lista kontrolna w szablonie PR).
- Komentarze w kodzie są krótkie i po polsku, a opis wszystkiego jest w README i w docs/.
- Pracujemy na dwóch komputerach: szkolnym i domowym. Na komputerze szkolnym po pracy nie zostają żadne loginy (skrypt `tools/github-tymczasowo.ps1`).
- Sprawozdania piszemy w Markdown (docs/sprawozdania/) i konwertujemy do .docx szkolnego szablonu skryptem `tools/md2docx.ps1`.
- Wyniki pomiarów i testów wpisujemy wyłącznie z rzeczywistych prób.

Etap uznajemy za ukończony, gdy:
- weryfikacja z instrukcji jest spełniona i pokazana prowadzącemu,
- kod jest w `main`, kompiluje się bez ostrzeżeń, a testy przechodzą,
- przegląd Wiktora jest zakończony,
- sprawozdanie z etapu jest gotowe.

## 10. Kwestie do ustalenia

- Typ czujnika temperatury (LM35 / NTC / DS18B20).
- Dokładna wersja mikrokontrolera (ATmega8 / 8A / 8L).
- Model Raspberry Pi (4 czy 5) i wersja systemu.
- Zasady sieci szkolnej: sposób nadania adresu IP, dostępność portu 5000, ewentualna izolacja urządzeń.
- Sterownik USBasp na komputerze szkolnym i to, który avrdude (8.0 czy 5.10) z nim współpracuje.
- Forma i termin oddawania sprawozdań (wydruk / plik; czy w dniu terminu etapu).
