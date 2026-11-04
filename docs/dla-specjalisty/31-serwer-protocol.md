# Serwer: `protocol.py`, rozumienie odpowiedzi układu

[← Poprzedni](30-serwer-start.md) · [Spis treści](README.md) · [Następny →](32-serwer-transport.md)

**Po co ten plik:** układ odsyła tekst, np. `OK pwm=60 mode=MANUAL`. Serwer potrzebuje tego w wygodnej formie: rodzaj wiadomości oraz słownik z polami. Ten plik zamienia tekst na dane.

## `server/app/protocol.py`

[Otwórz ten plik w repozytorium](../../server/app/protocol.py)

```python
  1  """Parser linii protokołu UART (docs/protokol.md)."""
  2
  3  from __future__ import annotations
  4
  5  from dataclasses import dataclass, field
  6
  7  MODES = ("MANUAL", "AUTO")
  8  PWM_MAX = 100
  9  TLIM_MAX = 125
 10  LINE_MAX = 32
```

- **Linia 1:** Docstring: parser linii protokołu UART. **Parser** to kod, który rozkłada tekst na znaczące części.
- **Linia 3:** Nowszy zapis podpowiedzi typów.
- **Linia 5:** Pobiera `dataclass` i `field`, czyli narzędzia do klas z danymi.
- **Linia 7:** Dozwolone tryby pracy, jako niezmienna krotka.
- **Linia 8:** Największa jasność, 100 %.
- **Linia 9:** Największy próg alarmu, 125 °C.
- **Linia 10:** Najdłuższa komenda, 32 znaki. Tak samo jak w firmware.

```python
 13  @dataclass
 14  class Message:
 15      kind: str
 16      name: str = ""
 17      fields: dict[str, str] = field(default_factory=dict)
 18
 19      @property
 20      def is_event(self) -> bool:
 21          return self.kind == "EVT"
```

- **Linia 13:** Dekorator tworzący automatycznie `__init__` i inne metody.
- **Linia 14:** Klasa `Message` opisuje jedną linię od układu.
- **Linia 15:** Pole `kind`: rodzaj wiadomości, np. `OK`, `EVT`, `STATE`.
- **Linia 16:** Pole `name`: nazwa, np. `BTN` w `EVT BTN`. Domyślnie pusta.
- **Linia 17:** Pole `fields`: słownik par klucz–wartość. `default_factory=dict` tworzy nowy słownik dla każdego obiektu, by nie współdzieliły jednego.
- **Linia 19:** Dekorator: poniższą metodę czyta się jak pole, bez nawiasów.
- **Linia 20:** Metoda `is_event` zwraca prawdę lub fałsz.
- **Linia 21:** Prawda, jeśli rodzaj to `EVT`, czyli zdarzenie.

### Rozkładanie linii

Przykład: `EVT BTN on=0` daje `kind="EVT"`, `name="BTN"`, `fields={"on": "0"}`.

```python
 24  def parse_line(line: str) -> Message:
 25      parts = line.strip().split()
 26      if not parts:
 27          return Message("")
 28      kind = parts[0].upper()
 29      rest = parts[1:]
 30      name = ""
 31      # EVT, ERR i VER mają nazwę przed parami klucz=wartość
 32      if kind in ("EVT", "ERR", "VER") and rest and "=" not in rest[0]:
 33          name = rest[0]
 34          rest = rest[1:]
 35      fields = {}
 36      for item in rest:
 37          key, sep, value = item.partition("=")
 38          if sep:
 39              fields[key.lower()] = value
 40      return Message(kind, name, fields)
```

- **Linia 24:** Funkcja dostaje tekst i zwraca obiekt `Message`.
- **Linia 25:** `strip` usuwa białe znaki z brzegów, a `split` dzieli po spacjach na listę słów.
- **Linia 26:** Jeśli lista jest pusta, czyli linia była pusta...
- **Linia 27:** ...zwróć pustą wiadomość.
- **Linia 28:** Pierwsze słowo, zamienione na wielkie litery, to rodzaj.
- **Linia 29:** Reszta słów, czyli wszystko od drugiego.
- **Linia 30:** Na razie bez nazwy.
- **Linia 31:** Komentarz: wiadomości `EVT`, `ERR` i `VER` mają nazwę przed parami klucz=wartość.
- **Linia 32:** Jeśli to jeden z tych rodzajów, jest jakieś kolejne słowo i nie zawiera ono znaku `=`...
- **Linia 33:** ...to ono jest nazwą...
- **Linia 34:** ...i usuwamy je z reszty.
- **Linia 35:** Pusty słownik na pola.
- **Linia 36:** Przejdź po kolejnych słowach.
- **Linia 37:** Podziel słowo na część przed `=`, sam `=` i część po nim.
- **Linia 38:** Jeśli znak `=` wystąpił...
- **Linia 39:** ...zapisz parę w słowniku, a klucz zamień na małe litery.
- **Linia 40:** Zwróć gotową wiadomość.

### Zamiana na wartości

W wiadomości wszystko jest tekstem, nawet `"60"`. Tu zamieniamy to na właściwe typy.

```python
 43  def fields_to_state(fields: dict[str, str]) -> dict:
 44      """Zamienia pola z OK / STATE / EVT na wartości stanu w formacie API."""
 45      state: dict = {}
 46      for key, value in fields.items():
 47          try:
 48              if key in ("on", "alarm"):
 49                  state[key] = value == "1"
 50              elif key in ("pwm", "pot", "ldr", "tlim"):
 51                  state[key] = int(value)
 52              elif key == "temp":
 53                  state[key] = float(value)
 54              elif key == "up":
 55                  state["uptime_s"] = int(value)
 56              elif key == "mode" and value in MODES:
 57                  state[key] = value
 58          except ValueError:
 59              continue
 60      return state
```

- **Linia 43:** Funkcja dostaje słownik tekstów i zwraca słownik wartości.
- **Linia 44:** Docstring: pola z `OK`, `STATE` i `EVT` zamieniane są na wartości w formacie API.
- **Linia 45:** Pusty słownik na wynik.
- **Linia 46:** Przejdź po kolejnych parach klucz–wartość.
- **Linia 47:** Próbuj, bo zamiana może się nie udać.
- **Linia 48:** Jeśli klucz to `on` albo `alarm`...
- **Linia 49:** ...zapisz prawdę, gdy wartość to `"1"`, a w przeciwnym razie fałsz.
- **Linia 50:** Jeśli klucz to jeden z liczbowych...
- **Linia 51:** ...zamień na liczbę całkowitą.
- **Linia 52:** Jeśli klucz to temperatura...
- **Linia 53:** ...zamień na liczbę z ułamkiem.
- **Linia 54:** Jeśli klucz to czas pracy...
- **Linia 55:** ...zamień na liczbę i zapisz pod dłuższą nazwą `uptime_s`.
- **Linia 56:** Jeśli klucz to tryb i wartość jest dozwolona...
- **Linia 57:** ...zapisz ją.
- **Linia 58:** Jeśli zamiana się nie udała, np. `int("x")`...
- **Linia 59:** ...pomiń to pole i przejdź do następnego. Uszkodzone dane nie zatrzymają serwera.
- **Linia 60:** Zwróć wynik.

---

[← Poprzedni](30-serwer-start.md) · [Spis treści](README.md) · [Następny →](32-serwer-transport.md)
