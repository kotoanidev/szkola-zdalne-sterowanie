# Testy: `test_firmware_avr.py`, sprawdzanie prawdziwego firmware

[← Poprzedni](52-testy-mcu-sim.md) · [Spis treści](README.md) · [Następny →](60-sim-makefile.md)

**Po co ten plik:** najcenniejsze testy, bo uruchamiają **prawdziwy** `firmware.hex` w emulatorze simavr. Sprawdzają, czy kod w C zachowuje się zgodnie z umową, i to bez fizycznej płytki. Wymagają jednak zbudowania emulatora, a bez niego są pomijane.

## `server/tests/test_firmware_avr.py`

[Otwórz ten plik w repozytorium](../../server/tests/test_firmware_avr.py)

```python
  1  """Testy prawdziwego firmware (firmware/build/firmware.hex) uruchomionego w simavr.
  2
  3  Wymagają zbudowanego symulatora: make -C sim. Bez niego są pomijane.
  4  """
```

- **Linia 1:** Początek docstringa: testy prawdziwego firmware uruchomionego w simavr.
- **Linia 3:** Wymagają zbudowanego symulatora, a bez niego są pomijane.
- **Linia 4:** Koniec docstringa.

```python
  6  import re
  7  import time
  8
  9  import pytest
 10  import serial
 11
 12  from app import create_app
 13  from app.avrsim import DEFAULT_BINARY, DEFAULT_HEX, AvrSim
 14  from app.config import Config
 15  from app.device import Device
 16  from app.transport import SerialTransport
```

- **Linia 6:** Biblioteka wyrażeń regularnych.
- **Linia 7:** Biblioteka czasu.
- **Linia 9:** Biblioteka testów.
- **Linia 10:** Biblioteka portu szeregowego.
- **Linia 12:** Funkcja tworząca aplikację.
- **Linia 13:** Ścieżki domyślne i klasa emulatora.
- **Linia 14:** Konfiguracja.
- **Linia 15:** Obiekt obsługi układu.
- **Linia 16:** Prawdziwe łącze szeregowe.

```python
 18  pytestmark = pytest.mark.skipif(
 19      not (DEFAULT_BINARY.exists() and DEFAULT_HEX.exists()),
 20      reason="brak sim/build/avrsim lub firmware.hex – uruchom: make -C sim",
 21  )
```

- **Linia 18:** Reguła dla całego pliku: pomiń testy, jeśli...
- **Linia 19:** ...nie ma programu emulatora albo pliku firmware.
- **Linia 20:** Podaj powód i podpowiedź.
- **Linia 21:** Koniec reguły.

### Terminal szeregowy

```python
 24  class Link:
 25      """Terminal szeregowy podłączony do UART emulowanego ATmega32A."""
 26
 27      def __init__(self, port: str) -> None:
 28          self.port = serial.Serial(port, 9600, timeout=0.05)
 29          self.events: list[str] = []
```

- **Linia 24:** Klasa udająca terminal podłączony do układu.
- **Linia 25:** Docstring: terminal szeregowy podłączony do UART emulowanego ATmega32A.
- **Linia 27:** Funkcja tworząca obiekt. Dostaje nazwę portu.
- **Linia 28:** Otwórz port: 9600 bitów na sekundę, odczyt czeka najwyżej 0,05 s.
- **Linia 29:** Lista odebranych zdarzeń.

```python
 31      def read_line(self, timeout: float) -> str | None:
 32          deadline = time.monotonic() + timeout
 33          data = b""
 34          while time.monotonic() < deadline:
 35              data += self.port.readline()
 36              if data.endswith(b"\n"):
 37                  return data.decode("ascii").strip()
 38          return None
```

- **Linia 31:** Metoda czytająca jedną linię, z limitem czasu.
- **Linia 32:** Oblicz chwilę końca czekania.
- **Linia 33:** Puste bajty na początek.
- **Linia 34:** Dopóki czas nie minął...
- **Linia 35:** ...dopisz to, co udało się odczytać.
- **Linia 36:** Jeśli na końcu jest koniec linii...
- **Linia 37:** ...zamień bajty na tekst, usuń białe znaki i zwróć.
- **Linia 38:** Czas minął, zwróć `None`.

```python
 40      def response(self, timeout: float = 1.0) -> str:
 41          deadline = time.monotonic() + timeout
 42          while (line := self.read_line(deadline - time.monotonic())) is not None:
 43              if line.startswith("EVT"):
 44                  self.events.append(line)
 45              else:
 46                  return line
 47          raise AssertionError("brak odpowiedzi mikrokontrolera")
```

