# Testy: `test_api.py`, sprawdzanie serwera

[← Poprzedni](50-testy-conftest-protocol.md) · [Spis treści](README.md) · [Następny →](52-testy-mcu-sim.md)

**Po co ten plik:** sprawdza serwer „z zewnątrz”, tak jak robi to przeglądarka: wysyła zapytania i sprawdza odpowiedzi. Układem jest tu symulator w Pythonie, więc sprzęt nie jest potrzebny.

## `server/tests/test_api.py`

[Otwórz ten plik w repozytorium](../../server/tests/test_api.py)

```python
  1  import json
  2  import time
  3
  4  from app.protocol import parse_line
```

- **Linia 1:** Biblioteka JSON.
- **Linia 2:** Biblioteka czasu.
- **Linia 4:** Parser linii protokołu.

```python
  7  def wait_for(predicate, timeout=2.0):
  8      deadline = time.monotonic() + timeout
  9      while time.monotonic() < deadline:
 10          if predicate():
 11              return True
 12          time.sleep(0.02)
 13      return False
```

- **Linia 7:** Funkcja pomocnicza czekająca, aż warunek stanie się prawdziwy. Dostaje funkcję sprawdzającą i limit czasu. Serwer działa w wątkach, więc zmiany nie zachodzą natychmiast.
- **Linia 8:** Oblicz chwilę końca czekania.
- **Linia 9:** Dopóki czas nie minął...
- **Linia 10:** ...jeśli warunek jest spełniony...
- **Linia 11:** ...zwróć prawdę.
- **Linia 12:** Odczekaj 0,02 s przed kolejnym sprawdzeniem.
- **Linia 13:** Czas minął, zwróć fałsz.

### Włączanie i wyłączanie

```python
 16  def test_on_off_compat(client, device):
 17      _, sim = device
 18      r = client.get("/on")
 19      assert r.status_code == 200
 20      assert r.json["on"] is True and sim.on
 21      r = client.get("/off")
 22      assert r.json["on"] is False and not sim.on
```

- **Linia 16:** Test adresów `/on` i `/off`.
- **Linia 17:** Pobierz symulator. Podkreślenie oznacza, że `Device` nie jest tu potrzebny.
- **Linia 18:** Wyślij zapytanie `GET /on`.
- **Linia 19:** Kod odpowiedzi musi być 200.
- **Linia 20:** W odpowiedzi `on` musi być prawdą, a wyjście symulatora włączone.
- **Linia 21:** Wyślij zapytanie `GET /off`.
- **Linia 22:** Teraz wszystko musi być wyłączone.

### Odczyt i zmiana stanu

```python
 25  def test_get_state(client):
 26      r = client.get("/api/state?refresh=1")
 27      assert r.status_code == 200
 28      data = r.json
 29      for key in ("connected", "on", "pwm", "mode", "pot", "ldr", "temp", "alarm", "tlim", "uptime_s"):
 30          assert key in data
 31      assert data["connected"] is True
 32      assert data["pot"] == 512
```

- **Linia 25:** Test odczytu stanu.
- **Linia 26:** Zapytaj o stan, wymuszając odczyt z układu.
- **Linia 27:** Kod musi być 200.
- **Linia 28:** Pobierz JSON z odpowiedzi.
- **Linia 29:** Dla każdej wymaganej nazwy pola...
- **Linia 30:** ...sprawdź, czy jest w odpowiedzi.
- **Linia 31:** Połączenie musi istnieć.
- **Linia 32:** Potencjometr musi mieć wartość startową symulatora.

```python
 35  def test_post_state(client, device):
 36      _, sim = device
 37      r = client.post("/api/state", json={"on": True, "pwm": 60, "tlim": 45})
 38      assert r.status_code == 200
 39      assert (r.json["on"], r.json["pwm"], r.json["tlim"]) == (True, 60, 45)
 40      assert (sim.on, sim.pwm, sim.tlim) == (True, 60, 45)
 41      r = client.post("/api/state", json={"mode": "auto"})
 42      assert r.json["mode"] == "AUTO"
```

