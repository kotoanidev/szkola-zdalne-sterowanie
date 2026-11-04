# Serwer: `transport.py`, łącze z układem

[← Poprzedni](31-serwer-protocol.md) · [Spis treści](README.md) · [Następny →](33-serwer-device.md)

**Po co ten plik:** ukrywa, przez co rozmawiamy z układem. Są dwie klasy o identycznych metodach (`open`, `close`, `write_line`, `read_line`):

- `SerialTransport` rozmawia przez prawdziwy port szeregowy,
- `SimTransport` rozmawia z symulatorem w Pythonie.

Reszta serwera nie musi wiedzieć, której używa. To jak gniazdko: urządzeniu wszystko jedno, skąd płynie prąd.

## `server/app/transport.py`

[Otwórz ten plik w repozytorium](../../server/app/transport.py)

```python
  1  """Łącze z mikrokontrolerem: prawdziwy port szeregowy albo symulator."""
  2
  3  from __future__ import annotations
  4
  5  import time
  6
  7  import serial
  8  from serial.tools import list_ports
  9
 10  from .mcu_sim import McuSim
 11
 12  BAUD = 9600
```

- **Linia 1:** Docstring: łącze z układem, prawdziwy port albo symulator.
- **Linia 3:** Nowszy zapis podpowiedzi typów.
- **Linia 5:** Biblioteka czasu.
- **Linia 7:** Biblioteka `pyserial` do obsługi portów szeregowych.
- **Linia 8:** Narzędzie do wypisania dostępnych portów.
- **Linia 10:** Pobiera nasz symulator.
- **Linia 12:** Prędkość łącza: 9600 bitów na sekundę, tak jak w firmware.

```python
 14  # VID:PID popularnych konwerterów USB-UART (docs/sprzet.md)
 15  KNOWN_CONVERTERS = {
 16      (0x1A86, 0x7523): "CH340",
 17      (0x10C4, 0xEA60): "CP2102",
 18      (0x0403, 0x6001): "FT232R",
 19      (0x067B, 0x2303): "PL2303",
 20  }
```

- **Linia 14:** Komentarz: VID i PID to numery, którymi urządzenie USB się przedstawia. Tu są popularne konwertery.
- **Linia 15:** Początek słownika. Kluczem jest para numerów, a wartością nazwa układu.
- **Linia 16:** Konwerter CH340.
- **Linia 17:** Konwerter CP2102.
- **Linia 18:** Konwerter FT232R.
- **Linia 19:** Konwerter PL2303.
- **Linia 20:** Koniec słownika.

```python
 23  class TransportError(Exception):
 24      pass
```

- **Linia 23:** Własny rodzaj błędu: „problem z łączem”.
- **Linia 24:** Klasa nie ma dodatkowej treści.

```python
 27  def find_converter() -> str | None:
 28      for port in sorted(list_ports.comports(), key=lambda p: p.device):
 29          if (port.vid, port.pid) in KNOWN_CONVERTERS:
 30              return port.device
 31      return None
```

- **Linia 27:** Funkcja szuka konwertera. Zwraca nazwę portu albo `None`.
- **Linia 28:** Przejdź po wszystkich portach, posortowanych po nazwie. `lambda` mówi, po czym sortować.
- **Linia 29:** Jeśli numery tego portu są na liście znanych konwerterów...
- **Linia 30:** ...zwróć jego nazwę, np. `/dev/ttyUSB0`.
- **Linia 31:** Nic nie znaleziono, zwróć `None`.

### Prawdziwy port szeregowy

```python
 34  class SerialTransport:
 35      def __init__(self, port: str | None = None, baud: int = BAUD) -> None:
 36          self.requested_port = port
 37          self.baud = baud
 38          self.port: str | None = None
 39          self._serial: serial.Serial | None = None
 40          self._buffer = b""
 41
 42      @property
 43      def description(self) -> str:
 44          return self.port or self.requested_port or "auto"
```

