# Testy: przygotowanie (`conftest.py`) i parser (`test_protocol.py`)

[← Poprzedni](42-www-app-js.md) · [Spis treści](README.md) · [Następny →](51-testy-api.md)

**Po co są testy:** to programy sprawdzające inne programy. Test wywołuje kawałek kodu i porównuje wynik z oczekiwanym. Dzięki temu po każdej zmianie można w kilka sekund sprawdzić, czy nic się nie zepsuło. Zasady działania pytest są w [podstawach Pythona](02-podstawy-pythona.md).

## `server/tests/conftest.py`

[Otwórz ten plik w repozytorium](../../server/tests/conftest.py)

Plik o tej nazwie pytest wczytuje sam. Zawiera wspólne przygotowania, czyli **fixture**, z których korzystają wszystkie testy.

```python
  1  import pytest
  2
  3  from app import create_app
  4  from app.config import Config
  5  from app.device import Device
  6  from app.mcu_sim import McuSim
  7  from app.transport import SimTransport, TransportError
```

- **Linia 1:** Biblioteka testów.
- **Linia 3:** Nasza funkcja tworząca aplikację.
- **Linia 4:** Nasza konfiguracja.
- **Linia 5:** Nasz obiekt obsługi układu.
- **Linia 6:** Nasz symulator układu.
- **Linia 7:** Symulowane łącze oraz rodzaj błędu łącza.

### Sztuczny zegar

Testy nie mogą naprawdę czekać, a wyniki muszą być powtarzalne. Sztuczny zegar pokazuje tyle, ile każe mu test.

```python
 10  class FakeClock:
 11      def __init__(self) -> None:
 12          self.t = 1000.0
 13
 14      def __call__(self) -> float:
 15          return self.t
 16
 17      def advance(self, seconds: float) -> None:
 18          self.t += seconds
```

- **Linia 10:** Początek klasy sztucznego zegara.
- **Linia 11:** Funkcja tworząca obiekt.
- **Linia 12:** Startowa wartość czasu.
- **Linia 14:** Metoda `__call__` sprawia, że obiekt można wywołać jak funkcję: `zegar()`.
- **Linia 15:** Zwraca aktualny czas.
- **Linia 17:** Metoda przesuwająca czas.
- **Linia 18:** Dodaje podaną liczbę sekund.

### Zepsute łącze

```python
 21  class DeadTransport:
 22      """Port, którego nie da się otworzyć (np. odłączony konwerter)."""
 23
 24      port = None
 25      description = "brak"
 26
 27      def open(self) -> None:
 28          raise TransportError("nie znaleziono konwertera USB-UART")
 29
 30      def close(self) -> None:
 31          pass
 32
 33      def write_line(self, line: str) -> None:
 34          raise TransportError("port zamknięty")
 35
 36      def read_line(self, timeout: float):
 37          raise TransportError("port zamknięty")
```

- **Linia 21:** Klasa udająca łącze, które nie działa.
- **Linia 22:** Docstring: port, którego nie da się otworzyć, np. po odłączeniu konwertera.
- **Linia 24:** Brak nazwy portu.
- **Linia 25:** Opis łącza.
- **Linia 27:** Otwarcie...
- **Linia 28:** ...zawsze zgłasza błąd.
- **Linia 30:** Zamknięcie...
- **Linia 31:** ...nic nie robi.
- **Linia 33:** Wysłanie...
- **Linia 34:** ...zawsze zgłasza błąd.
- **Linia 36:** Odczyt...
- **Linia 37:** ...zawsze zgłasza błąd.

### Fixture, czyli gotowe składniki dla testów

```python
 40  @pytest.fixture
 41  def clock():
 42      return FakeClock()
 43
 44
 45  @pytest.fixture
 46  def sim(clock):
 47      s = McuSim(clock=clock)
 48      s.read_line()  # EVT BOOT
 49      return s
```

- **Linia 40:** Dekorator: funkcja jest fixture.
- **Linia 41:** Test z parametrem `clock` dostanie wynik tej funkcji.
- **Linia 42:** Utwórz sztuczny zegar.
- **Linia 45:** Dekorator: funkcja jest fixture.
- **Linia 46:** Fixture `sim`. Sam korzysta z fixture `clock`.
- **Linia 47:** Utwórz symulator ze sztucznym zegarem.
- **Linia 48:** Odczytaj i wyrzuć pierwszą linię, zdarzenie startu, by nie przeszkadzała testom.
- **Linia 49:** Zwróć symulator.

```python
 52  @pytest.fixture
 53  def device():
 54      sim = McuSim()
 55      dev = Device(SimTransport(sim), poll_interval=0.1, simulated="python")
 56      dev.start()
 57      assert dev.wait_connected(3)
 58      yield dev, sim
 59      dev.stop()
```

- **Linia 52:** Dekorator: funkcja jest fixture.
- **Linia 53:** Fixture `device`, czyli działający obiekt `Device` z symulatorem.
- **Linia 54:** Utwórz symulator z prawdziwym zegarem.
- **Linia 55:** Utwórz obiekt `Device` podłączony do symulatora, z częstym odczytem co 0,1 s.
- **Linia 56:** Uruchom jego wątki.
- **Linia 57:** Upewnij się, że połączenie powstało w ciągu 3 s.
- **Linia 58:** Przekaż testowi parę obiektów. Tu test się wykonuje.
- **Linia 59:** Po teście zatrzymaj wątki.

