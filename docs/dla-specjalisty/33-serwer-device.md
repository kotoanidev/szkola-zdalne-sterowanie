# Serwer: `device.py`, główny obiekt obsługi układu

[← Poprzedni](32-serwer-transport.md) · [Spis treści](README.md) · [Następny →](34-serwer-routes.md)

**Po co ten plik:** najważniejszy plik serwera. Pilnuje, by z układem rozmawiał jeden obiekt i by naraz szła tylko jedna komenda. Działają tu dwa stale pracujące **wątki**:

- **czytelnik** (`_reader`) odbiera linie i rozdziela je: zdarzenia `EVT` obsługuje, odpowiedzi odkłada do kolejki,
- **nadzorca** (`_supervisor`) łączy się z układem, w razie potrzeby łączy ponownie i co chwilę prosi o aktualny stan.

Do tego dochodzą wątki Flaska, które wysyłają komendy ze strony.

**Publikowanie zmian:** każda karta przeglądarki ma własną kolejkę (subskrypcję). Gdy zmieni się stan, wiadomość trafia do wszystkich kolejek, a strony pokazują nowy stan.

## `server/app/device.py`

[Otwórz ten plik w repozytorium](../../server/app/device.py)

### Początek pliku

```python
  1  """Jedyny obiekt obsługujący łącze z mikrokontrolerem.
  2
  3  Wątek odczytu rozdziela zdarzenia EVT od odpowiedzi na komendy, a wątek nadzorcy
  4  łączy się (i ponownie łączy) z mikrokontrolerem oraz co chwilę odczytuje stan komendą GET.
  5  """
  6
  7  from __future__ import annotations
  8
  9  import logging
 10  import queue
 11  import threading
 12  import time
 13  from collections import deque
 14  from typing import Callable, Protocol
 15
 16  from .protocol import MODES, PWM_MAX, TLIM_MAX, Message, fields_to_state, parse_line
 17  from .transport import TransportError
 18
 19  log = logging.getLogger(__name__)
 20
 21  RESPONSE_TIMEOUT = 0.5
 22  EXPECTED_REPLY = {"PING": "PONG", "VER": "VER", "GET": "STATE"}
 23  RECONNECT_INTERVAL = 2.0
 24  LOG_SIZE = 50
```

- **Linia 1:** Początek docstringa: jedyny obiekt obsługujący łącze.
- **Linia 3:** Wątek odczytu rozdziela zdarzenia od odpowiedzi na komendy, a wątek nadzorcy...
- **Linia 4:** ...łączy się ponownie z układem i co chwilę odczytuje stan.
- **Linia 5:** Koniec docstringa.
- **Linia 7:** Nowszy zapis podpowiedzi typów.
- **Linia 9:** Biblioteka komunikatów o pracy programu.
- **Linia 10:** Biblioteka kolejek.
- **Linia 11:** Biblioteka wątków.
- **Linia 12:** Biblioteka czasu.
- **Linia 13:** `deque` to lista z ograniczonym rozmiarem, która sama wyrzuca najstarsze elementy.
- **Linia 14:** Narzędzia do opisywania typów.
- **Linia 16:** Pobiera z naszego parsera stałe, klasę wiadomości i funkcje.
- **Linia 17:** Pobiera nasz rodzaj błędu łącza.
- **Linia 19:** Tworzy rejestrator komunikatów o nazwie tego modułu.
- **Linia 21:** Na odpowiedź układu czekamy najwyżej 0,5 s.
- **Linia 22:** Słownik: jakiej odpowiedzi oczekujemy na daną komendę. Dla pozostałych będzie to `OK`.
- **Linia 23:** Ponowne łączenie co 2 s.
- **Linia 24:** Dziennik mieści 50 wpisów.

### Opis wymaganego łącza

```python
 27  class Transport(Protocol):
 28      port: str | None
 29      description: str
 30
 31      def open(self) -> None: ...
 32      def close(self) -> None: ...
 33      def write_line(self, line: str) -> None: ...
 34      def read_line(self, timeout: float) -> str | None: ...
```

- **Linia 27:** `Protocol` opisuje, jakie metody musi mieć łącze. Dotyczy to zarówno prawdziwego, jak i symulowanego.
- **Linia 28:** Musi mieć pole `port`.
- **Linia 29:** Musi mieć pole `description`.
- **Linia 31:** Musi mieć metodę `open`. Wielokropek oznacza: treści tu nie ma, to tylko opis.
- **Linia 32:** Musi mieć metodę `close`.
- **Linia 33:** Musi mieć metodę `write_line`.
- **Linia 34:** Musi mieć metodę `read_line`.

