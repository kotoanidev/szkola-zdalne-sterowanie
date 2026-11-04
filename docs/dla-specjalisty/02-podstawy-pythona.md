# Podstawy Pythona

[← Poprzedni](01-podstawy-jezyka-c.md) · [Spis treści](README.md) · [Następny →](03-podstawy-www.md)

W Pythonie napisany jest serwer na Raspberry Pi, jego testy i narzędzie `tools/ntc_table.py`. Python jest zwykle bardziej czytelny od C, bo sam dba o pamięć i nie wymaga kompilacji.

## Wcięcia są częścią języka

W C blok zamykają klamry. W Pythonie blok wyznaczają **wcięcia**, a linia otwierająca kończy się dwukropkiem:

```python
if temperatura > 50:
    print("gorąco")      # wewnątrz if, bo jest wcięte
print("koniec")          # poza if, wykona się zawsze
```

Komentarz zaczyna się od `#`. Średników nie używamy.

**Docstring** to napis w potrójnym cudzysłowie (`"""..."""`) na początku pliku, klasy lub funkcji. Opisuje ich przeznaczenie.

## Wartości

| Typ | Przykład | Znaczenie |
|---|---|---|
| `int` | `42` | liczba całkowita |
| `float` | `2.5` | liczba z ułamkiem |
| `str` | `"tekst"` | napis |
| `bool` | `True`, `False` | prawda i fałsz |
| `None` | `None` | „brak wartości” |
| `bytes` | `b"PING"` | surowe bajty, np. do wysłania przewodem |

## Kolekcje

| Typ | Zapis | Znaczenie |
|---|---|---|
| lista | `[1, 2, 3]` | uporządkowany ciąg, można zmieniać |
| krotka | `(1, 2)` | jak lista, ale niezmienna |
| słownik | `{"on": True}` | pary klucz–wartość, jak prawdziwy słownik |
| zbiór | `{1, 2}` | bez powtórzeń i bez kolejności |

Użycie:

- `lista[0]`: pierwszy element, `lista[-1]`: ostatni,
- `lista[1:]`: wszystko od drugiego elementu, to tzw. **wycinek**,
- `slownik["on"]`: wartość pod kluczem `on`,
- `slownik.get("on", 0)`: to samo, ale jeśli klucza nie ma, wynikiem jest 0, a nie błąd,
- `{**a, **b}`: nowy słownik złożony z dwóch, wartości z `b` mają pierwszeństwo.

## Działania

Działa wszystko, co zwykle: `+ - * /`, `==`, `!=`, `<`, `>=`. Do tego:

- `//`: dzielenie bez reszty,
- `%`: reszta z dzielenia,
- `**`: potęga,
- `and`, `or`, `not`: logiczne „i”, „lub”, „nie”,
- `x in kolekcja`: czy `x` jest w kolekcji,
- `x is None`: czy `x` to dokładnie `None`,
- `a if warunek else b`: jeśli warunek jest prawdziwy, wynikiem jest `a`, a inaczej `b`,
- `wartosc or inna`: wynikiem jest `wartosc`, chyba że jest „pusta” (0, `""`, `None`), wtedy `inna`.

## Napisy z wstawkami: f-string

Litera `f` przed cudzysłowem pozwala wstawiać wartości w klamrach:

```python
f"OK pwm={value}"       # dla value = 60 daje: OK pwm=60
f"{t:.1f}"              # liczba z jedną cyfrą po przecinku
```

## Warunki i pętle

```python
if a:
    ...
elif b:          # "w przeciwnym razie, jeśli"
    ...
else:
    ...

for x in lista:          # po kolei każdy element
    ...
while warunek:           # dopóki warunek jest prawdziwy
    ...
for i in range(3):       # i = 0, 1, 2
    ...
```

`break` kończy pętlę, a `continue` przechodzi do następnego obrotu.

Pętla `for` może mieć gałąź `else`. Wykona się ona tylko wtedy, gdy pętla **nie** została przerwana przez `break`.

## Funkcje

```python
def dodaj(a: int, b: int = 0) -> int:
    return a + b
```

- `def` rozpoczyna definicję,
- `a: int` to **podpowiedź typu**: tu ma trafić liczba całkowita (Python tego nie wymusza, pomaga to ludziom i narzędziom),
- `b: int = 0` to parametr z wartością domyślną, którą można pominąć przy wywołaniu,
- `-> int` mówi, co funkcja zwróci (`-> None` oznacza, że nic),
- `str | None` oznacza „napis albo nic”.

Przy wywołaniu parametry można nazywać: `dodaj(a=2, b=3)`. Zapis `funkcja(**slownik)` rozkłada słownik na takie nazwane parametry.

**Lambda** to krótka funkcja bez nazwy: `lambda p: p.device`.

Funkcje można przekazywać jak zwykłe wartości, np. `Thread(target=self._reader)` przekazuje funkcję, którą nowy wątek ma uruchomić.

Nazwa z podkreśleniem na początku (`_reader`) to umowa oznaczająca: „do użytku wewnętrznego”.

## Klasy i obiekty

**Klasa** to foremka, a **obiekt** to ciastko z tej foremki. Klasa łączy dane (pola) i funkcje (metody).