- **Linia 35:** Test zmiany stanu.
- **Linia 36:** Pobierz symulator.
- **Linia 37:** Wyślij zmianę trzech wartości.
- **Linia 38:** Kod musi być 200.
- **Linia 39:** Odpowiedź musi zawierać nowe wartości.
- **Linia 40:** Symulator też musi je mieć.
- **Linia 41:** Wyślij zmianę trybu małymi literami.
- **Linia 42:** Tryb musi zostać rozpoznany i zamieniony na wielkie litery.

### Błędne dane

```python
 45  def test_post_state_validation(client):
 46      assert client.post("/api/state", json={"pwm": 101}).status_code == 400
 47      assert client.post("/api/state", json={"pwm": "60"}).status_code == 400
 48      assert client.post("/api/state", json={"on": 1}).status_code == 400
 49      assert client.post("/api/state", json={"mode": "FAST"}).status_code == 400
 50      assert client.post("/api/state", json={"color": "red"}).status_code == 400
 51      assert client.post("/api/state", data="xyz").status_code == 400
 52      assert client.post("/api/state", json={}).status_code == 400
```

- **Linia 45:** Test odrzucania złych danych. Każdy przypadek ma dać kod 400.
- **Linia 46:** Jasność poza zakresem.
- **Linia 47:** Jasność jako tekst.
- **Linia 48:** Liczba zamiast prawdy lub fałszu.
- **Linia 49:** Nieznany tryb.
- **Linia 50:** Nieznane pole.
- **Linia 51:** Dane niebędące JSON-em.
- **Linia 52:** Pusty obiekt.

### Stan serwera i przycisk

```python
 55  def test_health(client):
 56      r = client.get("/api/health")
 57      assert r.json["ok"] and r.json["connected"] and r.json["simulated"]
 58      assert r.json["firmware"].startswith("0.10.0")
```

- **Linia 55:** Test adresu `/api/health`.
- **Linia 56:** Wyślij zapytanie.
- **Linia 57:** Serwer działa, ma połączenie i jest w trybie symulacji.
- **Linia 58:** Wersja firmware zaczyna się od `0.10.0`.

```python
 61  def test_local_button_reaches_state(client, device):
 62      dev, sim = device
 63      client.get("/off")
 64      sim.press_button()
 65      assert wait_for(lambda: dev.snapshot()["on"] is True)
 66      assert any(e["text"] == "EVT BTN on=1" for e in client.get("/api/log").json)
```

- **Linia 61:** Test: naciśnięcie przycisku dociera do serwera.
- **Linia 62:** Pobierz oba obiekty.
- **Linia 63:** Wyłącz wyjście.
- **Linia 64:** Naciśnij wirtualny przycisk.
- **Linia 65:** Poczekaj, aż stan w serwerze się zmieni.
- **Linia 66:** W dzienniku musi być wpis o zdarzeniu. `any` sprawdza, czy pasuje choć jeden wpis.

### Wirtualna płytka

```python
 69  def test_sim_endpoints(client, device):
 70      dev, _ = device
 71      r = client.post("/api/sim", json={"ldr": 700, "temp": 23.5})
 72      assert r.status_code == 200 and r.json["ldr"] == 700
 73      assert wait_for(lambda: dev.snapshot()["ldr"] == 700 and dev.snapshot()["temp"] == 23.5)
 74      assert client.post("/api/sim", json={"button": "double"}).status_code == 400
```

- **Linia 69:** Test adresów `/api/sim`.
- **Linia 70:** Pobierz obiekt `Device`.
- **Linia 71:** Ustaw fotorezystor i temperaturę.
- **Linia 72:** Kod musi być 200, a wartość się zgadzać.
- **Linia 73:** Poczekaj, aż nowe wartości dotrą do serwera.
- **Linia 74:** Nieznany rodzaj naciśnięcia ma dać kod 400.

### Strumień zdarzeń

```python
 77  def test_sse_init_and_updates(client):
 78      r = client.get("/api/events", buffered=False)
 79      assert r.mimetype == "text/event-stream"
 80      chunks = r.response
 81      first = next(chunks).decode()
 82      assert first.startswith("event: init\n")
 83      init = json.loads(first.split("data: ", 1)[1])
 84      assert init["simulated"] is True and "state" in init
 85      client.post("/api/state", json={"pwm": 33})
 86      for _ in range(20):
 87          chunk = next(chunks).decode()
 88          if chunk.startswith("event: state") and '"pwm": 33' in chunk:
 89              break
 90      else:
 91          raise AssertionError("brak zdarzenia state z pwm=33")
 92      r.close()
```

