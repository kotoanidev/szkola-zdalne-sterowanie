---
tytul: WNIOSEK
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 0 – przygotowanie
nazwa: Wniosek o realizację projektu
data: 28.10.2026
---

# Wniosek o realizację projektu

<!-- Konwersja: tools\md2docx.ps1 docs\WNIOSEK.md -> docs\WNIOSEK.docx -->

## 1. Dane projektu

| Pole | Wartość |
|---|---|
| Przedmiot | Pracownia Urządzeń Techniki Komputerowej |
| Temat | Zdalny system sterowania urządzeniem niskonapięciowym przez sieć LAN (mikrokontroler ATmega8 + serwer WWW na Raspberry Pi) |
| Wariant | zaawansowany – inteligentne oświetlenie LED sterowane z przeglądarki |
| Zespół | Mikołaj Milcarz, Wiktor Selwon |
| Data złożenia wniosku | 28.10.2026 |
| Rozpoczęcie prac | 28.10.2026 |
| Zakończenie prac | 13.01.2027 (oddanie etapu 4) |
| Repozytorium kodu | https://github.com/kotoanidev/szkola-zdalne-sterowanie |

## 2. Cel projektu

Zaprojektować, uruchomić i przetestować układ zdalnego sterowania urządzeniem niskonapięciowym z poziomu przeglądarki internetowej w szkolnej sieci LAN.

W wariancie zaawansowanym, oprócz wymaganego włączania i wyłączania, system:
- reguluje jasność diody (PWM 0–100 %) z płynnym przejściem,
- ma sterowanie lokalne przyciskiem i potencjometrem, a zmiany lokalne widać od razu na stronie,
- w trybie AUTO dobiera jasność do oświetlenia otoczenia (fotorezystor),
- mierzy temperaturę i sygnalizuje przekroczenie progu,
- pokazuje stan na żywo, a po zaniku zasilania sam wraca do ostatniego stanu.

## 3. Zespół i podział ról

| Osoba | Rola | Zakres |
|---|---|---|
| Mikołaj Milcarz | Programista Embedded, Specjalista Sieciowo-Serwerowy, Front-End | firmware w C (ATmega8), serwer Python/Flask na Raspberry Pi, interfejs WWW, montaż układu |
| Wiktor Selwon | Dokumentator | przegląd kodu, testy według scenariuszy, pomiary, dokumentacja techniczna, sprawozdania z etapów |

## 4. Harmonogram

| Etap | Zakres | Okres prac | Termin oddania | Weryfikacja |
|---|---|---|---|---|
| 1 | Układ bazowy: ATmega8 + GPIO | 28.10 – 04.11.2026 | 04.11.2026 | miganie / włączanie diody na rozkaz z kodu |
| 2 | Komunikacja szeregowa UART | 05.11 – 18.11.2026 | 18.11.2026 | dioda reaguje na znaki wysyłane z klawiatury komputera |
| 3 | Mostek sieciowy: Raspberry Pi + Flask | 19.11 – 09.12.2026 | 09.12.2026 | `http://[IP_MALINY]:5000/on` zapala diodę |
| 4 | Interfejs WWW, testy, dokumentacja | 10.12.2026 – 13.01.2027 | 13.01.2027 | pełna demonstracja działania dla prowadzącego |

Po każdym etapie zespół oddaje sprawozdanie według szkolnego szablonu.

## 5. Języki programowania

| Część systemu | Język | Biblioteki i technologie |
|---|---|---|
| Mikrokontroler ATmega8 | C (standard gnu99) | avr-libc, obliczenia całkowitoliczbowe |
| Serwer na Raspberry Pi | Python 3.11+ | Flask, pyserial, pytest |
| Interfejs użytkownika | HTML, CSS, JavaScript | bez frameworków i zewnętrznych bibliotek – strona działa w LAN bez internetu |
| Dokumentacja | Markdown | konwersja do DOCX na szkolnym szablonie |