- **Linia 40:** Metoda czekająca na odpowiedź na komendę.
- **Linia 41:** Oblicz chwilę końca czekania.
- **Linia 42:** Czytaj linie, dopóki jakieś są.
- **Linia 43:** Jeśli to zdarzenie...
- **Linia 44:** ...odłóż je na później.
- **Linia 45:** W przeciwnym razie:
- **Linia 46:** To odpowiedź, zwróć ją.
- **Linia 47:** Nie było odpowiedzi, zgłoś błąd testu.

```python
 49      def cmd(self, text: str) -> str:
 50          self.port.write(text.encode("ascii") + b"\n")
 51          return self.response()
 52
 53      def raw(self, data: bytes) -> str:
 54          self.port.write(data)
 55          return self.response()
```

- **Linia 49:** Metoda wysyłająca komendę.
- **Linia 50:** Wyślij tekst z końcem linii.
- **Linia 51:** Zwróć odpowiedź.
- **Linia 53:** Metoda wysyłająca surowe bajty, bez końca linii.
- **Linia 54:** Wyślij bajty.
- **Linia 55:** Zwróć odpowiedź.

```python
 57      def wait_event(self, prefix: str, timeout: float = 1.0) -> str:
 58          deadline = time.monotonic() + timeout
 59          while True:
 60              for i, line in enumerate(self.events):
 61                  if line.startswith(prefix):
 62                      return self.events.pop(i)
 63              remaining = deadline - time.monotonic()
 64              if remaining <= 0:
 65                  raise AssertionError(f"brak zdarzenia {prefix}")
 66              line = self.read_line(remaining)
 67              if line is not None:
 68                  self.events.append(line)
```

- **Linia 57:** Metoda czekająca na zdarzenie o podanym początku.
- **Linia 58:** Oblicz chwilę końca czekania.
- **Linia 59:** Pętla bez końca.
- **Linia 60:** Przejdź przez odłożone zdarzenia, z ich numerami.
- **Linia 61:** Jeśli zdarzenie pasuje...
- **Linia 62:** ...usuń je z listy i zwróć.
- **Linia 63:** Oblicz, ile zostało czasu.
- **Linia 64:** Jeśli nic...
- **Linia 65:** ...zgłoś błąd testu.
- **Linia 66:** Odczytaj kolejną linię.
- **Linia 67:** Jeśli coś przyszło...
- **Linia 68:** ...dopisz to do listy.

```python
 71  def wait_for(predicate, timeout: float = 2.0) -> bool:
 72      deadline = time.monotonic() + timeout
 73      while time.monotonic() < deadline:
 74          if predicate():
 75              return True
 76          time.sleep(0.02)
 77      return False
```

- **Linia 71:** Funkcja czekająca, aż warunek stanie się prawdziwy.
- **Linia 72:** Oblicz chwilę końca czekania.
- **Linia 73:** Dopóki czas nie minął...
- **Linia 74:** ...jeśli warunek jest spełniony...
- **Linia 75:** ...zwróć prawdę.
- **Linia 76:** Odczekaj 0,02 s.
- **Linia 77:** Czas minął, zwróć fałsz.

### Fixture

```python
 80  @pytest.fixture
 81  def avr(tmp_path):
 82      sim = AvrSim(eeprom=tmp_path / "eeprom.bin")
 83      yield sim
 84      sim.close()
```

- **Linia 80:** Dekorator: funkcja jest fixture.
- **Linia 81:** Fixture `avr`. `tmp_path` to tymczasowy katalog od pytest.
- **Linia 82:** Uruchom emulator z plikiem EEPROM w katalogu tymczasowym, by każdy test zaczynał od czystej pamięci.
- **Linia 83:** Przekaż go testowi.
- **Linia 84:** Po teście zamknij emulator.

```python
 87  @pytest.fixture
 88  def link(avr):
 89      link = Link(avr.pty)
 90      assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=POR ver=0.10.0"
 91      yield link
 92      link.port.close()
```

- **Linia 87:** Dekorator: funkcja jest fixture.
- **Linia 88:** Fixture `link`, czyli terminal podłączony do emulatora.
- **Linia 89:** Utwórz terminal.
- **Linia 90:** Układ po starcie ma się przedstawić dokładnie takim tekstem.
- **Linia 91:** Przekaż terminal testowi.
- **Linia 92:** Po teście zamknij port.

