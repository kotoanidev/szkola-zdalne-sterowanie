---
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 3
nazwa: Integracja z bramką sieciową (Raspberry Pi + serwer WWW)
data: 09.12.2026
---

# Sprawozdanie – etap 3

<!-- Szkic. Wpisujemy tylko to, co faktycznie zrobiono i zmierzono. Nie podajemy prawdziwych adresów IP sieci szkolnej w repozytorium. -->

## 1. Cel etapu

Celem etapu było uruchomienie serwera sieciowego na Raspberry Pi (Python, Flask, pyserial), fizyczne połączenie Raspberry Pi z konwerterem USB-UART i mikrokontrolerem oraz zdalne sterowanie urządzeniem żądaniami HTTP z dowolnego komputera w szkolnej sieci LAN.

## 2. Wykaz zastosowanych urządzeń, narzędzi i oprogramowania

- **Sprzęt:**
  - układ z etapów 1–2, konwerter USB-UART
  - Raspberry Pi [DO UZUPEŁNIENIA: model], karta microSD, zasilacz
- **Środowisko:**
  - Raspberry Pi OS [DO UZUPEŁNIENIA: wersja], Python [DO UZUPEŁNIENIA: wersja], Flask, pyserial

## 3. Schemat połączeń / opis ideowy

<!-- Raspberry Pi (USB) -> konwerter USB-UART -> ATmega8; schemat sieci: komputer ucznia -> LAN -> Raspberry Pi :5000. -->

[DO UZUPEŁNIENIA]

## 4. Kluczowy fragment kodu / Konfiguracja

<!-- Obsługa /on i /off oraz zapis do portu szeregowego. -->

```python
[DO UZUPEŁNIENIA]
```

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** [DO UZUPEŁNIENIA]
- **Napotkane trudności i sposób ich rozwiązania:** [DO UZUPEŁNIENIA]

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** wywołanie w przeglądarce komputera ucznia adresu `http://[IP_MALINY]:5000/on` zapala diodę.

| Żądanie | Oczekiwana reakcja | Wynik |
|---|---|---|
| `GET /on` | dioda włączona | [DO UZUPEŁNIENIA] |
| `GET /off` | dioda wyłączona | [DO UZUPEŁNIENIA] |

**Wnioski:** [DO UZUPEŁNIENIA]