### Rodzaje błędów

```python
 37  class DeviceError(Exception):
 38      """Błąd po stronie mikrokontrolera (ERR ...) – HTTP 502."""
 39
 40
 41  class NotConnected(Exception):
 42      """Brak połączenia lub odpowiedzi mikrokontrolera – HTTP 503."""
```

- **Linia 37:** Błąd zgłoszony przez układ.
- **Linia 38:** Docstring: układ odpowiedział `ERR`, serwer odda kod HTTP 502.
- **Linia 41:** Brak łącza.
- **Linia 42:** Docstring: nie ma połączenia albo odpowiedzi, serwer odda kod HTTP 503.

### Tworzenie obiektu

```python
 45  class Device:
 46      def __init__(self, transport: Transport, poll_interval: float = 1.0,
 47                   simulated: str = "", clock: Callable[[], float] = time.time) -> None:
 48          self.transport = transport
 49          self.poll_interval = poll_interval
 50          self.simulated = simulated
 51          self._clock = clock
 52          self._cmd_lock = threading.Lock()
 53          self._state_lock = threading.Lock()
 54          self._responses: queue.Queue[Message] = queue.Queue()
 55          self._subscribers: set[queue.Queue] = set()
 56          self._log: deque[dict] = deque(maxlen=LOG_SIZE)
 57          self._link_open = threading.Event()
 58          self._connected = threading.Event()
 59          self._refresh = threading.Event()
 60          self._stop = threading.Event()
 61          self._threads: list[threading.Thread] = []
 62          self._state: dict = {
 63              "connected": False,
 64              "on": False,
 65              "pwm": 0,
 66              "mode": "MANUAL",
 67              "pot": 0,
 68              "ldr": 0,
 69              "temp": None,
 70              "alarm": False,
 71              "tlim": 0,
 72              "uptime_s": 0,
 73          }
 74          self.firmware: str | None = None
 75          self.started_at = self._clock()
```

- **Linia 45:** Początek klasy `Device`.
- **Linia 46:** Funkcja tworząca obiekt. Dostaje łącze i odstęp odczytów...
- **Linia 47:** ...nazwę symulatora i zegar. Zegar można podmienić w testach.
- **Linia 48:** Zapamiętaj łącze.
- **Linia 49:** Zapamiętaj odstęp odczytów.
- **Linia 50:** Zapamiętaj rodzaj symulacji.
- **Linia 51:** Zapamiętaj zegar.
- **Linia 52:** Blokada komend: tylko jedna komenda naraz.
- **Linia 53:** Blokada stanu: chroni stan, dziennik i subskrypcje.
- **Linia 54:** Kolejka odpowiedzi od układu.
- **Linia 55:** Zbiór kolejek przeglądarek.
- **Linia 56:** Dziennik, trzyma 50 ostatnich wpisów.
- **Linia 57:** Flaga: łącze jest otwarte.
- **Linia 58:** Flaga: układ potwierdził połączenie.
- **Linia 59:** Flaga: odczytaj stan od razu.
- **Linia 60:** Flaga: kończymy pracę.
- **Linia 61:** Lista uruchomionych wątków.
- **Linia 62:** Początek słownika z aktualnym stanem. Wartości startowe:
- **Linia 63:** Brak połączenia.
- **Linia 64:** Wyjście wyłączone.
- **Linia 65:** Jasność 0.
- **Linia 66:** Tryb ręczny.
- **Linia 67:** Potencjometr 0.
- **Linia 68:** Fotorezystor 0.
- **Linia 69:** Temperatura nieznana.
- **Linia 70:** Bez alarmu.
- **Linia 71:** Próg 0.
- **Linia 72:** Czas pracy 0.
- **Linia 73:** Koniec słownika.
- **Linia 74:** Wersja firmware, na razie nieznana.
- **Linia 75:** Zapamiętaj czas uruchomienia serwera.

### Start i stop