- **Linia 34:** Początek klasy.
- **Linia 35:** Funkcja tworząca obiekt. Port można podać, a jeśli nie, zostanie wykryty.
- **Linia 36:** Zapamiętaj żądany port.
- **Linia 37:** Zapamiętaj prędkość.
- **Linia 38:** Port faktycznie użyty. Na razie żaden.
- **Linia 39:** Obiekt biblioteki `pyserial`. Na razie brak.
- **Linia 40:** Bufor na odebrane bajty, jeszcze bez końca linii. `b""` to puste bajty.
- **Linia 42:** Dekorator: metodę czyta się jak pole.
- **Linia 43:** Metoda zwraca opis łącza.
- **Linia 44:** Zwróć pierwszą niepustą wartość: użyty port, żądany port albo napis `auto`.

```python
 46      def open(self) -> None:
 47          port = self.requested_port or find_converter()
 48          if port is None:
 49              raise TransportError("nie znaleziono konwertera USB-UART")
 50          try:
 51              self._serial = serial.Serial(port, self.baud, timeout=0.1, write_timeout=1)
 52              self._serial.reset_input_buffer()
 53          except (serial.SerialException, OSError) as e:
 54              raise TransportError(str(e)) from e
 55          self.port = port
 56          self._buffer = b""
```

- **Linia 46:** Metoda otwierająca łącze.
- **Linia 47:** Użyj żądanego portu, a jeśli go nie ma, poszukaj konwertera.
- **Linia 48:** Jeśli niczego nie ma...
- **Linia 49:** ...zgłoś błąd łącza.
- **Linia 50:** Próbuj, bo otwarcie może się nie udać.
- **Linia 51:** Otwórz port. `timeout=0.1` oznacza, że czytanie czeka najwyżej 0,1 s, a `write_timeout=1` to samo dla pisania, 1 s.
- **Linia 52:** Wyrzuć bajty, które czekały przed otwarciem portu.
- **Linia 53:** Jeśli wystąpił błąd portu lub systemu...
- **Linia 54:** ...zgłoś go jako nasz błąd łącza, zachowując informację o przyczynie.
- **Linia 55:** Zapamiętaj użyty port.
- **Linia 56:** Wyzeruj bufor.

```python
 58      def close(self) -> None:
 59          if self._serial is not None:
 60              try:
 61                  self._serial.close()
 62              except (serial.SerialException, OSError):
 63                  pass
 64          self._serial = None
```

- **Linia 58:** Metoda zamykająca łącze.
- **Linia 59:** Jeśli port jest otwarty...
- **Linia 60:** Próbuj, bo przy odłączonym kablu zamknięcie może się nie udać.
- **Linia 61:** Zamknij port.
- **Linia 62:** Jeśli wystąpił błąd...
- **Linia 63:** ...zignoruj go, bo i tak kończymy pracę z tym portem.
- **Linia 64:** Zapamiętaj, że portu już nie ma.

```python
 66      def write_line(self, line: str) -> None:
 67          if self._serial is None:
 68              raise TransportError("port zamknięty")
 69          try:
 70              self._serial.write(line.encode("ascii") + b"\n")
 71          except (serial.SerialException, OSError) as e:
 72              raise TransportError(str(e)) from e
```

- **Linia 66:** Metoda wysyłająca jedną linię.
- **Linia 67:** Jeśli port jest zamknięty...
- **Linia 68:** ...zgłoś błąd.
- **Linia 69:** Próbuj.
- **Linia 70:** Zamień tekst na bajty ASCII, dopisz bajt końca linii i wyślij.
- **Linia 71:** Jeśli wystąpił błąd, np. wyjęto kabel...
- **Linia 72:** ...zgłoś go jako błąd łącza.

