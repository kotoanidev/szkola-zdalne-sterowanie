# Serwer: start programu i konfiguracja

[← Poprzedni](21-firmware-demo.md) · [Spis treści](README.md) · [Następny →](31-serwer-protocol.md)

**Po co te pliki:** to, co dzieje się przy uruchomieniu serwera. `__main__.py` jest punktem startu, `config.py` czyta ustawienia, a `__init__.py` składa wszystko w działającą aplikację. Składnia Pythona jest wytłumaczona w [podstawach Pythona](02-podstawy-pythona.md).

## `server/app/__main__.py`

[Otwórz ten plik w repozytorium](../../server/app/__main__.py)

Plik uruchamiany poleceniem `python -m app`.

```python
  1  """Uruchomienie: python -m app (konfiguracja w zmiennych środowiskowych, patrz README)."""
  2
  3  import logging
  4
  5  from . import create_app
  6  from .config import Config
```

- **Linia 1:** Docstring: opis pliku. Uruchamiamy go przez `python -m app`, a konfiguracja jest w zmiennych środowiskowych.
- **Linia 3:** Biblioteka do zapisywania komunikatów o pracy programu.
- **Linia 5:** Pobiera funkcję `create_app` z pliku `__init__.py` tego samego pakietu. Sama kropka oznacza „ten pakiet”.
- **Linia 6:** Pobiera klasę `Config` z pliku `config.py`.

```python
  9  def main() -> None:
 10      logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")
 11      config = Config.from_env()
 12      app = create_app(config)
 13      mode = f"symulacja: {config.simulate}" if config.simulate else (config.serial_port or "autowykrywanie portu")
 14      logging.getLogger("app").info("start na %s:%d (%s)", config.host, config.port, mode)
 15      app.run(host=config.host, port=config.port, threaded=True, use_reloader=False)
```

- **Linia 9:** Definicja funkcji `main`. `-> None` oznacza, że nic nie zwraca.
- **Linia 10:** Ustawia sposób wypisywania komunikatów: poziom `INFO` i wyżej, a format zawiera czas, poziom, nazwę źródła i treść.
- **Linia 11:** Wczytuje konfigurację ze zmiennych środowiskowych.
- **Linia 12:** Tworzy gotową aplikację Flask.
- **Linia 13:** Tworzy opis trybu pracy. Jeśli włączona jest symulacja, napis o symulacji. W przeciwnym razie nazwa portu, a gdy jej nie podano, napis „autowykrywanie portu”.
- **Linia 14:** Zapisuje komunikat startowy. `%s` i `%d` zastępowane są kolejno przez adres, numer portu i opis trybu.
- **Linia 15:** Uruchamia serwer WWW i czeka na zapytania. `threaded=True` pozwala obsługiwać wiele zapytań naraz, a `use_reloader=False` wyłącza automatyczne przeładowanie, które uruchomiłoby wszystko dwukrotnie.

```python
 18  if __name__ == "__main__":
 19      main()
```

- **Linia 18:** Warunek prawdziwy tylko wtedy, gdy plik uruchomiono bezpośrednio, a nie zaimportowano.
- **Linia 19:** Wywołaj funkcję `main`.

## `server/app/config.py`

[Otwórz ten plik w repozytorium](../../server/app/config.py)

Odczyt ustawień. Numer portu czy nazwa łącza nie są wpisane w kodzie na stałe, dzięki czemu ten sam kod działa na różnych komputerach.

```python
  1  """Konfiguracja wyłącznie ze zmiennych środowiskowych."""
  2
  3  from __future__ import annotations
  4
  5  import os
  6  from dataclasses import dataclass
```

- **Linia 1:** Docstring: konfiguracja pochodzi wyłącznie ze zmiennych środowiskowych.
- **Linia 3:** Pozwala w podpowiedziach typów używać nowszego zapisu, np. `str | None`.
- **Linia 5:** Biblioteka dostępu do systemu, m.in. do zmiennych środowiskowych.
- **Linia 6:** Pobiera dekorator `dataclass`, który ułatwia pisanie klas z danymi.

```python
  9  def _simulator(value: str) -> str:
 10      """SIMULATE=1 – symulator w Pythonie, SIMULATE=avr – prawdziwy firmware w simavr."""
 11      value = value.strip().lower()
 12      if value == "avr":
 13          return "avr"
 14      return "python" if value in ("1", "true", "tak", "yes", "on", "python") else ""
```