```python
 62  @pytest.fixture
 63  def client(device):
 64      dev, sim = device
 65      app = create_app(Config(simulate="python"), device=dev, sim=sim)
 66      app.testing = True
 67      return app.test_client()
```

- **Linia 62:** Dekorator: funkcja jest fixture.
- **Linia 63:** Fixture `client`, czyli sztuczna przeglądarka.
- **Linia 64:** Rozpakuj parę z fixture `device`.
- **Linia 65:** Utwórz aplikację z gotowymi obiektami.
- **Linia 66:** Włącz tryb testowy Flaska: błędy nie są ukrywane.
- **Linia 67:** Zwróć klienta testowego, który wysyła zapytania bez prawdziwej sieci.

```python
 70  @pytest.fixture
 71  def offline_client():
 72      dev = Device(DeadTransport(), poll_interval=0.1)
 73      dev.start()
 74      app = create_app(Config(), device=dev)
 75      yield app.test_client()
 76      dev.stop()
```

- **Linia 70:** Dekorator: funkcja jest fixture.
- **Linia 71:** Fixture `offline_client`, czyli serwer bez układu.
- **Linia 72:** Utwórz obiekt `Device` z zepsutym łączem.
- **Linia 73:** Uruchom jego wątki.
- **Linia 74:** Utwórz aplikację.
- **Linia 75:** Przekaż testowi klienta.
- **Linia 76:** Po teście zatrzymaj wątki.

## `server/tests/test_protocol.py`

[Otwórz ten plik w repozytorium](../../server/tests/test_protocol.py)

Testy parsera, czyli sprawdzenie, czy tekst jest poprawnie rozkładany na części.

```python
  1  from app.protocol import fields_to_state, parse_line
  2
  3
  4  def test_parse_ok():
  5      msg = parse_line("OK pwm=60 mode=MANUAL\r")
  6      assert msg.kind == "OK"
  7      assert msg.fields == {"pwm": "60", "mode": "MANUAL"}
  8      assert not msg.is_event
```

- **Linia 1:** Pobierz funkcje, które będziemy testować.
- **Linia 4:** Test: odpowiedź `OK`.
- **Linia 5:** Rozłóż przykładową linię. `\r` na końcu sprawdza, czy zostanie usunięty.
- **Linia 6:** Rodzaj musi być `OK`.
- **Linia 7:** Pola muszą się zgadzać.
- **Linia 8:** To nie jest zdarzenie.

```python
 11  def test_parse_event_with_name():
 12      msg = parse_line("EVT BTN on=0")
 13      assert msg.is_event
 14      assert msg.name == "BTN"
 15      assert msg.fields == {"on": "0"}
```

- **Linia 11:** Test: zdarzenie z nazwą.
- **Linia 12:** Rozłóż linię.
- **Linia 13:** To musi być zdarzenie.
- **Linia 14:** Nazwa musi być `BTN`.
- **Linia 15:** Pola muszą się zgadzać.

```python
 18  def test_parse_err_and_ver():
 19      assert parse_line("ERR ARG").name == "ARG"
 20      assert parse_line("VER 0.10.0").name == "0.10.0"
 21      assert parse_line("PONG").kind == "PONG"
 22      assert parse_line("   ").kind == ""
```

- **Linia 18:** Test: wiadomości `ERR` i `VER`.
- **Linia 19:** Nazwa błędu to `ARG`.
- **Linia 20:** Nazwa wersji to `0.10.0`.
- **Linia 21:** Rodzaj odpowiedzi to `PONG`.
- **Linia 22:** Linia z samych spacji daje pusty rodzaj.

```python
 25  def test_state_conversion():
 26      msg = parse_line("STATE on=1 pwm=60 mode=AUTO pot=512 ldr=301 temp=-3.5 alarm=0 tlim=50 up=1234")
 27      assert fields_to_state(msg.fields) == {
 28          "on": True, "pwm": 60, "mode": "AUTO", "pot": 512, "ldr": 301,
 29          "temp": -3.5, "alarm": False, "tlim": 50, "uptime_s": 1234,
 30      }
```

- **Linia 25:** Test: zamiana pól na wartości.
- **Linia 26:** Rozłóż pełną linię `STATE`.
- **Linia 27:** Wynik musi być równy temu słownikowi:
- **Linia 28:** Pierwsza część oczekiwanych wartości. Jest tam prawda zamiast `1` i liczby zamiast tekstu.
- **Linia 29:** Druga część. `up` musi się nazywać `uptime_s`.
- **Linia 30:** Koniec słownika.

```python
 33  def test_state_conversion_skips_garbage():
 34      assert fields_to_state({"pwm": "x", "mode": "FOO", "temp": "2.5"}) == {"temp": 2.5}
 35      assert fields_to_state({"temp": "ERR"}) == {"temp": None}
```

- **Linia 33:** Test: uszkodzone dane są pomijane.
- **Linia 34:** Zła liczba i zły tryb znikają, zostaje tylko poprawna temperatura.
- **Linia 35:** Temperatura `ERR` (czujnik nie odpowiada) zamienia się na `None`, czyli brak wartości.

---

[← Poprzedni](42-www-app-js.md) · [Spis treści](README.md) · [Następny →](51-testy-api.md)
