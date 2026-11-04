# Serwer: `routes.py`, adresy strony i API

[← Poprzedni](33-serwer-device.md) · [Spis treści](README.md) · [Następny →](35-serwer-mcu-sim.md)

**Po co ten plik:** łączy adresy z funkcjami. Gdy przeglądarka poprosi np. o `/api/state`, Flask uruchomi przypisaną funkcję, a jej wynik odeśle. Tu też trafiają błędy, które zamieniamy na właściwe kody HTTP. Pojęcia są w [podstawach WWW](03-podstawy-www.md).

## `server/app/routes.py`

[Otwórz ten plik w repozytorium](../../server/app/routes.py)

```python
  1  """Strona WWW, API JSON i strumień SSE."""
  2
  3  from __future__ import annotations
  4
  5  import json
  6  import queue
  7  import time
  8
  9  from flask import Blueprint, Response, current_app, jsonify, request
 10
 11  from .device import Device, DeviceError, NotConnected
 12
 13  bp = Blueprint("main", __name__)
 14
 15  SSE_KEEPALIVE = 15
```

- **Linia 1:** Docstring: strona WWW, API JSON i strumień zdarzeń SSE.
- **Linia 3:** Nowszy zapis podpowiedzi typów.
- **Linia 5:** Biblioteka zamiany danych na JSON.
- **Linia 6:** Biblioteka kolejek. Potrzebna tu jest jej nazwa błędu `queue.Empty`.
- **Linia 7:** Biblioteka czasu.
- **Linia 9:** Pobiera z Flaska: zestaw adresów (`Blueprint`), odpowiedź (`Response`), bieżącą aplikację (`current_app`), funkcję tworzącą JSON (`jsonify`) i dane zapytania (`request`).
- **Linia 11:** Pobiera naszą klasę `Device` i rodzaje błędów.
- **Linia 13:** Tworzy zestaw adresów o nazwie `main`.
- **Linia 15:** Co 15 s wyślemy w strumieniu komentarz, by połączenie nie wygasło.

### Funkcje pomocnicze

```python
 18  def _device() -> Device:
 19      return current_app.extensions["device"]
 20
 21
 22  def _sim():
 23      return current_app.extensions.get("sim")
 24
 25
 26  def _error(message: str, status: int):
 27      return jsonify({"error": message}), status
```

- **Linia 18:** Funkcja zwracająca obiekt `Device`.
- **Linia 19:** Bierze go z aplikacji, gdzie odłożył go `create_app`.
- **Linia 22:** Funkcja zwracająca symulator.
- **Linia 23:** Bierze go z aplikacji. `get` da `None`, jeśli go nie ma.
- **Linia 26:** Funkcja tworząca odpowiedź z błędem.
- **Linia 27:** Zwraca parę: JSON z opisem i kod HTTP.

### Zamiana błędów na kody HTTP

```python
 30  @bp.errorhandler(NotConnected)
 31  def _not_connected(e):
 32      return _error(str(e), 503)
 33
 34
 35  @bp.errorhandler(DeviceError)
 36  def _device_error(e):
 37      return _error(str(e), 502)
 38
 39
 40  @bp.errorhandler(ValueError)
 41  def _bad_value(e):
 42      return _error(str(e), 400)
```

- **Linia 30:** Gdy gdziekolwiek w tych adresach wystąpi błąd `NotConnected`, Flask wywoła poniższą funkcję.
- **Linia 31:** Funkcja dostaje obiekt błędu.
- **Linia 32:** Odsyła kod 503 i treść błędu.
- **Linia 35:** To samo dla błędu `DeviceError`.
- **Linia 36:** Funkcja obsługi.
- **Linia 37:** Odsyła kod 502.
- **Linia 40:** To samo dla błędu `ValueError`.
- **Linia 41:** Funkcja obsługi.
- **Linia 42:** Odsyła kod 400.

### Strona i proste adresy

```python
 45  @bp.get("/")
 46  def index():
 47      return current_app.send_static_file("index.html")
 48
 49
 50  # Zgodność z instrukcją (etap 3): http://[IP_MALINY]:5000/on
 51  @bp.get("/on")
 52  def on():
 53      return jsonify(_device().set_state(on=True))
 54
 55
 56  @bp.get("/off")
 57  def off():
 58      return jsonify(_device().set_state(on=False))
```