## 6. Narzędzia i oprogramowanie

- kompilator avr-gcc 7.3 i programator avrdude 8.0 (z Arduino IDE); zapasowo WinAVR 20100110,
- GNU Make – kompilacja i wgrywanie programu (`make`, `make flash`),
- Git i GitHub – kontrola wersji, przegląd kodu przez Pull Requesty,
- Raspberry Pi OS, Python 3 w środowisku venv, usługa systemd (autostart serwera),
- terminal szeregowy: Serial Monitor z Arduino IDE,
- symulatory do pracy bez sprzętu: simavr (uruchamia program ATmega8) i symulator mikrokontrolera w Pythonie,
- przeglądarka internetowa na komputerze i telefonie (interfejs WWW).

## 7. Sprzęt

| Element | Uwagi |
|---|---|
| Mikrokontroler ATmega8 (DIP-28) | wersja [DO UZUPEŁNIENIA: 8 / 8A / 8L]; fabryczny oscylator RC 1 MHz |
| Programator USBasp | programowanie przez złącze ISP |
| Konwerter USB-UART | CH340 / CP2102 / FT232RL / PL2303, UART 9600 bps 8N1 |
| Raspberry Pi | model [DO UZUPEŁNIENIA: 4 lub 5], Raspberry Pi OS |
| Diody LED z rezystorami 330 Ω | dioda główna (PWM) i dioda statusu |
| Przycisk, potencjometr 10 kΩ, fotorezystor z rezystorem 10 kΩ | sterowanie lokalne i tryb AUTO |
| Czujnik temperatury | typ [DO UZUPEŁNIENIA: LM35 lub NTC 10 kΩ] |
| Płytka stykowa, przewody, kondensatory 100 nF, multimetr | montaż i pomiary |

Układ zasilany jest napięciem 5 V z jednego źródła. Raspberry Pi łączy się z ATmega8 wyłącznie przez konwerter USB-UART.

## 8. Zakres prac

Najpierw realizujemy minimum z instrukcji (sterowanie diodą z kodu, komendy `1` / `0` przez UART, adresy `/on` i `/off` na serwerze, strona WWW z przyciskami Włącz / Wyłącz / Stan), a następnie rozszerzenia:

| Nr | Rozszerzenie | Etap |
|---|---|---|
| R1 | PWM jasności 0–100 % z płynnym przejściem (Timer1) | 1 |
| R2 | Programowy debouncing przycisku (takt 1 ms, Timer2) | 1 |
| R3 | UART na przerwaniach, bufor kołowy, komendy z odpowiedziami i zdarzeniami | 2 |
| R4 | Pomiary ADC: potencjometr, fotorezystor, temperatura | 2 |
| R5 | Tryb AUTO – jasność zależna od oświetlenia | 4 |
| R6 | Watchdog i przywracanie stanu z EEPROM po zaniku zasilania | 2 |
| R7 | Serwer: wykrywanie konwertera, ponowne łączenie, API JSON | 3 |
| R8 | Tryb symulacji bez sprzętu i testy automatyczne (pytest) | 3 |
| R9 | Interfejs WWW na żywo (SSE), responsywny | 4 |
| R10 | Autostart serwera na Raspberry Pi (systemd) | 3 |
| R11 | Alarm przekroczenia temperatury | 4 |

Rozszerzenie, które zagraża terminowi etapu, przesuwamy do kolejnego etapu.

## 9. Sposób realizacji i dokumentowania

- Kod przechowujemy w repozytorium GitHub; każda większa zmiana przechodzi przegląd drugiej osoby.
- Prace prowadzimy na dwóch komputerach: w pracowni (wgrywanie programu i testy na sprzęcie) i w domu (programowanie i symulacja).
- Po każdym etapie: weryfikacja według instrukcji pokazana prowadzącemu i sprawozdanie.
- W sprawozdaniach i dokumentacji podajemy wyłącznie wyniki rzeczywistych prób i pomiarów.
