# Podstawy ogólne: co warto wiedzieć, zanim zaczniesz czytać kod

[Spis treści](README.md) · [Następny →](01-podstawy-jezyka-c.md)

Ten dokument nie wymaga żadnej wiedzy o programowaniu. Tłumaczy pojęcia, które wracają w każdym kolejnym pliku. Nie musisz zapamiętać wszystkiego za pierwszym razem: gdy trafisz na nieznane słowo, po prostu tu wróć.

## 1. Co robi ten projekt

Na stronie internetowej klikasz „Włącz” i na płytce zapala się dioda. Klikasz na płytce przycisk, a strona sama pokazuje nowy stan. Żeby to zadziałało, informacja przechodzi przez kilka urządzeń, jak list przekazywany z rąk do rąk:

```text
przeglądarka (telefon albo komputer)
    │   sieć LAN, język rozmowy: HTTP
    ▼
Raspberry Pi – mały komputer, działa na nim serwer w Pythonie (Flask)
    │   kabel USB
    ▼
konwerter USB-UART – tłumaczy USB na dwa proste przewody
    │   przewody TX i RX, język rozmowy: UART
    ▼
ATmega8 – mikrokontroler z naszym programem (firmware)
    │   przewody na płytce
    ▼
dioda główna, dioda statusu, przycisk, potencjometr, fotorezystor, czujnik temperatury
```

Stąd trzy duże części kodu:

- **firmware** (język C): program działający wewnątrz układu ATmega8,
- **serwer** (język Python): program na Raspberry Pi, pośrednik między stroną a układem,
- **strona WWW** (HTML, CSS, JavaScript): to, co widzisz w przeglądarce.

Do tego dochodzą **testy** (programy sprawdzające inne programy), **symulator** (udaje płytkę, gdy nie ma jej pod ręką) i **narzędzia** (skrypty pomocnicze).

## 2. Kod, program i kompilator

**Kod źródłowy** to zwykły tekst napisany przez człowieka, który opisuje krok po kroku, co ma zrobić urządzenie. Komputer czyta go od góry do dołu, linijka po linijce, chyba że kod każe mu skoczyć gdzie indziej albo coś powtórzyć.

Procesor nie rozumie jednak tekstu. Rozumie tylko liczby, czyli **kod maszynowy**. Są dwa sposoby, żeby to pogodzić:

- **Kompilator** tłumaczy cały tekst na kod maszynowy z wyprzedzeniem. Tak działa język C. Kompilator `avr-gcc` robi z naszych plików `.c` plik `firmware.hex`, który potem wgrywamy do układu. Czynność wgrywania nazywa się **flashowaniem**.
- **Interpreter** czyta tekst i wykonuje go na bieżąco. Tak działają Python i JavaScript.

**Biblioteka** to gotowy kod napisany przez kogoś innego, z którego można korzystać, np. `Flask` do budowy serwerów.

**Komentarz** to tekst w kodzie przeznaczony tylko dla ludzi. Komputer go pomija.

## 3. Bit, bajt i zapisywanie liczb

**Bit** to najmniejsza porcja informacji: 0 albo 1, jak włącznik światła. **Bajt** to 8 bitów ułożonych obok siebie. Bajt może przyjąć 256 różnych wartości, czyli liczby od 0 do 255.

Numerujemy bity od prawej strony, zaczynając od zera. Każdy bit ma swoją wagę:

| Numer bitu | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| Waga | 128 | 64 | 32 | 16 | 8 | 4 | 2 | 1 |
| Przykład: liczba 5 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |

Liczba 5 to `00000101`, bo 4 + 1 = 5.

Liczby można zapisywać na kilka sposobów, ale wartość pozostaje ta sama:

| Zapis | Nazwa | Przykład (liczba 255) |
|---|---|---|
| zwykły | dziesiętny | `255` |
| z `0b` na początku | dwójkowy | `0b11111111` |
| z `0x` na początku | szesnastkowy | `0xFF` |

W zapisie szesnastkowym są cyfry 0–9 i litery A–F (A = 10, ..., F = 15). Jedna cyfra szesnastkowa odpowiada dokładnie czterem bitom, dlatego programiści lubią ten zapis: `0x80` to od razu `1000 0000`.

Zakresy, które będą się powtarzać:

| Liczba bitów | Największa wartość | Gdzie to spotkasz |
|---|---|---|
| 8 | 255 | większość zmiennych w firmware |
| 10 | 1023 | wynik pomiaru przetwornika ADC i poziom PWM |
| 16 | 65 535 | suma 16 pomiarów ADC, licznik milisekund |
| 32 | 4 294 967 295 | czas pracy układu w milisekundach |

Zmienna ma stałą pojemność. Gdy przekroczysz największą wartość, licznik „przekręca się” do zera, jak stary licznik kilometrów.