### Komendy

```python
 95  def test_commands(link):
 96      assert link.cmd("PING") == "PONG"
 97      assert link.cmd("ver") == "VER 0.10.0"
 98      assert link.cmd("ON") == "OK on=1"
 99      assert link.cmd("TOGGLE") == "OK on=0"
100      assert link.cmd(" pwm 60 ") == "OK pwm=60"
101      assert link.cmd("MODE AUTO") == "OK mode=AUTO"
102      assert link.cmd("PWM 40") == "OK pwm=40 mode=MANUAL"
103      assert link.cmd("TLIM 30") == "OK tlim=30"
```

- **Linia 95:** Test: podstawowe komendy.
- **Linia 96:** `PING` daje `PONG`.
- **Linia 97:** Małe litery działają.
- **Linia 98:** `ON` włącza wyjście.
- **Linia 99:** `TOGGLE` je odwraca.
- **Linia 100:** Dodatkowe spacje nie przeszkadzają.
- **Linia 101:** Włączenie trybu AUTO.
- **Linia 102:** `PWM` w trybie AUTO przełącza na ręczny.
- **Linia 103:** Zmiana progu alarmu.

```python
106  def test_compat_single_chars(link):
107      assert link.raw(b"1") == "OK on=1"
108      assert link.raw(b"0") == "OK on=0"
```

- **Linia 106:** Test: tryb zgodności z instrukcją.
- **Linia 107:** Znak `1` działa bez końca linii.
- **Linia 108:** Znak `0` też.

```python
111  def test_errors(link):
112      assert link.cmd("FOO") == "ERR UNKNOWN"
113      assert link.cmd("PWM") == "ERR ARG"
114      assert link.cmd("PWM 101") == "ERR ARG"
115      assert link.cmd("MODE X") == "ERR ARG"
116      assert link.cmd("TLIM 126") == "ERR ARG"
117      assert link.cmd("PING 1") == "ERR ARG"
118      assert link.cmd("A" * 32) == "ERR UNKNOWN"
119      assert link.cmd("A" * 33) == "ERR TOOLONG"
```

- **Linia 111:** Test: błędy.
- **Linia 112:** Nieznana komenda.
- **Linia 113:** `PWM` bez argumentu.
- **Linia 114:** Jasność poza zakresem.
- **Linia 115:** Nieznany tryb.
- **Linia 116:** Próg poza zakresem.
- **Linia 117:** `PING` z niepotrzebnym argumentem.
- **Linia 118:** 32 znaki mieszczą się w limicie.
- **Linia 119:** 33 znaki przekraczają limit.

```python
122  def test_get_state(link):
123      # Pierwszy pomiar DS18B20 jest gotowy ok. 0,8 s po starcie
124      assert wait_for(lambda: "temp=ERR" not in link.cmd("GET"))
125      line = link.cmd("GET")
126      assert re.fullmatch(r"STATE on=0 pwm=50 mode=MANUAL pot=512 ldr=500 temp=22\.0 "
127                          r"alarm=0 tlim=50 up=\d+", line), line
```

- **Linia 122:** Test: linia stanu.
- **Linia 123:** Komentarz: pierwszy pomiar z czujnika DS18B20 jest gotowy dopiero ok. 0,8 s po starcie.
- **Linia 124:** Czekaj, aż w odpowiedzi na `GET` zamiast `temp=ERR` pojawi się temperatura.
- **Linia 125:** Wyślij `GET`.
- **Linia 126:** Cała linia musi pasować do wzorca. `\.` to dosłowna kropka...
- **Linia 127:** ...a `\d+` to jedna lub więcej cyfr. Czas pracy może być różny.

```python
130  def test_ds18b20_negative_and_missing(link, avr):
131      avr.set_inputs(temp_c=-10.5)
132      assert wait_for(lambda: "temp=-10.5 " in link.cmd("GET"), 3)
133      avr._send("temp none")
134      assert wait_for(lambda: "temp=ERR " in link.cmd("GET"), 3)
135      avr.set_inputs(temp_c=23.5)
136      assert wait_for(lambda: "temp=23.5 " in link.cmd("GET"), 3)
```