```python
class Licznik:
    def __init__(self, start):    # __init__ uruchamia się przy tworzeniu obiektu
        self.wartosc = start      # self to "ten konkretny obiekt"

    def zwieksz(self):
        self.wartosc += 1

l = Licznik(5)     # tworzymy obiekt
l.zwieksz()        # wywołujemy metodę
```

**Dziedziczenie:** `class AvrSimError(Exception):` tworzy nowy rodzaj błędu oparty na ogólnym typie `Exception`.

**Słowo `pass`** oznacza: nic nie rób. Stosuje się je tam, gdzie składnia wymaga jakiejś treści.

## Dekoratory

Linia z `@` nad funkcją lub klasą dokleja do niej dodatkowe zachowanie:

| Dekorator | Znaczenie |
|---|---|
| `@dataclass` | sam tworzy `__init__` i inne metody klasy przechowującej dane |
| `@property` | metodę czyta się jak pole, bez nawiasów: `device.connected` |
| `@classmethod` | metoda dotyczy klasy, a nie konkretnego obiektu (pierwszy parametr to `cls`) |
| `@bp.get("/on")` | gdy przyjdzie zapytanie GET o `/on`, Flask uruchomi tę funkcję |
| `@pytest.fixture` | funkcja przygotowuje dane dla testów |

## Importy

```python
import json                       # cała biblioteka: json.loads(...)
from pathlib import Path          # jedna rzecz: Path(...)
from .protocol import parse_line  # kropka: z naszego pakietu, z pliku protocol.py
```

Linia `from __future__ import annotations` pozwala w podpowiedziach typów używać nowszego zapisu i nie wymaga, by typ istniał w chwili czytania linii.

**Moduł** to jeden plik `.py`. **Pakiet** to katalog modułów z plikiem `__init__.py`. Zmienna `__name__` zawiera nazwę modułu, a przy bezpośrednim uruchomieniu pliku ma wartość `"__main__"`.

## Obsługa błędów

Gdy coś pójdzie nie tak, Python zgłasza **wyjątek**, który przerywa wykonywanie, dopóki ktoś go nie złapie.

```python
try:
    x = int(tekst)           # to może się nie udać
except ValueError as e:      # co zrobić, jeśli się nie uda
    print(e)
finally:
    print("to wykona się zawsze")

raise ValueError("opis")     # zgłoszenie własnego błędu
```

Zapis `raise A(...) from e` zgłasza nowy błąd, zachowując informację o pierwotnym.

## Instrukcja `with`

```python
with self._lock:
    ...
```

Na początku bloku pobiera zasób, a na końcu sam go oddaje, nawet jeśli wystąpi błąd. Tu: zamyka blokadę, a potem ją otwiera.

## Wątki, czyli kilka czynności jednocześnie

**Wątek** to osobna linia wykonywania programu. Serwer jednocześnie czyta z portu, obsługuje stronę i odpytuje układ. Wątki wymagają ostrożności, bo mogą sobie wchodzić w drogę. Pomagają w tym:

| Narzędzie | Znaczenie |
|---|---|
| `threading.Thread(target=f, daemon=True)` | nowy wątek uruchamiający `f`; `daemon` oznacza, że nie blokuje zamknięcia programu |
| `threading.Lock()` | **blokada**: tylko jeden wątek naraz może wejść do chronionego fragmentu |
| `threading.RLock()` | blokada, którą ten sam wątek może zająć kilka razy |
| `threading.Event()` | **flaga**, na którą można czekać: `set()` podnosi, `clear()` opuszcza, `wait(t)` czeka najwyżej `t` sekund, `is_set()` sprawdza |
| `queue.Queue()` | **kolejka**: jeden wątek wkłada (`put`), drugi wyjmuje (`get`), a `get` umie czekać |

## Inne przydatne zapisy

| Zapis | Znaczenie |
|---|---|
| `[x * 2 for x in lista]` | **wyrażenie listowe**: buduje nową listę na podstawie innej |
| `", ".join(lista)` | skleja napisy z listy, wstawiając przecinek ze spacją |
| `"a=b".partition("=")` | dzieli napis na trzy części: `("a", "=", "b")` |
| `"a b".split()` | dzieli napis po spacjach na listę |
| `x := wartosc` | przypisanie wewnątrz warunku (tzw. **operator morsa**) |
| `yield wartosc` | funkcja oddaje wartość, ale tylko się zawiesza i potem działa dalej; to **generator** |
| `assert warunek` | jeśli warunek jest fałszywy, zgłoś błąd; podstawa testów |
| `isinstance(x, int)` | czy `x` jest typu `int` |

## Testy z pytest

**Pytest** uruchamia wszystkie funkcje o nazwach zaczynających się od `test_`. Sprawdzają one wyniki słowem `assert`. Parametr testu o nazwie takiej jak funkcja oznaczona `@pytest.fixture` sprawia, że pytest najpierw uruchomi tę funkcję i poda jej wynik do testu. Kod fixture po `yield` wykonuje się po teście, czyli sprząta.

## Flask

**Flask** to biblioteka do budowy serwerów WWW. Łączy adresy z funkcjami: gdy przeglądarka poprosi o `/on`, Flask uruchamia przypisaną funkcję, a jej wynik odsyła z powrotem. Więcej o samym HTTP znajdziesz w [podstawach WWW](03-podstawy-www.md).

---

[← Poprzedni](01-podstawy-jezyka-c.md) · [Spis treści](README.md) · [Następny →](03-podstawy-www.md)