- **Linia 9:** Funkcja zamienia tekst zmiennej `SIMULATE` na jeden z trzech wyników. Podkreślenie na początku nazwy to umowa: do użytku wewnętrznego.
- **Linia 10:** Docstring: `1` oznacza symulator w Pythonie, a `avr` prawdziwy firmware w simavr.
- **Linia 11:** Usuwa białe znaki z początku i końca, zamienia litery na małe.
- **Linia 12:** Jeśli wartość to `avr`...
- **Linia 13:** ...zwróć `avr`.
- **Linia 14:** Jeśli wartość to jedno z kilku słów oznaczających „tak”, zwróć `python`, a w przeciwnym razie pusty napis, czyli brak symulacji.

```python
 17  @dataclass
 18  class Config:
 19      host: str = "0.0.0.0"
 20      port: int = 5000
 21      serial_port: str | None = None
 22      simulate: str = ""
 23      poll_interval: float = 1.0
```

- **Linia 17:** Dekorator: Python sam dopisze klasie m.in. funkcję `__init__`.
- **Linia 18:** Początek klasy `Config`, przechowującej ustawienia.
- **Linia 19:** Adres nasłuchu z wartością domyślną `0.0.0.0`, czyli „przyjmuj połączenia na wszystkich interfejsach sieciowych”.
- **Linia 20:** Numer portu, domyślnie 5000.
- **Linia 21:** Nazwa portu szeregowego. `None` oznacza „wykryj automatycznie”.
- **Linia 22:** Tryb symulacji. Pusty napis oznacza brak symulacji.
- **Linia 23:** Co ile sekund odczytywać stan układu, domyślnie co 1 s.

```python
 25      @classmethod
 26      def from_env(cls) -> "Config":
 27          return cls(
 28              host=os.environ.get("HOST", cls.host),
 29              port=int(os.environ.get("PORT", cls.port)),
 30              serial_port=os.environ.get("SERIAL_PORT") or None,
 31              simulate=_simulator(os.environ.get("SIMULATE", "")),
 32              poll_interval=float(os.environ.get("POLL_INTERVAL", cls.poll_interval)),
 33          )
```

- **Linia 25:** Dekorator: to metoda klasy, a nie konkretnego obiektu.
- **Linia 26:** Metoda `from_env` tworzy `Config` na podstawie środowiska. `cls` to sama klasa.
- **Linia 27:** Tworzy i zwraca nowy obiekt. Dalej idą jego pola.
- **Linia 28:** `HOST` ze środowiska, a jeśli go nie ma, wartość domyślna z klasy.
- **Linia 29:** `PORT` ze środowiska, zamieniony na liczbę całkowitą.
- **Linia 30:** `SERIAL_PORT` ze środowiska. `or None` zamienia pusty napis na `None`.
- **Linia 31:** Tryb symulacji po przepuszczeniu przez funkcję `_simulator`.
- **Linia 32:** `POLL_INTERVAL` ze środowiska, zamieniony na liczbę z ułamkiem.
- **Linia 33:** Koniec wywołania.

## `server/app/__init__.py`

[Otwórz ten plik w repozytorium](../../server/app/__init__.py)

Plik `__init__.py` czyni z katalogu pakiet. Tu powstaje aplikacja: wybieramy łącze (prawdziwe albo symulowane) i rejestrujemy adresy strony.

```python
  1  """Serwer Flask: mostek między przeglądarką a ATmega8 (docs/protokol.md, rozdz. 5)."""
  2
  3  from __future__ import annotations
  4
  5  import atexit
  6
  7  from flask import Flask
  8
  9  from .config import Config
 10  from .device import Device
 11  from .mcu_sim import McuSim
 12  from .transport import SerialTransport, SimTransport
```

- **Linia 1:** Docstring: serwer Flask jest mostkiem między przeglądarką a układem ATmega8.
- **Linia 3:** Nowszy zapis podpowiedzi typów.
- **Linia 5:** Biblioteka pozwalająca zarejestrować funkcję do wykonania przy zamykaniu programu.
- **Linia 7:** Pobiera klasę `Flask`, czyli główny obiekt serwera WWW.
- **Linia 9:** Pobiera naszą konfigurację.
- **Linia 10:** Pobiera naszą klasę `Device`, która obsługuje łącze z układem.
- **Linia 11:** Pobiera nasz symulator układu w Pythonie.
- **Linia 12:** Pobiera dwa rodzaje łącza: prawdziwy port i symulowany.