```python
 74      def read_line(self, timeout: float) -> str | None:
 75          if self._serial is None:
 76              raise TransportError("port zamknięty")
 77          deadline = time.monotonic() + timeout
 78          while b"\n" not in self._buffer:
 79              try:
 80                  chunk = self._serial.read(max(1, self._serial.in_waiting))
 81              except (serial.SerialException, OSError, TypeError) as e:
 82                  raise TransportError(str(e)) from e
 83              if chunk:
 84                  self._buffer += chunk
 85              elif time.monotonic() >= deadline:
 86                  return None
 87          line, _, self._buffer = self._buffer.partition(b"\n")
 88          return line.decode("ascii", errors="replace").strip()
```

- **Linia 74:** Metoda czytająca jedną linię. Czeka najwyżej `timeout` sekund. Zwraca tekst albo `None`.
- **Linia 75:** Jeśli port jest zamknięty...
- **Linia 76:** ...zgłoś błąd.
- **Linia 77:** Oblicz chwilę, po której przestajemy czekać. `monotonic` to zegar, który nigdy nie cofa się przy zmianie godziny.
- **Linia 78:** Dopóki w buforze nie ma końca linii...
- **Linia 79:** Próbuj.
- **Linia 80:** Odczytaj tyle bajtów, ile czeka, ale co najmniej jeden.
- **Linia 81:** Jeśli wystąpił błąd...
- **Linia 82:** ...zgłoś go jako błąd łącza.
- **Linia 83:** Jeśli coś odczytano...
- **Linia 84:** ...dopisz to do bufora.
- **Linia 85:** W przeciwnym razie, jeśli czas minął...
- **Linia 86:** ...zwróć `None`, czyli brak linii.
- **Linia 87:** Podziel bufor na część przed końcem linii, sam koniec i resztę. Reszta zostaje w buforze na później.
- **Linia 88:** Zamień bajty na tekst, a błędne znaki na znak zastępczy. `strip` usuwa `\r`. Zwróć gotową linię.

### Symulowany port

```python
 91  class SimTransport:
 92      """Symulator podpięty jak port szeregowy; zadania okresowe MCU wykonuje przy odczycie."""
 93
 94      port = "symulator"
 95      description = "symulator"
 96
 97      def __init__(self, sim: McuSim) -> None:
 98          self.sim = sim
 99
100      def open(self) -> None:
101          pass
102
103      def close(self) -> None:
104          pass
```

- **Linia 91:** Początek klasy.
- **Linia 92:** Docstring: symulator podpięty jak port. Zadania okresowe układu są wykonywane przy odczycie.
- **Linia 94:** Stała nazwa portu.
- **Linia 95:** Stały opis.
- **Linia 97:** Funkcja tworząca obiekt. Dostaje symulator.
- **Linia 98:** Zapamiętaj symulator.
- **Linia 100:** Otwarcie łącza...
- **Linia 101:** ...nic nie robi, bo symulator jest zawsze gotowy.
- **Linia 103:** Zamknięcie łącza...
- **Linia 104:** ...też nic nie robi.

```python
106      def write_line(self, line: str) -> None:
107          self.sim.feed(line + "\n")
108
109      def read_line(self, timeout: float) -> str | None:
110          deadline = time.monotonic() + timeout
111          while True:
112              self.sim.tick()
113              line = self.sim.read_line(timeout=min(0.05, max(0.0, deadline - time.monotonic())))
114              if line is not None or time.monotonic() >= deadline:
115                  return line
```

- **Linia 106:** Wysłanie linii.
- **Linia 107:** Przekaż symulatorowi tekst z dopisanym końcem linii.
- **Linia 109:** Odczyt linii.
- **Linia 110:** Oblicz chwilę, po której przestajemy czekać.
- **Linia 111:** Pętla bez końca, z wyjściem przez `return`.
- **Linia 112:** Uruchom okresowe zadania symulatora, czyli odpowiednik pętli głównej firmware.
- **Linia 113:** Spróbuj odczytać linię, czekając najwyżej 0,05 s lub tyle, ile zostało czasu.
- **Linia 114:** Jeśli jest linia albo czas minął...
- **Linia 115:** ...zwróć wynik, którym może być `None`.

---

[← Poprzedni](31-serwer-protocol.md) · [Spis treści](README.md) · [Następny →](33-serwer-device.md)