## 4. Operacje na bitach

W mikrokontrolerze jeden bajt często oznacza osiem osobnych włączników. Trzeba umieć ruszyć jednym, nie dotykając pozostałych. Służą do tego operatory bitowe:

| Zapis | Nazwa | Działanie |
|---|---|---|
| `a & b` | AND, „i” | bit wyniku jest 1 tylko wtedy, gdy w obu liczbach jest 1 |
| `a \| b` | OR, „lub” | bit wyniku jest 1, gdy w którejkolwiek liczbie jest 1 |
| `a ^ b` | XOR, „albo” | bit wyniku jest 1, gdy bity są różne |
| `~a` | NOT, „nie” | odwraca wszystkie bity |
| `a << n` | przesunięcie w lewo | przesuwa bity o `n` miejsc w lewo, czyli mnoży przez 2, `n` razy |
| `a >> n` | przesunięcie w prawo | przesuwa bity o `n` miejsc w prawo, czyli dzieli przez 2, `n` razy, bez reszty |

Kluczowy przepis: `_BV(n)` to skrót od `(1 << n)`, czyli bajt z jedynką tylko na bicie numer `n`. Na przykład `_BV(1)` to `00000010`. Taką liczbę nazywamy **maską**. Trzy zapisy spotkasz w firmware dziesiątki razy:

```c
REJESTR |= _BV(3);    // ustaw bit 3 na 1, reszty nie ruszaj
REJESTR &= ~_BV(3);   // ustaw bit 3 na 0, reszty nie ruszaj
REJESTR ^= _BV(3);    // odwróć bit 3 (0 zmień na 1, a 1 na 0)
```

Przykład drugiego zapisu, gdy `REJESTR` zawiera `00001100`:

```text
_BV(3)                = 00001000
~_BV(3)               = 11110111
00001100 & 11110111   = 00000100   ← bit 3 zgaszony, bit 2 nietknięty
```

Sprawdzanie jednego bitu wygląda tak: `if (REJESTR & _BV(3))`. Wynik jest różny od zera tylko wtedy, gdy bit 3 jest ustawiony.

## 5. Mikrokontroler

**Mikrokontroler** to mały, kompletny komputer w jednym układzie scalonym. Ma procesor, pamięć i wyprowadzenia (nóżki) do podłączania elektroniki. Nasz to **ATmega8**. Pracuje z zegarem 1 MHz, czyli wykonuje około miliona prostych kroków na sekundę.

### Trzy rodzaje pamięci

| Pamięć | Rozmiar w ATmega8 | Co przechowuje | Czy znika po odłączeniu prądu |
|---|---|---|---|
| **Flash** | 8 KB | program i stałe teksty | nie |
| **RAM** | 1 KB | zmienne działającego programu | tak |
| **EEPROM** | 512 B | ustawienia zapisywane przez program | nie |

Pamięć EEPROM ma ograniczoną liczbę zapisów, dlatego firmware zapisuje ją rzadko i tylko te bajty, które naprawdę się zmieniły.

### Rejestry

**Rejestr** to specjalne miejsce w pamięci, które steruje sprzętem. Zapisanie w nim liczby daje natychmiastowy skutek fizyczny, np. pojawia się napięcie na nóżce. Układ ma ich dużo, a ich nazwy pochodzą z dokumentacji producenta (np. `ADCSRA`, `TCCR1A`). Każdy bit rejestru zwykle ma własną nazwę i znaczenie.

### Piny i porty

**Pin** to nóżka układu. Piny są pogrupowane w **porty** (`B`, `C`, `D`), a nazwa `PB1` oznacza „port B, bit 1”. Każdym portem sterują trzy rejestry:

| Rejestr | Znaczenie | Przykład |
|---|---|---|
| `DDRx` | kierunek: 1 = wyjście, 0 = wejście | `DDRB` |
| `PORTx` | na wyjściu: 1 = napięcie 5 V, 0 = 0 V; na wejściu: 1 = włącz pull-up | `PORTB` |
| `PINx` | odczyt tego, co naprawdę jest na pinie | `PINB` |

**Pull-up** to wbudowany rezystor, który „podciąga” pin do 5 V, gdy nic go nie ściąga w dół. Nasz przycisk łączy pin z masą (GND), więc po naciśnięciu pin ma 0 V, a bez naciśnięcia 5 V. Stąd odwrócona logika: 0 oznacza „wciśnięty”.

**Masa (GND)** to punkt odniesienia dla napięć, czyli „0 V”.

### Przerwania

**Przerwanie** (po angielsku *interrupt*) to sygnał, że stało się coś ważnego, np. przyszedł znak lub minęła milisekunda. Procesor wstrzymuje wtedy to, co robi, wykonuje krótką funkcję obsługi i wraca dokładnie tam, gdzie przerwał. To jak dzwonek do drzwi w trakcie gotowania. Taką funkcję obsługi nazywamy **ISR**.

