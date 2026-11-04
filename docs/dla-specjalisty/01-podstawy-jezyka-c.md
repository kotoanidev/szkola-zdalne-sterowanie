# Podstawy języka C

[← Poprzedni](00-podstawy-ogolne.md) · [Spis treści](README.md) · [Następny →](02-podstawy-pythona.md)

W języku C napisany jest firmware, czyli program działający w układzie ATmega8, oraz symulator `sim/avrsim.c`. Ten dokument pokazuje wszystkie elementy języka, które w nich występują.

## Znaki, które widać wszędzie

| Zapis | Znaczenie |
|---|---|
| `;` | koniec polecenia, jak kropka w zdaniu |
| `{` i `}` | początek i koniec bloku, czyli grupy poleceń |
| `( )` | parametry funkcji albo grupowanie działań, jak w matematyce |
| `[ ]` | numer elementu w tablicy |
| `// tekst` | komentarz do końca linii, komputer go pomija |

Wcięcia (spacje na początku linii) nie mają w C znaczenia dla komputera. Służą ludziom, żeby było widać, co jest wewnątrz czego.

## Zmienne i typy

**Zmienna** to podpisane pudełko w pamięci, w którym trzymamy wartość. **Typ** mówi, jak duże jest pudełko i co można w nim trzymać.

```c
uint8_t on = 0;
```

To polecenie tworzy zmienną o nazwie `on` i typie `uint8_t`, po czym wkłada do niej 0.

| Typ | Znaczenie | Zakres |
|---|---|---|
| `uint8_t` | liczba całkowita bez znaku, 8 bitów | 0 do 255 |
| `int8_t` | liczba całkowita ze znakiem, 8 bitów | -128 do 127 |
| `uint16_t` | bez znaku, 16 bitów | 0 do 65 535 |
| `int16_t` | ze znakiem, 16 bitów | -32 768 do 32 767 |
| `uint32_t` | bez znaku, 32 bity | 0 do około 4,29 miliarda |
| `uint64_t` | bez znaku, 64 bity | bardzo duże liczby (tylko w symulatorze) |
| `char` | jeden znak tekstu | np. `'A'` |
| `double` | liczba z ułamkiem (tylko w symulatorze) | np. `2.5` |
| `void` | „nic”, brak wartości | – |

Mikrokontroler ma mało pamięci, dlatego dobieramy najmniejszy typ, który wystarczy, i unikamy ułamków.

**Zapis liczb:** `0x5A` to zapis szesnastkowy, a przyrostek `UL` w `1000000UL` każe traktować liczbę jako dużą, bez znaku (*unsigned long*).

## Działania

| Zapis | Znaczenie |
|---|---|
| `a = 5` | **przypisanie**: włóż 5 do `a` |
| `a == 5` | **porównanie**: czy `a` jest równe 5? |
| `a != 5` | czy `a` jest różne od 5? |
| `a < b`, `a <= b`, `a > b`, `a >= b` | mniejsze, mniejsze lub równe, większe, większe lub równe |
| `a + b`, `a - b`, `a * b`, `a / b` | dodawanie, odejmowanie, mnożenie, dzielenie (liczby całkowite dzielone są bez reszty: `7 / 2` to 3) |
| `a % b` | reszta z dzielenia: `7 % 2` to 1 |
| `a += 2` | skrót od `a = a + 2` (podobnie `-=`, `\|=`, `&=`, `^=`) |
| `a++` | zwiększ o 1, wynikiem jest stara wartość |
| `++a` | zwiększ o 1, wynikiem jest nowa wartość |
| `a--`, `--a` | to samo, tylko zmniejszanie |
| `x && y` | logiczne „i”: oba warunki prawdziwe |
| `x \|\| y` | logiczne „lub”: co najmniej jeden warunek prawdziwy |
| `!x` | logiczne „nie”: odwraca prawdę i fałsz |
| `warunek ? a : b` | jeśli warunek jest prawdziwy, wynikiem jest `a`, w przeciwnym razie `b` |

W języku C **0 oznacza fałsz, a każda inna liczba prawdę**. Dlatego `if (x)` znaczy to samo, co „jeśli `x` jest różne od zera”.

Operatory bitowe (`&`, `|`, `^`, `~`, `<<`, `>>`) są opisane w [podstawach ogólnych](00-podstawy-ogolne.md).

