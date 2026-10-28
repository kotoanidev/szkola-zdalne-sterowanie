---
zespol: Mikołaj Milcarz, Wiktor Selwon
etap: 1
nazwa: Układ bazowy i sterowanie lokalne (ATmega8 + GPIO)
data: 04.11.2026
---

# Sprawozdanie – etap 1

<!-- Szkic. Wpisujemy tylko to, co faktycznie zrobiono i zmierzono. Konwersja: tools\md2docx.ps1 -->

## 1. Cel etapu

Celem etapu było zapoznanie się ze środowiskiem programistycznym mikrokontrolera ATmega8, konfiguracja portów wejścia/wyjścia (rejestry DDRx i PORTx), podłączenie diody LED i przycisku, wgranie programu programatorem USBasp oraz lokalne sterowanie diodą.

## 2. Wykaz zastosowanych urządzeń, narzędzi i oprogramowania

- **Sprzęt:**
  - mikrokontroler ATmega8 [DO UZUPEŁNIENIA: wersja], programator USBasp
  - płytka stykowa, dioda LED z rezystorem 330 Ω (PB1), dioda statusu z rezystorem 330 Ω (PD7), przycisk (PD2)
- **Środowisko:**
  - avr-gcc [DO UZUPEŁNIENIA: wersja], avrdude [DO UZUPEŁNIENIA: wersja], GNU Make, Git

## 3. Schemat połączeń / opis ideowy

<!-- Schemat lub zdjęcie układu (wstawić w Wordzie) + tabela faktycznie użytych pinów z docs/sprzet.md. -->

[DO UZUPEŁNIENIA]

## 4. Kluczowy fragment kodu / Konfiguracja

<!-- Najważniejszy fragment: konfiguracja DDRx/PORTx i pętla główna. -->

```c
[DO UZUPEŁNIENIA]
```

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** [DO UZUPEŁNIENIA]
- **Napotkane trudności i sposób ich rozwiązania:** [DO UZUPEŁNIENIA]

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** miganie / włączanie diody na rozkaz z kodu.

**Wynik weryfikacji:** [DO UZUPEŁNIENIA]

**Wnioski:** [DO UZUPEŁNIENIA]