- **Linia 45:** Gdy przyjdzie GET o adres główny `/`...
- **Linia 46:** ...uruchom funkcję `index`.
- **Linia 47:** Odeślij plik `index.html` z katalogu `static`.
- **Linia 50:** Komentarz: zgodność z instrukcją do etapu 3, czyli adres `/on`.
- **Linia 51:** Gdy przyjdzie GET o `/on`...
- **Linia 52:** ...uruchom funkcję `on`.
- **Linia 53:** Włącz wyjście i odeślij nowy stan jako JSON.
- **Linia 56:** Gdy przyjdzie GET o `/off`...
- **Linia 57:** ...uruchom funkcję `off`.
- **Linia 58:** Wyłącz wyjście i odeślij nowy stan.

### Odczyt stanu

```python
 61  @bp.get("/api/state")
 62  def get_state():
 63      device = _device()
 64      if request.args.get("refresh") == "1":
 65          return jsonify(device.refresh())
 66      return jsonify(device.snapshot())
```

- **Linia 61:** Gdy przyjdzie GET o `/api/state`...
- **Linia 62:** ...uruchom funkcję `get_state`.
- **Linia 63:** Pobierz obiekt `Device`.
- **Linia 64:** Jeśli w adresie jest `?refresh=1`...
- **Linia 65:** ...odczytaj stan z układu od razu i odeślij.
- **Linia 66:** W przeciwnym razie odeślij ostatnio znany stan.

### Zmiana stanu

```python
 69  @bp.post("/api/state")
 70  def post_state():
 71      data = request.get_json(silent=True)
 72      if not isinstance(data, dict) or not data:
 73          return _error("oczekiwano obiektu JSON, np. {\"on\": true}", 400)
 74      unknown = set(data) - {"on", "pwm", "mode", "tlim"}
 75      if unknown:
 76          return _error(f"nieznane pola: {', '.join(sorted(unknown))}", 400)
 77      changes = {}
 78      if "on" in data:
 79          if not isinstance(data["on"], bool):
 80              return _error("on musi być true albo false", 400)
 81          changes["on"] = data["on"]
 82      for key in ("pwm", "tlim"):
 83          if key in data:
 84              if not isinstance(data[key], int) or isinstance(data[key], bool):
 85                  return _error(f"{key} musi być liczbą całkowitą", 400)
 86              changes[key] = data[key]
 87      if "mode" in data:
 88          if not isinstance(data["mode"], str):
 89              return _error("mode musi być tekstem", 400)
 90          changes["mode"] = data["mode"].upper()
 91      return jsonify(_device().set_state(**changes))
```

- **Linia 69:** Gdy przyjdzie POST na `/api/state`...
- **Linia 70:** ...uruchom funkcję `post_state`.
- **Linia 71:** Odczytaj JSON z zapytania. `silent=True`: przy błędzie zwróć `None` zamiast zgłaszać wyjątek.
- **Linia 72:** Jeśli to nie jest słownik albo jest pusty...
- **Linia 73:** ...odeślij kod 400 z opisem. `\"` to cudzysłów wewnątrz napisu.
- **Linia 74:** Znajdź nieznane pola: wszystkie klucze minus dozwolone.
- **Linia 75:** Jeśli są nieznane pola...
- **Linia 76:** ...odeślij kod 400 z ich listą.
- **Linia 77:** Pusty słownik na zmiany.
- **Linia 78:** Jeśli podano pole `on`...
- **Linia 79:** Jeśli nie jest to prawda ani fałsz...
- **Linia 80:** ...odeślij kod 400.
- **Linia 81:** Zapisz zmianę.
- **Linia 82:** Dla pól `pwm` i `tlim`...
- **Linia 83:** ...jeśli pole podano...
- **Linia 84:** ...i nie jest liczbą całkowitą. Osobno wykluczamy `bool`, bo w Pythonie prawda liczy się też jako liczba.
- **Linia 85:** Odeślij kod 400.
- **Linia 86:** Zapisz zmianę.
- **Linia 87:** Jeśli podano pole `mode`...
- **Linia 88:** Jeśli nie jest napisem...
- **Linia 89:** ...odeślij kod 400.
- **Linia 90:** Zapisz zmianę, zamieniając litery na wielkie.
- **Linia 91:** Wykonaj zmiany i odeślij nowy stan. `**changes` rozkłada słownik na nazwane parametry.