```python
 77      # --- cykl życia ---------------------------------------------------------
 78
 79      def start(self) -> None:
 80          for target, name in ((self._reader, "uart-reader"), (self._supervisor, "uart-supervisor")):
 81              t = threading.Thread(target=target, name=name, daemon=True)
 82              t.start()
 83              self._threads.append(t)
 84
 85      def stop(self) -> None:
 86          self._stop.set()
 87          for t in self._threads:
 88              t.join(timeout=2)
 89          self.transport.close()
 90
 91      @property
 92      def connected(self) -> bool:
 93          return self._connected.is_set()
 94
 95      def wait_connected(self, timeout: float) -> bool:
 96          return self._connected.wait(timeout)
```

- **Linia 77:** Komentarz oddzielający część „cykl życia”.
- **Linia 79:** Metoda uruchamiająca wątki.
- **Linia 80:** Dla każdej pary: funkcja wątku i jego nazwa...
- **Linia 81:** ...utwórz wątek. `daemon=True`: nie blokuje zamknięcia programu.
- **Linia 82:** Uruchom go.
- **Linia 83:** Dopisz go do listy.
- **Linia 85:** Metoda kończąca pracę.
- **Linia 86:** Podnieś flagę końca. Pętle wątków ją sprawdzają.
- **Linia 87:** Dla każdego wątku...
- **Linia 88:** ...poczekaj na jego zakończenie, najwyżej 2 s.
- **Linia 89:** Zamknij łącze.
- **Linia 91:** Dekorator: metodę czyta się jak pole.
- **Linia 92:** Metoda mówi, czy jest połączenie.
- **Linia 93:** Zwraca stan flagi.
- **Linia 95:** Metoda czekająca na połączenie, najwyżej `timeout` sekund.
- **Linia 96:** Zwraca prawdę, jeśli się doczekała.

### Stan, dziennik i subskrypcje

```python
 98      # --- stan, dziennik, subskrypcje SSE --------------------------------------
 99
100      def snapshot(self) -> dict:
101          with self._state_lock:
102              return dict(self._state)
103
104      def recent_log(self) -> list[dict]:
105          with self._state_lock:
106              return list(self._log)
107
108      def subscribe(self) -> queue.Queue:
109          q: queue.Queue = queue.Queue(maxsize=100)
110          with self._state_lock:
111              self._subscribers.add(q)
112          return q
113
114      def unsubscribe(self, q: queue.Queue) -> None:
115          with self._state_lock:
116              self._subscribers.discard(q)
```

- **Linia 98:** Komentarz oddzielający tę część.
- **Linia 100:** Metoda zwracająca kopię stanu.
- **Linia 101:** Zajmij blokadę, by nikt nie zmienił stanu w trakcie kopiowania.
- **Linia 102:** Zwróć kopię. Dzięki temu nikt z zewnątrz nie zmieni oryginału.
- **Linia 104:** Metoda zwracająca ostatnie wpisy dziennika.
- **Linia 105:** Zajmij blokadę.
- **Linia 106:** Zwróć je jako zwykłą listę.
- **Linia 108:** Metoda zapisująca przeglądarkę na aktualizacje.
- **Linia 109:** Utwórz dla niej kolejkę, mieszczącą najwyżej 100 wiadomości.
- **Linia 110:** Zajmij blokadę.
- **Linia 111:** Dodaj kolejkę do zbioru.
- **Linia 112:** Zwróć ją.
- **Linia 114:** Metoda wypisująca przeglądarkę.
- **Linia 115:** Zajmij blokadę.
- **Linia 116:** Usuń kolejkę. `discard` nie zgłasza błędu, gdy jej nie ma.

```python
118      def _publish(self, kind: str, data: dict) -> None:
119          with self._state_lock:
120              subscribers = list(self._subscribers)
121          for q in subscribers:
122              try:
123                  q.put_nowait((kind, data))
124              except queue.Full:
125                  pass
126
127      def publish_sim(self, snapshot: dict) -> None:
128          self._publish("sim", snapshot)
```

- **Linia 118:** Metoda wysyłająca wiadomość do wszystkich przeglądarek.
- **Linia 119:** Zajmij blokadę.
- **Linia 120:** Zrób kopię listy kolejek. Dzięki temu wysyłanie odbywa się już bez blokady.
- **Linia 121:** Dla każdej kolejki...
- **Linia 122:** Próbuj.
- **Linia 123:** ...włóż wiadomość bez czekania. To para: rodzaj i dane.
- **Linia 124:** Jeśli kolejka jest pełna, np. przeglądarka się zawiesiła...
- **Linia 125:** ...pomiń ją, by serwer na nią nie czekał.
- **Linia 127:** Metoda przekazująca stan wirtualnej płytki.
- **Linia 128:** Wyślij go jako wiadomość typu `sim`.