Główna pętla programu i przerwania korzystają czasem z tych samych zmiennych, dlatego trzeba uważać, aby przerwanie nie zmieniło liczby w połowie jej odczytywania.

### Timer

**Timer** to sprzętowy licznik, który sam zlicza takty zegara, niezależnie od programu. Może np. wywoływać przerwanie co 1 ms albo sam generować sygnał na pinie. **Preskaler** to dzielnik: spowalnia tempo liczenia, np. preskaler 8 przepuszcza co ósmy takt.

### PWM

Mikrokontroler nie umie płynnie zmieniać napięcia na pinie, zna tylko 0 V i 5 V. **PWM** (modulacja szerokości impulsu) polega na bardzo szybkim włączaniu i wyłączaniu diody. **Wypełnienie** to część czasu, przez którą dioda świeci. Przy wypełnieniu 25 % dioda świeci przez jedną czwartą czasu, a oko, które nie nadąża, widzi słabsze światło. U nas dzieje się to około 977 razy na sekundę.

### ADC

**Przetwornik analogowo-cyfrowy (ADC)** mierzy napięcie i zamienia je na liczbę. U nas ma 10 bitów: 0 V to 0, a napięcie odniesienia 5 V to około 1023. Dzięki niemu układ „czuje” położenie potencjometru, ilość światła i temperaturę.

- **Potencjometr** to pokrętło, którego obrót zmienia napięcie.
- **Fotorezystor (LDR)** to element, którego opór zależy od światła.
- **Czujnik temperatury**: firmware obsługuje dwa typy, LM35 (10 mV na stopień) i termistor NTC (opór zależy od temperatury). Ostateczny typ jest jeszcze **do ustalenia**, patrz `docs/sprzet.md`.

### UART

**UART** to prosty sposób przesyłania danych dwoma przewodami: TX (nadawanie) i RX (odbiór). Znaki idą bit po bicie. Obie strony muszą ustalić tempo, u nas 9600 bitów na sekundę (**bps**), czyli około 960 znaków na sekundę. Format **8N1** oznacza: 8 bitów danych, bez bitu parzystości, 1 bit stopu.

Tekst jest przesyłany jako liczby według tabeli **ASCII**, np. `A` = 65, `a` = 97, `0` = 48. Dwa znaki specjalne kończą linię: `\n` (nowa linia, 10) i `\r` (powrót karetki, 13).

### Watchdog

**Watchdog** („pies stróżujący”) to sprzętowy licznik, który trzeba regularnie zerować. Jeśli program się zawiesi i przestanie to robić, watchdog sam zresetuje układ.

### Reset

**Reset** to uruchomienie programu od początku. Układ zapamiętuje jego przyczynę: **POR** (włączenie zasilania), **EXT** (sygnał na nóżce RESET), **WDT** (watchdog), **BOR** (spadek napięcia).

### Drgania styków

Metalowe styki przycisku w chwili naciśnięcia odbijają się przez kilka milisekund, więc układ zobaczyłby wiele szybkich naciśnięć. Odfiltrowanie tego zjawiska to **debouncing**.

### Histereza

**Histereza** to celowy margines, który zapobiega ciągłym przełączeniom w pobliżu progu. Alarm włącza się np. przy 50 °C, ale gaśnie dopiero poniżej 48 °C. Inaczej temperatura 49,9–50,1 °C włączałaby i wyłączała alarm bez przerwy.

## 6. Pojęcia z komunikacji

- **Protokół** to umowa, jak dwie strony mają ze sobą rozmawiać. Nasz protokół tekstowy jest opisany w `docs/protokol.md`: serwer wysyła np. `PWM 60`, a układ odpowiada `OK pwm=60`.
- **Komenda** to polecenie wysłane do układu.
- **Zdarzenie** (`EVT`) to wiadomość, którą układ wysyła sam, np. po naciśnięciu przycisku.
- **Port szeregowy** to w komputerze „gniazdo” do rozmowy przez UART, np. `/dev/ttyUSB0` w Linuksie.
- **Pseudoterminal** to programowa imitacja portu szeregowego. Przydaje się w symulacji.
- **Bufor** to pamięć podręczna na dane, które czekają na przetworzenie.

## 7. Jak czytać kolejne dokumenty

Każdy z nich pokazuje fragment prawdziwego kodu z numerami linii (numer nie jest częścią kodu), a pod nim objaśnia każdą linię:

```c
 14      ADCSRA |= _BV(ADSC);
```

- **Linia 14:** tak wygląda objaśnienie.

Puste linie nie mają opisu, bo komputer je pomija. Służą tylko temu, żeby kod lepiej się czytało.

---

[Spis treści](README.md) · [Następny →](01-podstawy-jezyka-c.md)