**Rzutowanie** to jawna zamiana typu. Zapis `(uint8_t)v` oznacza „potraktuj `v` jak `uint8_t`”. Zapis `(uint32_t)x * y` wymusza liczenie na 32 bitach, żeby duży wynik zmieścił się w pudełku.

## Instrukcje warunkowe

```c
if (temperatura > 50) {
    // to wykona się, gdy warunek jest prawdziwy
} else if (temperatura > 30) {
    // to wykona się, gdy pierwszy warunek jest fałszywy, a drugi prawdziwy
} else {
    // to wykona się w pozostałych przypadkach
}
```

Jeśli do wykonania jest tylko jedno polecenie, klamry można pominąć. Tego skrótu firmware używa bardzo często.

## Pętle

Pętla powtarza fragment kodu.

```c
while (warunek) { ... }            // powtarzaj, dopóki warunek jest prawdziwy
do { ... } while (warunek);        // to samo, ale sprawdzaj dopiero po wykonaniu, więc co najmniej raz
for (i = 0; i < 6; i++) { ... }    // i = 0, 1, 2, 3, 4, 5, czyli sześć powtórzeń
for (;;) { ... }                   // powtarzaj bez końca
```

Pętla `for` ma trzy części rozdzielone średnikami: co zrobić na początku; jaki warunek sprawdzać przed każdym obrotem; co zrobić po każdym obrocie.

- `break` natychmiast kończy pętlę,
- `continue` przeskakuje do następnego obrotu.

## Funkcje

**Funkcja** to nazwany fragment kodu, który można uruchamiać wiele razy, jak przepis w książce kucharskiej.

```c
uint8_t dodaj(uint8_t a, uint8_t b)
{
    return a + b;
}
```

- `uint8_t` przed nazwą: co funkcja zwraca (`void` oznacza, że nic),
- `dodaj`: nazwa funkcji,
- `(uint8_t a, uint8_t b)`: **parametry**, czyli dane wejściowe (`void` oznacza brak parametrów),
- `return`: kończy funkcję i oddaje wynik.

Użycie, czyli **wywołanie**: `x = dodaj(2, 3);`.

Każdy program w C zaczyna się od funkcji `main`.

**Deklaracja** to sama zapowiedź funkcji, zakończona średnikiem: `void pwm_tick(void);`. Mówi kompilatorowi, że taka funkcja istnieje gdzieś indziej. **Definicja** zawiera jej treść w klamrach.

## Tablice i napisy

**Tablica** to ponumerowany rząd pudełek tego samego typu. Numeracja zaczyna się od zera.

```c
uint16_t sums[3];    // trzy pudełka: sums[0], sums[1], sums[2]
```

**Napis** to tablica znaków zakończona specjalnym znakiem `'\0'` (bajtem o wartości 0). Dzięki temu funkcje wiedzą, gdzie napis się kończy. Pojedynczy znak zapisujemy w apostrofach (`'A'`), a napis w cudzysłowach (`"AUTO"`).

Znaki specjalne: `'\n'` nowa linia, `'\r'` powrót karetki, `'\0'` koniec napisu, `'\\'` ukośnik, `'\"'` cudzysłów.

Znaki to liczby, więc można na nich liczyć: `c - '0'` zamienia znak cyfry na jej wartość (`'7' - '0'` to 7).

## Wskaźniki

**Wskaźnik** to zmienna, która przechowuje nie wartość, tylko **adres**, czyli numer miejsca w pamięci. To jak kartka z adresem domu zamiast samego domu.

| Zapis | Znaczenie |
|---|---|
| `char *c` | `c` jest wskaźnikiem na znak |
| `&x` | adres zmiennej `x` |
| `*c` | wartość pod adresem `c` |
| `s++` | przesuń wskaźnik na następny element |
| `s->pole` | pole struktury, na którą wskazuje `s` |
| `NULL` | wskaźnik, który nie wskazuje na nic |

Funkcja może dzięki temu zmienić zmienną należącą do tego, kto ją wywołał: dostaje adres i wpisuje wartość pod ten adres. Tak działa `uart_read(&c)`.

## Struktury i wyliczenia

**Struktura** łączy kilka zmiennych w jedną całość, jak formularz z polami. Słowo `typedef` nadaje jej krótką nazwę.

```c
typedef struct {
    uint8_t on;
    uint8_t pwm;
} state_t;

state_t state;     // jedna zmienna typu state_t
state.on = 1;      // kropka: dostęp do pola
```