- **Linia 77:** Test strumienia SSE.
- **Linia 78:** Otwórz strumień bez buforowania.
- **Linia 79:** Typ odpowiedzi musi oznaczać strumień zdarzeń.
- **Linia 80:** Pobierz iterator, czyli obiekt dający kolejne kawałki odpowiedzi.
- **Linia 81:** Odczytaj pierwszy kawałek i zamień bajty na tekst.
- **Linia 82:** Musi to być wiadomość `init`.
- **Linia 83:** Odetnij część od `data: ` i zamień JSON na obiekt.
- **Linia 84:** Sprawdź zawartość tej wiadomości.
- **Linia 85:** Zmień jasność, co powinno wywołać wiadomość.
- **Linia 86:** Sprawdź najwyżej 20 kolejnych kawałków.
- **Linia 87:** Odczytaj kawałek.
- **Linia 88:** Jeśli to wiadomość o stanie z oczekiwaną jasnością...
- **Linia 89:** ...przerwij pętlę, sukces.
- **Linia 90:** Gałąź `else` pętli wykona się tylko, gdy nie było `break`:
- **Linia 91:** Zgłoś błąd testu.
- **Linia 92:** Zamknij strumień.

### Brak układu

```python
 95  def test_offline_returns_503(offline_client):
 96      assert offline_client.get("/on").status_code == 503
 97      assert offline_client.post("/api/state", json={"on": True}).status_code == 503
 98      r = offline_client.get("/api/state")
 99      assert r.status_code == 200 and r.json["connected"] is False
100      assert offline_client.get("/api/health").json["connected"] is False
101      assert offline_client.get("/api/sim").status_code == 404
```

- **Linia 95:** Test zachowania bez układu.
- **Linia 96:** `/on` ma dać kod 503.
- **Linia 97:** Zmiana stanu też.
- **Linia 98:** Odczyt ostatniego stanu działa...
- **Linia 99:** ...i pokazuje brak połączenia.
- **Linia 100:** Adres `/api/health` pokazuje to samo.
- **Linia 101:** Adres symulacji poza trybem symulacji daje kod 404.

### Śmieci na łączu

```python
104  def test_stale_and_broken_lines_are_skipped(device):
105      dev, _ = device
106      # Urwana linia z otwarcia portu w trakcie nadawania i spóźnione PONG
107      dev._responses.put(parse_line("OT reset=POR ver=0.10.0"))
108      original_write = dev.transport.write_line
109
110      def write_with_noise(line):
111          original_write(line)
112          dev._responses.put(parse_line("PONG"))
113
114      dev.transport.write_line = write_with_noise
115      try:
116          assert dev.command("VER").name.startswith("0.10.0")
117          assert dev.command("ON").fields == {"on": "1"}
118      finally:
119          dev.transport.write_line = original_write
```

- **Linia 104:** Test: urwane i spóźnione linie są pomijane.
- **Linia 105:** Pobierz obiekt `Device`.
- **Linia 106:** Komentarz: symulujemy urwaną linię z otwarcia portu w trakcie nadawania oraz spóźnione `PONG`.
- **Linia 107:** Włóż urwaną linię do kolejki odpowiedzi.
- **Linia 108:** Zapamiętaj oryginalną metodę wysyłania.
- **Linia 110:** Nowa metoda wysyłania, która dokłada zakłócenie.
- **Linia 111:** Wyślij naprawdę.
- **Linia 112:** Dorzuć do kolejki fałszywe `PONG`.
- **Linia 114:** Podmień metodę w obiekcie łącza.
- **Linia 115:** Próbuj, a na końcu posprzątaj.
- **Linia 116:** Komenda `VER` mimo śmieci dostaje właściwą odpowiedź.
- **Linia 117:** Komenda `ON` także.
- **Linia 118:** Na końcu, także przy błędzie:
- **Linia 119:** Przywróć oryginalną metodę.

---

[← Poprzedni](50-testy-conftest-protocol.md) · [Spis treści](README.md) · [Następny →](52-testy-mcu-sim.md)
