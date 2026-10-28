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
  - układ z etapu 1 (ATmega8, USBasp, diody, przycisk)
  - konwerter USB-UART [DO UZUPEŁNIENIA: model / układ, tryb 5 V lub 3,3 V]
- **Środowisko:**
  - avr-gcc, avrdude, GNU Make, terminal szeregowy [DO UZUPEŁNIENIA: np. Serial Monitor Arduino IDE]
  - parametry transmisji: 9600 bps, 8N1

## 3. Schemat połączeń / opis ideowy

<!-- Połączenie konwerter TXD -> PD0, RXD <- PD1, wspólna masa; zasilanie z jednego źródła. -->

[DO UZUPEŁNIENIA]

## 4. Kluczowy fragment kodu / Konfiguracja

<!-- Inicjalizacja UART (U2X, UBRR = 12) i obsługa odbioru. -->

```c
[DO UZUPEŁNIENIA]
```

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** [DO UZUPEŁNIENIA]
- **Napotkane trudności i sposób ich rozwiązania:** [DO UZUPEŁNIENIA]

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** dioda reaguje na znaki wysyłane z klawiatury komputera.

| Wysłano | Oczekiwana reakcja | Wynik |
|---|---|---|
| `1` | dioda włączona, odpowiedź `OK on=1` | [DO UZUPEŁNIENIA] |
| `0` | dioda wyłączona, odpowiedź `OK on=0` | [DO UZUPEŁNIENIA] |

**Wnioski:** [DO UZUPEŁNIENIA]