- **Linia 130:** Test: czujnik DS18B20 – temperatura ujemna i brak czujnika.
- **Linia 131:** Ustaw -10,5 °C. Ujemne temperatury czujnik koduje inaczej niż dodatnie, więc to osobny przypadek.
- **Linia 132:** W ciągu 3 s odpowiedź na `GET` musi zawierać `temp=-10.5`.
- **Linia 133:** Odłącz wirtualny czujnik (linia 1-Wire nikt nie odpowiada).
- **Linia 134:** Układ musi zgłosić `temp=ERR`.
- **Linia 135:** Podłącz czujnik z powrotem z temperaturą 23,5 °C.
- **Linia 136:** Pomiary muszą wrócić same, bez resetu.

### Dioda i przycisk

```python
139  def test_pwm_fade_and_gamma(link, avr):
140      link.cmd("PWM 100")
141      link.cmd("ON")
142      time.sleep(0.15)
143      assert 0 < avr.duty < 1, "jasność ma rosnąć płynnie"
144      assert wait_for(lambda: avr.duty == 1.0)
145      link.cmd("PWM 50")
146      assert wait_for(lambda: abs(avr.duty - 256 / 1023) < 1e-3), avr.duty
147      link.cmd("OFF")
148      assert wait_for(lambda: avr.duty == 0)
```

- **Linia 139:** Test: płynna zmiana jasności i korekcja gamma.
- **Linia 140:** Ustaw pełną jasność.
- **Linia 141:** Włącz wyjście.
- **Linia 142:** Poczekaj 0,15 s, czyli krócej niż trwa pełne przejście.
- **Linia 143:** Wypełnienie musi być między 0 a 1, czyli jasność jest w trakcie zmiany.
- **Linia 144:** Po chwili wypełnienie musi dojść do 100 %.
- **Linia 145:** Ustaw jasność 50 %.
- **Linia 146:** Wypełnienie musi wynieść około 256/1023, czyli 25 %, bo jasność jest podnoszona do kwadratu.
- **Linia 147:** Wyłącz wyjście.
- **Linia 148:** Wypełnienie musi spaść do zera.

```python
151  def test_button_short_and_long(link, avr):
152      avr.press_button()
153      assert link.wait_event("EVT BTN") == "EVT BTN on=1"
154      avr.press_button(long=True)
155      assert link.wait_event("EVT BTN", 3) == "EVT BTN mode=AUTO"
```

- **Linia 151:** Test: krótkie i długie naciśnięcie.
- **Linia 152:** Krótkie naciśnięcie.
- **Linia 153:** Zdarzenie o włączeniu.
- **Linia 154:** Długie naciśnięcie.
- **Linia 155:** Zdarzenie o zmianie trybu.

### Czujniki

```python
158  def test_potentiometer_event(link, avr):
159      time.sleep(0.2)  # pierwszy odczyt po starcie to punkt odniesienia, nie zmiana
160      avr.set_inputs(pot=1023)
161      assert link.wait_event("EVT POT") == "EVT POT pwm=100"
```

- **Linia 158:** Test: zdarzenie od potencjometru.
- **Linia 159:** Poczekaj, bo pierwszy odczyt to tylko punkt odniesienia.
- **Linia 160:** Skręć potencjometr na maksimum.
- **Linia 161:** Musi przyjść zdarzenie z jasnością 100 %.

```python
164  def test_auto_mode_follows_light(link, avr):
165      link.cmd("ON")
166      link.cmd("MODE AUTO")
167      for ldr, pwm in ((100, 100), (900, 0), (500, 50)):
168          avr.set_inputs(ldr=ldr)
169          assert wait_for(lambda: f"pwm={pwm} " in link.cmd("GET")), (ldr, pwm)
```

- **Linia 164:** Test: tryb AUTO reaguje na światło.
- **Linia 165:** Włącz wyjście.
- **Linia 166:** Włącz tryb AUTO.
- **Linia 167:** Dla trzech par: odczyt fotorezystora i oczekiwana jasność...
- **Linia 168:** ...ustaw odczyt...
- **Linia 169:** ...i poczekaj, aż `GET` pokaże oczekiwaną jasność.