```python
130      def _update(self, changes: dict) -> None:
131          with self._state_lock:
132              new = {**self._state, **changes}
133              if new == self._state:
134                  return
135              self._state = new
136              snapshot = dict(new)
137          self._publish("state", snapshot)
138
139      def _add_log(self, text: str, source: str) -> None:
140          entry = {"t": self._clock(), "text": text, "source": source}
141          with self._state_lock:
142              self._log.append(entry)
143          self._publish("log", entry)
```

- **Linia 130:** Metoda aktualizująca stan.
- **Linia 131:** Zajmij blokadę.
- **Linia 132:** Utwórz nowy stan: stary z nałożonymi zmianami.
- **Linia 133:** Jeśli nic się nie zmieniło...
- **Linia 134:** ...zakończ i nikogo nie powiadamiaj.
- **Linia 135:** Zapisz nowy stan.
- **Linia 136:** Zrób kopię do wysłania.
- **Linia 137:** Wyślij stan do przeglądarek, już poza blokadą.
- **Linia 139:** Metoda dodająca wpis do dziennika.
- **Linia 140:** Utwórz wpis: czas, treść i źródło.
- **Linia 141:** Zajmij blokadę.
- **Linia 142:** Dopisz wpis.
- **Linia 143:** Wyślij wpis do przeglądarek.

### Wysyłanie komend

```python
145      # --- komendy -----------------------------------------------------------
146
147      def command(self, line: str, timeout: float = RESPONSE_TIMEOUT) -> Message:
148          if not self._link_open.is_set():
149              raise NotConnected("brak połączenia z mikrokontrolerem")
150          with self._cmd_lock:
151              while not self._responses.empty():
152                  self._responses.get_nowait()
153              try:
154                  self.transport.write_line(line)
155              except TransportError as e:
156                  self._disconnect(str(e))
157                  raise NotConnected(str(e)) from e
158              expected = EXPECTED_REPLY.get(line.split()[0].upper(), "OK")
159              deadline = time.monotonic() + timeout
160              while True:
161                  try:
162                      msg = self._responses.get(timeout=max(0.0, deadline - time.monotonic()))
163                  except queue.Empty:
164                      raise NotConnected(f"brak odpowiedzi na {line}") from None
165                  # Spóźniona odpowiedź albo urwana linia (np. port otwarty w trakcie nadawania)
166                  if msg.kind in (expected, "ERR"):
167                      break
168                  log.debug("pominięto linię %s %s w odpowiedzi na %s", msg.kind, msg.name, line)
169          if msg.kind == "ERR":
170              raise DeviceError(f"{line}: ERR {msg.name}")
171          if msg.kind in ("OK", "STATE"):
172              self._update(fields_to_state(msg.fields))
173          return msg
```

- **Linia 145:** Komentarz oddzielający tę część.
- **Linia 147:** Metoda wysyła jedną komendę i czeka na odpowiedź.
- **Linia 148:** Jeśli łącze nie jest otwarte...
- **Linia 149:** ...zgłoś błąd braku połączenia.
- **Linia 150:** Zajmij blokadę komend. Inne wątki poczekają na swoją kolej.
- **Linia 151:** Dopóki w kolejce odpowiedzi coś jest...
- **Linia 152:** ...wyrzuć to. Są to stare, spóźnione odpowiedzi.
- **Linia 153:** Próbuj.
- **Linia 154:** Wyślij komendę.
- **Linia 155:** Jeśli wysyłanie się nie udało...
- **Linia 156:** ...oznacz rozłączenie...
- **Linia 157:** ...i zgłoś błąd braku połączenia.
- **Linia 158:** Ustal oczekiwaną odpowiedź: weź pierwsze słowo komendy, poszukaj go w słowniku, a domyślnie przyjmij `OK`.
- **Linia 159:** Oblicz chwilę końca czekania.
- **Linia 160:** Pętla bez końca, z wyjściem przez `break`.
- **Linia 161:** Próbuj.
- **Linia 162:** Czekaj na wiadomość w kolejce przez tyle czasu, ile zostało.
- **Linia 163:** Jeśli czas minął...
- **Linia 164:** ...zgłoś brak odpowiedzi. `from None` ukrywa pierwotny wyjątek, bo nic nie wnosi.
- **Linia 165:** Komentarz: na łączu może być spóźniona odpowiedź albo urwana linia.
- **Linia 166:** Jeśli wiadomość to oczekiwana odpowiedź lub błąd...
- **Linia 167:** ...zakończ czekanie.
- **Linia 168:** Inaczej zapisz komunikat i czekaj dalej.
- **Linia 169:** Jeśli to błąd od układu...
- **Linia 170:** ...zgłoś go.
- **Linia 171:** Jeśli to odpowiedź zawierająca pola stanu...
- **Linia 172:** ...zaktualizuj stan.
- **Linia 173:** Zwróć wiadomość.

