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
  - mikrokontroler ATmega8 (DIP-28) [DO UZUPEŁNIENIA: wersja 8 / 8A / 8L], praca na fabrycznym oscylatorze RC 1 MHz,
  - programator USBasp z taśmą ISP,
  - płytka stykowa z przewodami,
  - dioda LED (kanał główny) z rezystorem 330 Ω (PB1) i dioda statusu z rezystorem 330 Ω (PD7),
  - przycisk (tact switch) do masy (PD2), z wewnętrznym rezystorem podciągającym,
  - rezystor 10 kΩ (RESET → VCC) i kondensator 100 nF (VCC–GND).
- **Środowisko:**
  - kompilator avr-gcc [DO UZUPEŁNIENIA: wersja użyta przy kompilacji, wynik `make toolchain`; Makefile wybiera najpierw avr-gcc 7.3 z Arduino IDE, a gdy go brak – WinAVR 4.3.3 z PATH],
  - avrdude [DO UZUPEŁNIENIA: wersja; dostępne 8.0 z Arduino IDE lub 5.10 z WinAVR],
  - GNU Make, Git,
  - program: standard gnu99, flagi `-Os -Wall -Wextra`, `F_CPU = 1000000UL`.

## 3. Schemat połączeń / opis ideowy

<!-- Schemat lub zdjęcie układu wstawić w Wordzie. Tabela poniżej wynika z docs/sprzet.md i firmware/include/pinout.h. -->

[DO UZUPEŁNIENIA: schemat lub zdjęcie układu]

ATmega8 zasilana jest napięciem 5 V z jednego źródła (z USBasp, zworka zasilania). Piny ISP (PB3–PB5, RESET) nie są używane do peryferiów, dzięki czemu programowanie działa bez rozłączania układu. Przycisk zwiera wejście PD2 do masy, a stan wysoki zapewnia wewnętrzny pull-up.

| Pin | Port / funkcja | Podłączenie |
|---|---|---|
| 1 | PC6 / RESET | rezystor 10 kΩ do VCC, linia RESET złącza ISP |
| 4 | PD2 | przycisk do GND (pull-up wewnętrzny) |
| 7 | VCC | +5 V, kondensator 100 nF do GND |
| 8, 22 | GND | masa wspólna |
| 13 | PD7 | dioda statusu + rezystor 330 Ω |
| 15 | PB1 / OC1A | dioda główna + rezystor 330 Ω |
| 17, 18, 19 | PB3 / PB4 / PB5 | tylko ISP (MOSI, MISO, SCK) |

[DO UZUPEŁNIENIA: potwierdzenie, że rzeczywisty montaż jest zgodny z tabelą; ewentualne odstępstwa]

## 4. Kluczowy fragment kodu / Konfiguracja

Program demonstracyjny etapu 1 (`firmware/demo/etap1.c`, kompilacja `make demo`, wgranie `make flash-demo`). Używa wyłącznie rejestrów DDRx, PORTx i PINx. Piny zdefiniowane są w `firmware/include/pinout.h`.

```c
// Konfiguracja portów
LED_DDR |= _BV(LED_BIT);        // PB1 jako wyjście (dioda główna)
STATUS_DDR |= _BV(STATUS_BIT);  // PD7 jako wyjście (dioda statusu)
BTN_DDR &= ~_BV(BTN_BIT);       // PD2 jako wejście
BTN_PORT |= _BV(BTN_BIT);       // włączenie pull-upu

// Na start trzy mignięcia diody głównej
for (i = 0; i < 6; i++) {
    LED_PORT ^= _BV(LED_BIT);
    _delay_ms(250);
}

// Pętla główna: wykrycie zbocza opadającego przycisku przełącza diodę
for (i = 0;; i++) {
    uint8_t now = (BTN_PIN & _BV(BTN_BIT)) ? 1 : 0;

    if (prev && !now) {
        on = !on;
        if (on)
            LED_PORT |= _BV(LED_BIT);
        else
            LED_PORT &= ~_BV(LED_BIT);
    }
    prev = now;

    // Dioda statusu miga co 0,5 s, pętla co 10 ms (opóźnienie tłumi drgania styków)
    if (i == 50) {
        i = 0;
        STATUS_PORT ^= _BV(STATUS_BIT);
    }
    _delay_ms(10);
}
```

Działanie programu:
- po uruchomieniu dioda główna miga trzy razy (6 przełączeń co 250 ms),
- każde naciśnięcie przycisku przełącza diodę główną (włączona / wyłączona),
- dioda statusu zmienia stan co 0,5 s, co potwierdza działanie pętli głównej,
- drgania styków są tłumione przez 10-milisekundowy krok pętli.

## 5. Przebieg prac i napotkane problemy

- **Co poszło zgodnie z planem:** [DO UZUPEŁNIENIA]
- **Napotkane trudności i sposób ich rozwiązania:** [DO UZUPEŁNIENIA]

<!-- Wpisać tylko to, co faktycznie się wydarzyło: np. wynik `make check` (sygnatura układu), odczyt fusebitów `make read-fuses` (oczekiwane fabryczne lfuse = 0xE1, hfuse = 0xD9), ewentualne problemy z montażem lub programatorem. -->

## 6. Wnioski i weryfikacja działania

**Kryterium weryfikacji (instrukcja):** miganie / włączanie diody na rozkaz z kodu.

| Test | Oczekiwany wynik | Wynik |
|---|---|---|
| Wgranie programu (`make flash-demo`) | wgranie bez błędów | [DO UZUPEŁNIENIA] |
| Reset / włączenie zasilania | dioda główna miga trzy razy | [DO UZUPEŁNIENIA] |
| Praca ciągła | dioda statusu zmienia stan co ok. 0,5 s | [DO UZUPEŁNIENIA] |
| Naciśnięcie przycisku | dioda główna zmienia stan na przeciwny | [DO UZUPEŁNIENIA] |
| Kolejne naciśnięcie | dioda główna wraca do poprzedniego stanu | [DO UZUPEŁNIENIA] |

**Wynik weryfikacji:** [DO UZUPEŁNIENIA]

**Wnioski:** [DO UZUPEŁNIENIA]