### Stan serwera i dziennik

```python
 94  @bp.get("/api/health")
 95  def health():
 96      device = _device()
 97      return jsonify({
 98          "ok": True,
 99          "connected": device.connected,
100          "simulated": bool(device.simulated),
101          "simulator": device.simulated or None,
102          "port": device.transport.port,
103          "firmware": device.firmware,
104          "server_uptime_s": int(time.time() - device.started_at),
105      })
106
107
108  @bp.get("/api/log")
109  def get_log():
110      return jsonify(_device().recent_log())
```

- **Linia 94:** Gdy przyjdzie GET o `/api/health`...
- **Linia 95:** ...uruchom funkcję `health`.
- **Linia 96:** Pobierz obiekt `Device`.
- **Linia 97:** Odeślij JSON z polami:
- **Linia 98:** Serwer działa.
- **Linia 99:** Czy jest połączenie z układem.
- **Linia 100:** Czy to symulacja, jako prawda lub fałsz.
- **Linia 101:** Rodzaj symulacji albo `None`.
- **Linia 102:** Nazwa użytego portu.
- **Linia 103:** Wersja firmware.
- **Linia 104:** Czas działania serwera w sekundach: teraz minus chwila startu.
- **Linia 105:** Koniec odpowiedzi.
- **Linia 108:** Gdy przyjdzie GET o `/api/log`...
- **Linia 109:** ...uruchom funkcję `get_log`.
- **Linia 110:** Odeślij ostatnie wpisy dziennika.

### Strumień zdarzeń SSE

Przeglądarka otwiera to połączenie raz i trzyma je cały czas otwarte. Wysyłamy przez nie wiadomości, gdy coś się zmieni.

```python
113  def _sse(event: str, data) -> str:
114      return f"event: {event}\ndata: {json.dumps(data, ensure_ascii=False)}\n\n"
```

- **Linia 113:** Funkcja buduje jedną wiadomość w formacie SSE.
- **Linia 114:** Format wymaga wierszy `event:` i `data:` oraz pustego wiersza na końcu. Dane zamieniamy na JSON, zachowując polskie znaki.

```python
117  @bp.get("/api/events")
118  def events():
119      device = _device()
120      q = device.subscribe()
121      sim = _sim()
122
123      def stream():
124          try:
125              yield _sse("init", {
126                  "state": device.snapshot(),
127                  "log": device.recent_log(),
128                  "simulated": bool(device.simulated),
129                  "simulator": device.simulated or None,
130                  "firmware": device.firmware,
131                  "sim": sim.snapshot() if sim else None,
132              })
133              while True:
134                  try:
135                      kind, data = q.get(timeout=SSE_KEEPALIVE)
136                  except queue.Empty:
137                      yield ": keepalive\n\n"
138                      continue
139                  yield _sse(kind, data)
140          finally:
141              device.unsubscribe(q)
142
143      return Response(stream(), mimetype="text/event-stream",
144                      headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"})
```

- **Linia 117:** Gdy przyjdzie GET o `/api/events`...
- **Linia 118:** ...uruchom funkcję `events`.
- **Linia 119:** Pobierz obiekt `Device`.
- **Linia 120:** Zapisz tę przeglądarkę na aktualizacje.
- **Linia 121:** Pobierz symulator.
- **Linia 123:** Funkcja wewnętrzna: **generator**, który będzie wysyłał kolejne fragmenty odpowiedzi.
- **Linia 124:** Próbuj, a na końcu posprzątaj.
- **Linia 125:** Najpierw wyślij wiadomość `init` z pełnym obrazem sytuacji.
- **Linia 126:** Aktualny stan.
- **Linia 127:** Dziennik.
- **Linia 128:** Czy to symulacja.
- **Linia 129:** Rodzaj symulacji.
- **Linia 130:** Wersja firmware.
- **Linia 131:** Stan wirtualnej płytki, jeśli jest symulator.
- **Linia 132:** Koniec wiadomości `init`.
- **Linia 133:** Pętla bez końca, bo połączenie ma trwać.
- **Linia 134:** Próbuj.
- **Linia 135:** Czekaj na wiadomość, najwyżej 15 s. Dostaniemy parę: rodzaj i dane.
- **Linia 136:** Jeśli czas minął, a wiadomości nie ma...
- **Linia 137:** ...wyślij komentarz podtrzymujący połączenie. Linia zaczynająca się od dwukropka to w SSE komentarz.
- **Linia 138:** Wróć na początek pętli.
- **Linia 139:** Wyślij wiadomość do przeglądarki.
- **Linia 140:** Gdy połączenie się zakończy, np. po zamknięciu karty...
- **Linia 141:** ...wypisz przeglądarkę z aktualizacji.
- **Linia 143:** Zwróć odpowiedź, której treścią jest generator. Typ `text/event-stream` oznacza strumień SSE.
- **Linia 144:** Nagłówki zakazują buforowania, żeby wiadomości szły od razu: przeglądarce (`Cache-Control`) i pośrednikom, np. serwerowi nginx (`X-Accel-Buffering`).