```python
175      def refresh(self) -> dict:
176          self.command("GET")
177          return self.snapshot()
178
179      def request_refresh(self) -> None:
180          self._refresh.set()
```

- **Linia 175:** Metoda natychmiast odczytująca stan.
- **Linia 176:** Wyślij komendę `GET`.
- **Linia 177:** Zwróć kopię stanu.
- **Linia 179:** Metoda prosząca nadzorcę o szybki odczyt.
- **Linia 180:** Podnieś flagę. Nadzorca ją zauważy.

```python
182      def set_state(self, on: bool | None = None, pwm: int | None = None,
183                    mode: str | None = None, tlim: int | None = None) -> dict:
184          """Zmienia stan; kolejność: on, pwm, mode, tlim (tryb AUTO po PWM ma pierwszeństwo)."""
185          if pwm is not None and not 0 <= pwm <= PWM_MAX:
186              raise ValueError("pwm poza zakresem 0–100")
187          if mode is not None and mode not in MODES:
188              raise ValueError("mode musi być MANUAL albo AUTO")
189          if tlim is not None and not 0 <= tlim <= TLIM_MAX:
190              raise ValueError(f"tlim poza zakresem 0–{TLIM_MAX}")
191          commands = []
192          if on is not None:
193              commands.append("ON" if on else "OFF")
194          if pwm is not None:
195              commands.append(f"PWM {pwm}")
196          if mode is not None:
197              commands.append(f"MODE {mode}")
198          if tlim is not None:
199              commands.append(f"TLIM {tlim}")
200          for line in commands:
201              self.command(line)
202              self._add_log(line, "web")
203          return self.snapshot()
```

- **Linia 182:** Metoda zmieniająca stan układu. Każdy parametr jest opcjonalny.
- **Linia 183:** Ciąg dalszy listy parametrów. Zwraca stan.
- **Linia 184:** Docstring: kolejność jest stała. Tryb AUTO po `PWM` ma pierwszeństwo, bo `PWM` przełącza na tryb ręczny.
- **Linia 185:** Jeśli podano jasność spoza zakresu...
- **Linia 186:** ...zgłoś błąd wartości, serwer odda kod 400.
- **Linia 187:** Jeśli podano nieznany tryb...
- **Linia 188:** ...zgłoś błąd wartości.
- **Linia 189:** Jeśli podano próg spoza zakresu...
- **Linia 190:** ...zgłoś błąd wartości.
- **Linia 191:** Pusta lista komend do wysłania.
- **Linia 192:** Jeśli podano stan wyjścia...
- **Linia 193:** ...dopisz `ON` albo `OFF`.
- **Linia 194:** Jeśli podano jasność...
- **Linia 195:** ...dopisz `PWM` z wartością.
- **Linia 196:** Jeśli podano tryb...
- **Linia 197:** ...dopisz `MODE` z wartością.
- **Linia 198:** Jeśli podano próg...
- **Linia 199:** ...dopisz `TLIM` z wartością.
- **Linia 200:** Dla każdej komendy...
- **Linia 201:** ...wyślij ją...
- **Linia 202:** ...i zapisz w dzienniku jako pochodzącą ze strony.
- **Linia 203:** Zwróć aktualny stan.

### Wątek czytelnika

```python
205      # --- wątki ---------------------------------------------------------------
206
207      def _reader(self) -> None:
208          while not self._stop.is_set():
209              if not self._link_open.is_set():
210                  self._stop.wait(0.05)
211                  continue
212              try:
213                  line = self.transport.read_line(0.2)
214              except TransportError as e:
215                  self._disconnect(str(e))
216                  continue
217              if not line:
218                  continue
219              msg = parse_line(line)
220              if msg.is_event:
221                  self._handle_event(msg, line)
222              elif msg.kind:
223                  self._responses.put(msg)
```