### Wybór łącza

```python
 15  def build_device(config: Config) -> tuple[Device, object | None]:
 16      if config.simulate == "avr":
 17          from .avrsim import AvrSim
 18
 19          avr = AvrSim()
 20          atexit.register(avr.close)
 21          device = Device(SerialTransport(avr.pty), config.poll_interval, simulated="avr")
 22          avr.on_change = device.publish_sim
 23          return device, avr
 24      if config.simulate:
 25          sim = McuSim()
 26          return Device(SimTransport(sim), config.poll_interval, simulated="python"), sim
 27      return Device(SerialTransport(config.serial_port), config.poll_interval), None
```

- **Linia 15:** Funkcja dostaje konfigurację, a zwraca parę: obiekt `Device` i symulator albo `None`.
- **Linia 16:** Jeśli wybrano symulację prawdziwego firmware...
- **Linia 17:** ...załaduj moduł `avrsim`. Import jest tutaj, bo moduł jest potrzebny tylko w tym trybie, a ten działa wyłącznie w Linuksie.
- **Linia 19:** Uruchom symulator z firmware.
- **Linia 20:** Zarejestruj zamknięcie symulatora przy kończeniu programu.
- **Linia 21:** Utwórz `Device` podłączony do pseudoterminala symulatora, tak jak do prawdziwego portu.
- **Linia 22:** Niech zmiany na wirtualnej płytce trafiają od razu do `device.publish_sim`, a stamtąd na stronę.
- **Linia 23:** Zwróć parę.
- **Linia 24:** Jeśli wybrano inną symulację, czyli model w Pythonie...
- **Linia 25:** ...utwórz symulator...
- **Linia 26:** ...i zwróć `Device` połączony z nim oraz sam symulator.
- **Linia 27:** W pozostałych przypadkach zwróć `Device` z prawdziwym portem szeregowym i brak symulatora.

### Budowa aplikacji

```python
 30  def create_app(config: Config | None = None, device: Device | None = None,
 31                 sim: object | None = None) -> Flask:
 32      config = config or Config.from_env()
 33      if device is None:
 34          device, sim = build_device(config)
 35          device.start()
 36
 37      app = Flask(__name__)
 38      app.json.ensure_ascii = False
 39      app.json.sort_keys = False
 40      app.extensions["device"] = device
 41      app.extensions["sim"] = sim
 42
 43      from .routes import bp
 44      app.register_blueprint(bp)
 45      return app
```

- **Linia 30:** Początek funkcji `create_app`. Wszystkie parametry są opcjonalne: testy mogą podać własne obiekty.
- **Linia 31:** Ciąg dalszy listy parametrów. Funkcja zwraca aplikację `Flask`.
- **Linia 32:** Użyj podanej konfiguracji, a jeśli jej nie ma, wczytaj ze środowiska.
- **Linia 33:** Jeśli nie podano `device`...
- **Linia 34:** ...utwórz go wraz z symulatorem...
- **Linia 35:** ...i uruchom jego wątki.
- **Linia 37:** Utwórz aplikację Flask. `__name__` pomaga jej znaleźć katalog `static`.
- **Linia 38:** Polskie znaki w JSON mają zostać polskimi znakami, a nie kodami takimi jak `ą`.
- **Linia 39:** Klucze w JSON mają zachować kolejność, a nie być sortowane.
- **Linia 40:** Odłóż `device` w aplikacji, by funkcje obsługujące adresy mogły go znaleźć.
- **Linia 41:** Tak samo odłóż symulator.
- **Linia 43:** Załaduj zestaw adresów strony. Import jest wykonywany dopiero tutaj, w trakcie tworzenia aplikacji.
- **Linia 44:** Zarejestruj te adresy w aplikacji.
- **Linia 45:** Zwróć gotową aplikację.

---

[← Poprzedni](21-firmware-demo.md) · [Spis treści](README.md) · [Następny →](31-serwer-protocol.md)