### Wirtualna płytka

Te adresy działają tylko w trybie symulacji. Zastępują fizyczny przycisk i czujniki.

```python
147  # --- wirtualna płytka, tylko w trybie symulacji ------------------------------
148
149  @bp.get("/api/sim")
150  def get_sim():
151      sim = _sim()
152      if sim is None:
153          return _error("serwer nie działa w trybie symulacji", 404)
154      return jsonify(sim.snapshot())
```

- **Linia 147:** Komentarz oddzielający tę część.
- **Linia 149:** Gdy przyjdzie GET o `/api/sim`...
- **Linia 150:** ...uruchom funkcję `get_sim`.
- **Linia 151:** Pobierz symulator.
- **Linia 152:** Jeśli go nie ma...
- **Linia 153:** ...odeślij kod 404.
- **Linia 154:** Odeślij stan wirtualnej płytki.

```python
157  @bp.post("/api/sim")
158  def post_sim():
159      sim = _sim()
160      if sim is None:
161          return _error("serwer nie działa w trybie symulacji", 404)
162      data = request.get_json(silent=True)
163      if not isinstance(data, dict):
164          return _error("oczekiwano obiektu JSON", 400)
165      try:
166          sim.set_inputs(pot=data.get("pot"), ldr=data.get("ldr"), temp_c=data.get("temp"))
167      except (TypeError, ValueError):
168          return _error("pot i ldr: 0–1023, temp: liczba", 400)
169      button = data.get("button")
170      if button in ("short", "long"):
171          sim.press_button(long=button == "long")
172      elif button is not None:
173          return _error("button: short albo long", 400)
174      if data.get("reset"):
175          sim.reset("POR" if data.get("reset") == "power" else "EXT")
176      _device().request_refresh()
177      return jsonify(sim.snapshot())
```

- **Linia 157:** Gdy przyjdzie POST na `/api/sim`...
- **Linia 158:** ...uruchom funkcję `post_sim`.
- **Linia 159:** Pobierz symulator.
- **Linia 160:** Jeśli go nie ma...
- **Linia 161:** ...odeślij kod 404.
- **Linia 162:** Odczytaj JSON z zapytania.
- **Linia 163:** Jeśli to nie jest słownik...
- **Linia 164:** ...odeślij kod 400.
- **Linia 165:** Próbuj.
- **Linia 166:** Ustaw wejścia symulatora. Pól niepodanych funkcja `get` da `None`, więc zostaną bez zmian.
- **Linia 167:** Jeśli wartości są błędne...
- **Linia 168:** ...odeślij kod 400.
- **Linia 169:** Odczytaj pole `button`.
- **Linia 170:** Jeśli to `short` albo `long`...
- **Linia 171:** ...naciśnij wirtualny przycisk. Parametr `long` jest prawdą dla długiego naciśnięcia.
- **Linia 172:** Jeśli podano coś innego...
- **Linia 173:** ...odeślij kod 400.
- **Linia 174:** Jeśli zażądano resetu...
- **Linia 175:** ...wykonaj go: `power` oznacza zanik zasilania, inna wartość reset zewnętrzny.
- **Linia 176:** Poproś o szybki odczyt stanu.
- **Linia 177:** Odeślij stan wirtualnej płytki.

---

[← Poprzedni](33-serwer-device.md) · [Spis treści](README.md) · [Następny →](35-serwer-mcu-sim.md)