- **Linia 205:** Komentarz oddzielający część „wątki”.
- **Linia 207:** Funkcja wątku czytającego.
- **Linia 208:** Dopóki nie ma polecenia końca...
- **Linia 209:** Jeśli łącze jest zamknięte...
- **Linia 210:** ...poczekaj 0,05 s. Czekamy na fladze końca, by można było przerwać od razu.
- **Linia 211:** Wróć na początek pętli.
- **Linia 212:** Próbuj.
- **Linia 213:** Odczytaj linię, czekając najwyżej 0,2 s.
- **Linia 214:** Jeśli wystąpił błąd łącza...
- **Linia 215:** ...oznacz rozłączenie...
- **Linia 216:** ...i wróć na początek pętli.
- **Linia 217:** Jeśli nie ma linii...
- **Linia 218:** ...wróć na początek pętli.
- **Linia 219:** Rozłóż linię na części.
- **Linia 220:** Jeśli to zdarzenie...
- **Linia 221:** ...obsłuż je.
- **Linia 222:** W przeciwnym razie, jeśli wiadomość ma rodzaj...
- **Linia 223:** ...włóż ją do kolejki odpowiedzi, gdzie czeka metoda `command`.

```python
225      def _handle_event(self, msg: Message, line: str) -> None:
226          self._add_log(line, "mcu")
227          if msg.name == "BOOT":
228              self.firmware = msg.fields.get("ver", self.firmware)
229              self._refresh.set()
230          else:
231              self._update(fields_to_state(msg.fields))
232              self._refresh.set()
```

- **Linia 225:** Metoda obsługująca zdarzenie od układu.
- **Linia 226:** Dopisz je do dziennika jako pochodzące z układu.
- **Linia 227:** Jeśli to zdarzenie startu...
- **Linia 228:** ...odczytaj wersję firmware, a jeśli jej nie ma, zostaw dotychczasową.
- **Linia 229:** Poproś o szybki odczyt stanu.
- **Linia 230:** W przeciwnym razie:
- **Linia 231:** Zaktualizuj stan na podstawie pól zdarzenia.
- **Linia 232:** Poproś o szybki odczyt stanu.

### Wątek nadzorcy

```python
234      def _supervisor(self) -> None:
235          next_poll = 0.0
236          while not self._stop.is_set():
237              if not self.connected:
238                  self._try_connect()
239                  if not self.connected:
240                      self._stop.wait(RECONNECT_INTERVAL)
241                      continue
242                  next_poll = 0.0
243              now = time.monotonic()
244              if self._refresh.is_set() or now >= next_poll:
245                  self._refresh.clear()
246                  next_poll = now + self.poll_interval
247                  try:
248                      self.command("GET")
249                  except NotConnected as e:
250                      log.warning("GET: %s", e)
251                  except DeviceError as e:
252                      log.warning("%s", e)
253              self._refresh.wait(max(0.0, next_poll - time.monotonic()))
```

- **Linia 234:** Funkcja wątku nadzorcy.
- **Linia 235:** Czas następnego odczytu. Zero oznacza „od razu”.
- **Linia 236:** Dopóki nie ma polecenia końca...
- **Linia 237:** Jeśli nie ma połączenia...
- **Linia 238:** ...spróbuj się połączyć.
- **Linia 239:** Jeśli nadal nie ma...
- **Linia 240:** ...poczekaj 2 s...
- **Linia 241:** ...i wróć na początek pętli.
- **Linia 242:** Połączenie jest, więc odczytaj stan od razu.
- **Linia 243:** Pobierz aktualny czas.
- **Linia 244:** Jeśli proszono o odczyt albo nadszedł czas...
- **Linia 245:** ...opuść flagę prośby...
- **Linia 246:** ...i ustal czas następnego odczytu.
- **Linia 247:** Próbuj.
- **Linia 248:** Wyślij komendę `GET`.
- **Linia 249:** Jeśli brak połączenia...
- **Linia 250:** ...zapisz ostrzeżenie.
- **Linia 251:** Jeśli układ zgłosił błąd...
- **Linia 252:** ...zapisz ostrzeżenie.
- **Linia 253:** Czekaj do następnego odczytu, ale obudź się wcześniej, jeśli ktoś podniesie flagę prośby.