**Wyliczenie** (`enum`) nadaje nazwy kolejnym liczbom, zaczynając od 0.

```c
enum { MODE_MANUAL, MODE_AUTO };   // MODE_MANUAL = 0, MODE_AUTO = 1
```

## Słowa dodatkowe przy zmiennych i funkcjach

| Słowo | Znaczenie |
|---|---|
| `static` | poza funkcją: nazwa jest widoczna tylko w tym pliku. Zmienna `static` żyje przez cały czas działania programu i zaczyna od 0 |
| `extern` | ta zmienna istnieje, ale jest utworzona w innym pliku |
| `const` | tej wartości nie wolno zmieniać |
| `volatile` | ta zmienna może się zmienić „za plecami” kodu, np. w przerwaniu, więc kompilator ma ją za każdym razem odczytywać z pamięci |

## Preprocesor: linie zaczynające się od `#`

**Preprocesor** działa przed właściwą kompilacją i edytuje tekst programu.

| Zapis | Znaczenie |
|---|---|
| `#include <plik.h>` | wklej tu zawartość pliku z biblioteki systemowej |
| `#include "plik.h"` | wklej tu zawartość pliku z naszego projektu |
| `#define NAZWA 123` | wszędzie dalej zamień tekst `NAZWA` na `123` (to tzw. **makro**) |
| `#ifndef NAZWA` | dalszy tekst bierz pod uwagę tylko wtedy, gdy `NAZWA` nie jest zdefiniowana |
| `#ifdef NAZWA` | dalszy tekst bierz pod uwagę tylko wtedy, gdy `NAZWA` jest zdefiniowana |
| `#if warunek` | dalszy tekst bierz pod uwagę tylko wtedy, gdy warunek jest prawdziwy |
| `#elif warunek` | w przeciwnym razie, jeśli inny warunek jest prawdziwy |
| `#else` | w przeciwnym razie |
| `#endif` | koniec bloku warunkowego |
| `#error "tekst"` | przerwij kompilację z komunikatem |

Fragmenty wyłączone warunkiem w ogóle nie trafiają do programu.

## Pliki `.h` i `.c`

Kod dzielimy na moduły. Każdy ma dwa pliki:

- `.c` zawiera **treść** funkcji,
- `.h` (nagłówek) zawiera **spis** tego, co moduł udostępnia innym.

Plik `.h` działa jak spis treści: `main.c` dołącza `uart.h` i wie, jakie funkcje oferuje UART, nie znając ich treści.

Każdy nagłówek zaczyna się tzw. **strażnikiem nagłówka**, który chroni przed wklejeniem go dwa razy:

```c
#ifndef UART_H      // jeśli UART_H jeszcze nie istnieje...
#define UART_H      // ...to ją utwórz i wklej treść poniżej
...
#endif              // koniec
```

Za drugim razem nazwa `UART_H` już istnieje, więc cała treść jest pomijana.

## Rzeczy szczególne dla AVR (biblioteka avr-libc)

| Zapis | Znaczenie |
|---|---|
| `_BV(n)` | maska z jedynką na bicie `n`, czyli `(1 << n)` |
| `ISR(nazwa) { ... }` | funkcja obsługi przerwania |
| `sei()` | włącz obsługę przerwań w całym układzie |
| `ATOMIC_BLOCK(...) { ... }` | wykonaj blok bez przerwań, żeby żadne nie weszło w środek |
| `PSTR("tekst")` | zapisz napis w pamięci flash zamiast w RAM |
| `PROGMEM` | umieść stałe w pamięci flash |
| `PGM_P` | typ wskaźnika na napis we flash |
| `pgm_read_byte(a)`, `pgm_read_word(a)` | odczytaj bajt albo 16-bitowe słowo z flash |
| `strcmp_P(a, b)` | porównaj napis `a` z RAM z napisem `b` z flash, 0 oznacza „takie same” |
| `EEMEM` | umieść zmienną w pamięci EEPROM |
| `_delay_ms(n)` | czekaj `n` milisekund, nic innego nie robiąc |

Napisy trzymamy we flash, bo RAM ma tylko 1 KB, a flash aż 8 KB.

---

[← Poprzedni](00-podstawy-ogolne.md) · [Spis treści](README.md) · [Następny →](02-podstawy-pythona.md)