```python
164  def test_auto_mode_follows_light(link, avr):
165      link.cmd("ON")
166      link.cmd("MODE AUTO")
167      for ldr, pwm in ((100, 100), (900, 0), (500, 50)):
168          avr.set_inputs(ldr=ldr)
169          assert wait_for(lambda: f"pwm={pwm} " in link.cmd("GET")), (ldr, pwm)
170
171
172  def test_temperature_alarm(link, avr):
173      link.cmd("TLIM 30")
174      avr.set_inputs(temp_c=35)
175      # DS18B20 mierzy co 1 s, alarm sprawdzany co 0,5 s
176      assert link.wait_event("EVT TEMP", 3) == "EVT TEMP alarm=1 temp=35.0"
177      avr.set_inputs(temp_c=20)
178      assert link.wait_event("EVT TEMP", 3) == "EVT TEMP alarm=0 temp=20.0"
```

- **Linia 172:** Test: alarm temperatury.
- **Linia 173:** Ustaw próg 30 °C.
- **Linia 174:** Temperatura 35 °C.
- **Linia 175:** Komentarz: czujnik DS18B20 mierzy co sekundę, a alarm jest sprawdzany co pół sekundy, więc czekamy do 3 s.
- **Linia 176:** Musi przyjść zdarzenie włączenia alarmu z dokładną temperaturą 35,0 °C.
- **Linia 177:** Temperatura 20 °C.
- **Linia 178:** Musi przyjść zdarzenie wyłączenia alarmu z temperaturą 20,0 °C.

### Pamięć i reset

```python
181  def test_state_survives_power_loss(link, avr):
182      link.cmd("ON")
183      link.cmd("PWM 70")
184      link.cmd("TLIM 40")
185      time.sleep(2.6)  # zapis do EEPROM 2 s po ostatniej zmianie
186      avr.reset("POR")
187      assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=POR ver=0.10.0"
188      state = link.cmd("GET")
189      assert "on=1 pwm=70" in state and "tlim=40" in state
```

- **Linia 181:** Test: stan przetrwa zanik zasilania.
- **Linia 182:** Włącz wyjście.
- **Linia 183:** Ustaw jasność.
- **Linia 184:** Ustaw próg.
- **Linia 185:** Poczekaj 2,6 s, bo zapis do EEPROM odbywa się 2 s po ostatniej zmianie.
- **Linia 186:** Symuluj zanik zasilania.
- **Linia 187:** Układ musi się przedstawić z przyczyną `POR`.
- **Linia 188:** Odczytaj stan.
- **Linia 189:** Zapisane wartości muszą wrócić.

```python
192  def test_external_reset_reason(link, avr):
193      avr.reset()
194      assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=EXT ver=0.10.0"
```

- **Linia 192:** Test: przyczyna resetu zewnętrznego.
- **Linia 193:** Zresetuj układ.
- **Linia 194:** Układ musi zgłosić przyczynę `EXT`.

### Cały łańcuch

```python
197  def test_server_end_to_end(avr):
198      dev = Device(SerialTransport(avr.pty), poll_interval=0.2, simulated="avr")
199      dev.start()
200      try:
201          assert dev.wait_connected(5)
202          client = create_app(Config(simulate="avr"), device=dev, sim=avr).test_client()
203          assert client.get("/on").json["on"] is True
204          assert client.post("/api/state", json={"pwm": 100}).json["pwm"] == 100
205          assert wait_for(lambda: avr.duty == 1.0)
206          avr.press_button()
207          assert wait_for(lambda: dev.snapshot()["on"] is False)
208          assert client.get("/api/health").json["firmware"] == "0.10.0"
209      finally:
210          dev.stop()
```

- **Linia 197:** Test całości: zapytanie HTTP, serwer, UART i firmware w emulatorze.
- **Linia 198:** Utwórz obiekt `Device` podłączony do emulatora.
- **Linia 199:** Uruchom jego wątki.
- **Linia 200:** Próbuj, a na końcu posprzątaj.
- **Linia 201:** Połączenie musi powstać w ciągu 5 s.
- **Linia 202:** Utwórz klienta testowego.
- **Linia 203:** `/on` włącza wyjście.
- **Linia 204:** Zmiana jasności działa.
- **Linia 205:** Wypełnienie w emulatorze musi dojść do 100 %.
- **Linia 206:** Naciśnij przycisk.
- **Linia 207:** Serwer musi zobaczyć wyłączenie.
- **Linia 208:** Wersja firmware musi być prawdziwa, bez końcówki `-sim`.
- **Linia 209:** Na końcu, także przy błędzie:
- **Linia 210:** Zatrzymaj wątki.

---

[← Poprzedni](52-testy-mcu-sim.md) · [Spis treści](README.md) · [Następny →](60-sim-makefile.md)