### Łączenie i rozłączanie

```python
255      def _try_connect(self) -> None:
256          try:
257              self.transport.open()
258          except TransportError as e:
259              log.debug("połączenie: %s", e)
260              return
261          self._link_open.set()
262          # Pierwsza linia po otwarciu portu bywa uszkodzona, więc PING ponawiamy
263          for _ in range(3):
264              try:
265                  if self.command("PING").kind == "PONG":
266                      break
267              except (NotConnected, DeviceError):
268                  continue
269          else:
270              self._disconnect("mikrokontroler nie odpowiada na PING")
271              return
272          try:
273              ver = self.command("VER")
274              self.firmware = ver.name or self.firmware
275          except (NotConnected, DeviceError):
276              pass
277          log.info("połączono: %s (firmware %s)", self.transport.port, self.firmware)
278          self._add_log(f"Połączono z mikrokontrolerem ({self.transport.port})", "server")
279          self._update({"connected": True})
280          self._connected.set()
```

- **Linia 255:** Metoda próbująca się połączyć.
- **Linia 256:** Próbuj.
- **Linia 257:** Otwórz łącze.
- **Linia 258:** Jeśli się nie udało...
- **Linia 259:** ...zapisz komunikat diagnostyczny...
- **Linia 260:** ...i zakończ.
- **Linia 261:** Oznacz, że łącze jest otwarte.
- **Linia 262:** Komentarz: pierwsza linia po otwarciu portu bywa uszkodzona, więc `PING` ponawiamy.
- **Linia 263:** Trzy próby. Podkreślenie oznacza, że numer próby nas nie interesuje.
- **Linia 264:** Próbuj.
- **Linia 265:** Jeśli odpowiedź na `PING` to `PONG`...
- **Linia 266:** ...przerwij pętlę, układ żyje.
- **Linia 267:** Jeśli wystąpił błąd...
- **Linia 268:** ...przejdź do następnej próby.
- **Linia 269:** Gałąź `else` pętli wykona się tylko wtedy, gdy nie było `break`, czyli wszystkie próby się nie udały:
- **Linia 270:** Oznacz rozłączenie...
- **Linia 271:** ...i zakończ.
- **Linia 272:** Próbuj.
- **Linia 273:** Zapytaj o wersję.
- **Linia 274:** Zapamiętaj wersję, a jeśli jest pusta, zostaw dotychczasową.
- **Linia 275:** Jeśli wystąpił błąd...
- **Linia 276:** ...zignoruj go. Wersja nie jest konieczna do pracy.
- **Linia 277:** Zapisz komunikat o połączeniu.
- **Linia 278:** Dopisz wpis do dziennika.
- **Linia 279:** Zaktualizuj stan i powiadom przeglądarki.
- **Linia 280:** Podnieś flagę połączenia.

```python
282      def _disconnect(self, reason: str) -> None:
283          if not self._link_open.is_set():
284              return
285          self._link_open.clear()
286          was_ready = self._connected.is_set()
287          self._connected.clear()
288          self.transport.close()
289          if not was_ready:
290              log.info("nieudane połączenie: %s", reason)
291              return
292          log.warning("rozłączono: %s", reason)
293          self._add_log(f"Utracono połączenie: {reason}", "server")
294          self._update({"connected": False})
```

- **Linia 282:** Metoda oznaczająca rozłączenie. Dostaje przyczynę.
- **Linia 283:** Jeśli łącze już jest zamknięte...
- **Linia 284:** ...nic nie rób.
- **Linia 285:** Opuść flagę otwartego łącza.
- **Linia 286:** Zapamiętaj, czy połączenie było w pełni gotowe.
- **Linia 287:** Opuść flagę połączenia.
- **Linia 288:** Zamknij łącze.
- **Linia 289:** Jeśli połączenie nie było gotowe, czyli była to tylko nieudana próba...
- **Linia 290:** ...zapisz spokojny komunikat...
- **Linia 291:** ...i zakończ, nie niepokojąc użytkownika strony.
- **Linia 292:** Zapisz ostrzeżenie o rozłączeniu.
- **Linia 293:** Dopisz wpis do dziennika.
- **Linia 294:** Zaktualizuj stan i powiadom przeglądarki.

---

[← Poprzedni](32-serwer-transport.md) · [Spis treści](README.md) · [Następny →](34-serwer-routes.md)
