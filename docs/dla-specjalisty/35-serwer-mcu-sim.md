# Serwer: `mcu_sim.py`, układ udawany w Pythonie

[← Poprzedni](34-serwer-routes.md) · [Spis treści](README.md) · [Następny →](36-serwer-avrsim.md)

**Po co ten plik:** pozwala uruchomić cały serwer i stronę bez żadnego sprzętu (`SIMULATE=1`). Klasa `McuSim` zachowuje się jak firmware: rozumie te same komendy, odpowiada tym samym tekstem i wysyła te same zdarzenia. Reguły są więc takie same jak w firmware, ale zapisane w Pythonie, co ułatwia ich porównanie.

**Uwaga projektowa:** każda zmiana logiki firmware wymaga tej samej zmiany tutaj.

## `server/app/mcu_sim.py`

[Otwórz ten plik w repozytorium](../../server/app/mcu_sim.py)

```python
  1  """Model ATmega32A (płytka ATB 1.05) na poziomie protokołu – zachowuje się jak firmware, ale bez sprzętu.
  2
  3  Stałe odpowiadają firmware/include/config.h; temperatura jak z DS18B20 (rozdzielczość 1/16 °C).
  4  """
  5
  6  from __future__ import annotations
  7
  8  import queue
  9  import threading
 10  import time
 11  from typing import Callable
 12
 13  from .protocol import LINE_MAX, TLIM_MAX
 14
 15  FW_VERSION = "0.10.0-sim"
 16  POT_HYST = 2
 17  AUTO_HYST = 3
 18  LDR_DARK, LDR_BRIGHT = 200, 800
 19  TEMP_HYST = 2.0
 20  POT_PERIOD, AUTO_PERIOD, TEMP_PERIOD = 0.05, 0.2, 0.5
```

- **Linia 1:** Początek docstringa: model układu ATmega32A na płytce ATB na poziomie protokołu. Działa jak firmware, ale bez sprzętu.
- **Linia 3:** Stałe odpowiadają plikowi `firmware/include/config.h`, a temperatura jest podawana tak jak z czujnika DS18B20, w szesnastych częściach stopnia.
- **Linia 4:** Koniec docstringa.
- **Linia 6:** Nowszy zapis podpowiedzi typów.
- **Linia 8:** Biblioteka kolejek. Kolejka udaje linię nadawczą układu.
- **Linia 9:** Biblioteka wątków.
- **Linia 10:** Biblioteka czasu.
- **Linia 11:** Narzędzie do opisu typu funkcji.
- **Linia 13:** Pobiera z naszego parsera limit długości linii i największy próg.
- **Linia 15:** Wersja symulatora. Końcówka `-sim` odróżnia ją od prawdziwego firmware.
- **Linia 16:** Margines potencjometru: 2 %.
- **Linia 17:** Margines trybu AUTO: 3 %.
- **Linia 18:** Progi fotorezystora. Zapis przypisuje dwie wartości do dwóch nazw naraz.
- **Linia 19:** Histereza alarmu: 2 °C.
- **Linia 20:** Odstępy sprawdzania w sekundach: potencjometr, tryb AUTO i temperatura.

### Przeliczanie pomiarów

```python
 23  def pot_percent(raw: int) -> int:
 24      return ((raw >> 2) * 100 + 127) // 255
 25
 26
 27  def auto_percent(ldr: int) -> int:
 28      if ldr <= LDR_DARK:
 29          return 100
 30      if ldr >= LDR_BRIGHT:
 31          return 0
 32      return 100 - (ldr - LDR_DARK) * 100 // (LDR_BRIGHT - LDR_DARK)
```

- **Linia 23:** Funkcja przelicza surowy odczyt potencjometru na procenty.
- **Linia 24:** Wzór jak w firmware: odrzuć dwa bity (`>> 2`), przelicz na 0–100, z zaokrągleniem. `//` to dzielenie bez reszty.
- **Linia 27:** Funkcja wylicza jasność trybu AUTO z odczytu fotorezystora.
- **Linia 28:** Jeśli jest ciemno...
- **Linia 29:** ...jasność 100 %.
- **Linia 30:** Jeśli jest jasno...
- **Linia 31:** ...jasność 0 %.
- **Linia 32:** Pomiędzy: liczymy proporcjonalnie, jak w firmware.

### Tworzenie symulatora

```python
 35  class McuSim:
 36      def __init__(self, clock: Callable[[], float] = time.monotonic) -> None:
 37          self._clock = clock
 38          self._lock = threading.RLock()
 39          self._out: queue.Queue[str] = queue.Queue()
 40          self.eeprom = {"on": False, "pwm": 50, "mode": "MANUAL", "tlim": 50}
 41          # Wirtualny sprzęt: surowe odczyty ADC i temperatura czujnika
 42          self.pot = 512
 43          self.ldr = 500
 44          self.temp_c = 22.0
 45          self.reset("POR")
```

- **Linia 35:** Początek klasy.
- **Linia 36:** Funkcja tworząca obiekt. Zegar można podmienić, co bardzo pomaga w testach.
- **Linia 37:** Zapamiętaj zegar.
- **Linia 38:** Blokada, którą ten sam wątek może zająć kilka razy.
- **Linia 39:** Kolejka linii wysyłanych przez układ.
- **Linia 40:** Symulowana pamięć EEPROM: słownik z wartościami domyślnymi.
- **Linia 41:** Komentarz: wirtualny sprzęt to surowe odczyty ADC i temperatura czujnika.
- **Linia 42:** Potencjometr w połowie zakresu.
- **Linia 43:** Fotorezystor w połowie zakresu.
- **Linia 44:** Temperatura 22 °C.
- **Linia 45:** Wykonaj reset jak po włączeniu zasilania.

### Wirtualny sprzęt

```python
 47      # --- wirtualny sprzęt -------------------------------------------------
 48
 49      def reset(self, reason: str = "EXT") -> None:
 50          with self._lock:
 51              self.on = self.eeprom["on"]
 52              self.mode = self.eeprom["mode"]
 53              self.pwm = self.eeprom["pwm"]
 54              self.tlim = self.eeprom["tlim"]
 55              self.alarm = False
 56              self._line = ""
 57              self._too_long = False
 58              self._pot_last: int | None = None
 59              now = self._clock()
 60              self._boot = now
 61              self._next = {"pot": now, "auto": now, "temp": now}
 62              self._emit(f"EVT BOOT reset={reason} ver={FW_VERSION}")
```

- **Linia 47:** Komentarz oddzielający tę część.
- **Linia 49:** Metoda symulująca reset układu. Dostaje przyczynę.
- **Linia 50:** Zajmij blokadę.
- **Linia 51:** Wczytaj stan wyjścia z EEPROM.
- **Linia 52:** Wczytaj tryb.
- **Linia 53:** Wczytaj jasność.
- **Linia 54:** Wczytaj próg alarmu.
- **Linia 55:** Alarm wyłączony.
- **Linia 56:** Pusty bufor odbieranej linii.
- **Linia 57:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 58:** Brak zapamiętanego położenia potencjometru.
- **Linia 59:** Pobierz aktualny czas.
- **Linia 60:** Zapamiętaj chwilę startu.
- **Linia 61:** Wszystkie zadania okresowe mają się wykonać od razu.
- **Linia 62:** Wyślij zdarzenie startu.

```python
 64      def press_button(self, long: bool = False) -> None:
 65          with self._lock:
 66              if long:
 67                  self.mode = "MANUAL" if self.mode == "AUTO" else "AUTO"
 68                  self._save()
 69                  self._emit(f"EVT BTN mode={self.mode}")
 70              else:
 71                  self.on = not self.on
 72                  self._save()
 73                  self._emit(f"EVT BTN on={int(self.on)}")
```

- **Linia 64:** Metoda symulująca naciśnięcie przycisku.
- **Linia 65:** Zajmij blokadę.
- **Linia 66:** Jeśli naciśnięcie jest długie...
- **Linia 67:** ...zmień tryb na przeciwny.
- **Linia 68:** Zapisz w EEPROM.
- **Linia 69:** Wyślij zdarzenie.
- **Linia 70:** W przeciwnym razie, dla krótkiego:
- **Linia 71:** Odwróć stan wyjścia.
- **Linia 72:** Zapisz w EEPROM.
- **Linia 73:** Wyślij zdarzenie. `int(self.on)` zamienia prawdę na 1, a fałsz na 0.

```python
 75      def set_inputs(self, pot: int | None = None, ldr: int | None = None,
 76                     temp_c: float | None = None) -> None:
 77          with self._lock:
 78              if pot is not None:
 79                  self.pot = max(0, min(1023, int(pot)))
 80              if ldr is not None:
 81                  self.ldr = max(0, min(1023, int(ldr)))
 82              if temp_c is not None:
 83                  self.temp_c = max(-55.0, min(125.0, float(temp_c)))
```

- **Linia 75:** Metoda ustawiająca wirtualne wejścia. Parametry są opcjonalne.
- **Linia 76:** Ciąg dalszy listy parametrów.
- **Linia 77:** Zajmij blokadę.
- **Linia 78:** Jeśli podano potencjometr...
- **Linia 79:** ...zapisz wartość przyciętą do zakresu 0–1023.
- **Linia 80:** Jeśli podano fotorezystor...
- **Linia 81:** ...zapisz wartość przyciętą do zakresu 0–1023.
- **Linia 82:** Jeśli podano temperaturę...
- **Linia 83:** ...zapisz wartość przyciętą do zakresu czujnika DS18B20, od -55 do 125 °C.

```python
 85      @property
 86      def brightness(self) -> int:
 87          return self.pwm if self.on else 0
 88
 89      def snapshot(self) -> dict:
 90          with self._lock:
 91              return {
 92                  "pot": self.pot,
 93                  "ldr": self.ldr,
 94                  "temp_c": self.temp_c,
 95                  "led": self.brightness,
 96                  "status_alarm": self.alarm,
 97              }
```

- **Linia 85:** Dekorator: metodę czyta się jak pole.
- **Linia 86:** Metoda zwraca jasność diody.
- **Linia 87:** Jasność, gdy wyjście jest włączone, a w przeciwnym razie 0.
- **Linia 89:** Metoda zwraca stan wirtualnej płytki.
- **Linia 90:** Zajmij blokadę.
- **Linia 91:** Zwróć słownik:
- **Linia 92:** Potencjometr.
- **Linia 93:** Fotorezystor.
- **Linia 94:** Temperatura.
- **Linia 95:** Jasność diody.
- **Linia 96:** Czy dioda statusu sygnalizuje alarm.
- **Linia 97:** Koniec słownika.

### Symulowany UART

```python
 99      # --- UART -------------------------------------------------------------
100
101      def feed(self, data: str) -> None:
102          with self._lock:
103              for c in data:
104                  self._rx(c)
105
106      def read_line(self, timeout: float = 0.0) -> str | None:
107          try:
108              return self._out.get(timeout=timeout) if timeout > 0 else self._out.get_nowait()
109          except queue.Empty:
110              return None
111
112      def _emit(self, line: str) -> None:
113          self._out.put(line)
```

- **Linia 99:** Komentarz oddzielający tę część.
- **Linia 101:** Metoda przyjmująca tekst od serwera.
- **Linia 102:** Zajmij blokadę.
- **Linia 103:** Przejdź po kolejnych znakach.
- **Linia 104:** Obsłuż każdy znak.
- **Linia 106:** Metoda oddająca jedną linię wysłaną przez układ.
- **Linia 107:** Próbuj.
- **Linia 108:** Jeśli podano czas, czekaj najwyżej tyle. W przeciwnym razie weź od razu, bez czekania.
- **Linia 109:** Jeśli kolejka jest pusta...
- **Linia 110:** ...zwróć `None`.
- **Linia 112:** Metoda „wysyłająca” linię.
- **Linia 113:** Włóż ją do kolejki.

```python
115      def _rx(self, c: str) -> None:
116          if c == "\r":
117              return
118          if c == "\n":
119              line = self._line.strip()
120              if self._too_long:
121                  self._emit("ERR TOOLONG")
122              elif line:
123                  self._execute(line)
124              self._line = ""
125              self._too_long = False
126              return
127          if not self._line and not self._too_long and c in "10":
128              self._set_on(c == "1")
129              self._emit(f"OK on={int(self.on)}")
130              return
131          if len(self._line) >= LINE_MAX:
132              self._too_long = True
133              return
134          self._line += c.upper()
```

- **Linia 115:** Metoda obsługująca jeden znak, odpowiednik `protocol_rx` w firmware.
- **Linia 116:** Jeśli to `\r`...
- **Linia 117:** ...pomiń go.
- **Linia 118:** Jeśli to koniec linii:
- **Linia 119:** Weź zebraną linię bez spacji na brzegach.
- **Linia 120:** Jeśli była zbyt długa...
- **Linia 121:** ...odeślij błąd.
- **Linia 122:** W przeciwnym razie, jeśli nie jest pusta...
- **Linia 123:** ...wykonaj komendę.
- **Linia 124:** Wyzeruj bufor.
- **Linia 125:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 126:** Zakończ.
- **Linia 127:** Jeśli bufor jest pusty, a znak to `1` lub `0`...
- **Linia 128:** ...włącz lub wyłącz wyjście.
- **Linia 129:** Odeślij nowy stan.
- **Linia 130:** Zakończ.
- **Linia 131:** Jeśli linia osiągnęła limit długości...
- **Linia 132:** ...oznacz, że jest zbyt długa...
- **Linia 133:** ...i zakończ.
- **Linia 134:** Dopisz znak do bufora, zamieniając go na wielką literę.

```python
136      def _execute(self, line: str) -> None:
137          cmd, _, arg = line.partition(" ")
138          arg = arg.strip()
```

- **Linia 136:** Metoda wykonująca komendę, odpowiednik `execute` w firmware.
- **Linia 137:** Podziel linię na komendę i argument. Podkreślenie to sama spacja, której nie potrzebujemy.
- **Linia 138:** Usuń ewentualne spacje z argumentu.

```python
140          if cmd == "PWM":
141              value = _parse_uint(arg, 100)
142              if value is None:
143                  self._emit("ERR ARG")
144                  return
145              mode_changed = self.mode != "MANUAL"
146              self._set_pwm(value)
147              self._emit(f"OK pwm={value}" + (" mode=MANUAL" if mode_changed else ""))
```

- **Linia 140:** Komenda `PWM`:
- **Linia 141:** Zamień argument na liczbę z limitem 100.
- **Linia 142:** Jeśli się nie udało...
- **Linia 143:** ...odeślij błąd...
- **Linia 144:** ...i zakończ.
- **Linia 145:** Zapamiętaj, czy tryb zmieni się na ręczny.
- **Linia 146:** Ustaw jasność.
- **Linia 147:** Odeślij odpowiedź, a jeśli tryb się zmienił, dopisz o tym informację.

```python
148          elif cmd == "MODE":
149              if arg not in ("AUTO", "MANUAL"):
150                  self._emit("ERR ARG")
151                  return
152              self.mode = arg
153              self._save()
154              self._emit(f"OK mode={self.mode}")
155          elif cmd == "TLIM":
156              value = _parse_uint(arg, TLIM_MAX)
157              if value is None:
158                  self._emit("ERR ARG")
159                  return
160              self.tlim = value
161              self._save()
162              self._emit(f"OK tlim={value}")
```

- **Linia 148:** Komenda `MODE`:
- **Linia 149:** Jeśli argument nie jest dozwolony...
- **Linia 150:** ...odeślij błąd...
- **Linia 151:** ...i zakończ.
- **Linia 152:** Ustaw tryb.
- **Linia 153:** Zapisz w EEPROM.
- **Linia 154:** Odeślij odpowiedź.
- **Linia 155:** Komenda `TLIM`:
- **Linia 156:** Zamień argument na liczbę z limitem 125.
- **Linia 157:** Jeśli się nie udało...
- **Linia 158:** ...odeślij błąd...
- **Linia 159:** ...i zakończ.
- **Linia 160:** Ustaw próg.
- **Linia 161:** Zapisz w EEPROM.
- **Linia 162:** Odeślij odpowiedź.

```python
163          elif cmd in ("PING", "VER", "ON", "OFF", "TOGGLE", "GET") and arg:
164              self._emit("ERR ARG")
165          elif cmd == "PING":
166              self._emit("PONG")
167          elif cmd == "VER":
168              self._emit(f"VER {FW_VERSION}")
169          elif cmd in ("ON", "OFF", "TOGGLE"):
170              self._set_on(not self.on if cmd == "TOGGLE" else cmd == "ON")
171              self._emit(f"OK on={int(self.on)}")
172          elif cmd == "GET":
173              self._emit(self._state_line())
174          else:
175              self._emit("ERR UNKNOWN")
```

- **Linia 163:** Jeśli komenda nie przyjmuje argumentu, a jakiś podano...
- **Linia 164:** ...odeślij `ERR ARG`.
- **Linia 165:** Komenda `PING`...
- **Linia 166:** ...odpowiedź to `PONG`.
- **Linia 167:** Komenda `VER`...
- **Linia 168:** ...odpowiedź to wersja.
- **Linia 169:** Komendy `ON`, `OFF`, `TOGGLE`:
- **Linia 170:** Dla `TOGGLE` weź stan odwrotny, a dla pozostałych prawdę, jeśli komenda to `ON`.
- **Linia 171:** Odeślij nowy stan.
- **Linia 172:** Komenda `GET`...
- **Linia 173:** ...odeślij pełny stan.
- **Linia 174:** Żadna komenda nie pasuje:
- **Linia 175:** Odeślij `ERR UNKNOWN`.

```python
177      def _state_line(self) -> str:
178          up = int(self._clock() - self._boot)
179          return (f"STATE on={int(self.on)} pwm={self.pwm} mode={self.mode} pot={self.pot} "
180                  f"ldr={self.ldr} temp={self._temp_reading():.1f} alarm={int(self.alarm)} "
181                  f"tlim={self.tlim} up={up}")
```

- **Linia 177:** Metoda budująca linię `STATE`.
- **Linia 178:** Czas pracy w sekundach: teraz minus chwila startu, jako liczba całkowita.
- **Linia 179:** Zwróć tekst, sklejony z trzech kawałków w nawiasach. Pierwszy kawałek: wyjście, jasność, tryb, potencjometr.
- **Linia 180:** Drugi kawałek: fotorezystor, temperatura z jedną cyfrą po przecinku i alarm.
- **Linia 181:** Trzeci kawałek: próg i czas pracy.

### Logika jak w firmware

```python
183      # --- logika jak w firmware ---------------------------------------------
184
185      def _set_on(self, on: bool) -> None:
186          self.on = on
187          self._save()
188
189      def _set_pwm(self, value: int) -> None:
190          self.mode = "MANUAL"
191          self.pwm = value
192          self._save()
193
194      def _save(self) -> None:
195          self.eeprom["on"] = self.on
196          self.eeprom["mode"] = self.mode
197          self.eeprom["tlim"] = self.tlim
198          if self.mode == "MANUAL":
199              self.eeprom["pwm"] = self.pwm
200
201      def _temp_reading(self) -> float:
202          # DS18B20 podaje 1/16 °C, firmware zaokrągla do 0,1 °C (dzielenie jak w C)
203          raw = round(self.temp_c * 16)
204          return int((raw * 10 + (8 if raw >= 0 else -8)) / 16) / 10
205
206      def tick(self) -> None:
```

- **Linia 183:** Komentarz oddzielający tę część.
- **Linia 185:** Metoda ustawiająca stan wyjścia.
- **Linia 186:** Zapisz stan.
- **Linia 187:** Zapisz w EEPROM.
- **Linia 189:** Metoda ustawiająca jasność.
- **Linia 190:** Przełącz na tryb ręczny.
- **Linia 191:** Zapisz jasność.
- **Linia 192:** Zapisz w EEPROM.
- **Linia 194:** Metoda zapisu do symulowanego EEPROM.
- **Linia 195:** Zapisz stan wyjścia.
- **Linia 196:** Zapisz tryb.
- **Linia 197:** Zapisz próg.
- **Linia 198:** Jeśli tryb jest ręczny...
- **Linia 199:** ...zapisz jasność. W trybie AUTO jej nie zapisujemy, tak samo jak w firmware.
- **Linia 201:** Metoda wyliczająca temperaturę „zmierzoną” przez układ.
- **Linia 202:** Komentarz: czujnik DS18B20 podaje szesnaste części stopnia, a firmware zaokrągla je do dziesiątych, dzieląc liczby całkowite tak jak język C.
- **Linia 203:** Zamień temperaturę na szesnaste części stopnia, jak czujnik.
- **Linia 204:** Przelicz tym samym wzorem co `decode()` w `ds18b20.c`. `int(...)` obcina ułamek w stronę zera, tak jak dzielenie w C. Wynik dzielimy przez 10, by otrzymać stopnie.

### Zadania okresowe

```python
206      def tick(self) -> None:
207          """Okresowe zadania firmware: potencjometr, tryb AUTO, alarm temperatury."""
208          with self._lock:
209              now = self._clock()
210              if now >= self._next["pot"]:
211                  self._next["pot"] = now + POT_PERIOD
212                  self._pot_check()
213              if now >= self._next["auto"]:
214                  self._next["auto"] = now + AUTO_PERIOD
215                  self._auto_check()
216              if now >= self._next["temp"]:
217                  self._next["temp"] = now + TEMP_PERIOD
218                  self._temp_check()
```

- **Linia 206:** Metoda odpowiadająca pętli głównej firmware.
- **Linia 207:** Docstring: zadania okresowe, czyli potencjometr, tryb AUTO i alarm temperatury.
- **Linia 208:** Zajmij blokadę.
- **Linia 209:** Pobierz aktualny czas.
- **Linia 210:** Jeśli nadszedł czas sprawdzenia potencjometru...
- **Linia 211:** ...ustal kolejny termin...
- **Linia 212:** ...i go sprawdź.
- **Linia 213:** Jeśli nadszedł czas przeliczenia trybu AUTO...
- **Linia 214:** ...ustal kolejny termin...
- **Linia 215:** ...i przelicz.
- **Linia 216:** Jeśli nadszedł czas sprawdzenia temperatury...
- **Linia 217:** ...ustal kolejny termin...
- **Linia 218:** ...i ją sprawdź.

```python
220      def _pot_check(self) -> None:
221          p = pot_percent(self.pot)
222          if self._pot_last is None:
223              self._pot_last = p
224              return
225          if p == self._pot_last:
226              return
227          if abs(p - self._pot_last) < POT_HYST and p not in (0, 100):
228              return
229          self._pot_last = p
230          mode_changed = self.mode != "MANUAL"
231          self._set_pwm(p)
232          self._emit(f"EVT POT pwm={p}" + (" mode=MANUAL" if mode_changed else ""))
```

- **Linia 220:** Metoda sprawdzająca potencjometr.
- **Linia 221:** Przelicz odczyt na procenty.
- **Linia 222:** Jeśli to pierwszy odczyt...
- **Linia 223:** ...zapamiętaj go jako punkt odniesienia...
- **Linia 224:** ...i zakończ.
- **Linia 225:** Jeśli nic się nie zmieniło...
- **Linia 226:** ...zakończ.
- **Linia 227:** Jeśli zmiana jest mniejsza niż margines, a pokrętło nie stoi na skraju...
- **Linia 228:** ...zakończ.
- **Linia 229:** Zapamiętaj nowe położenie.
- **Linia 230:** Zapamiętaj, czy tryb się zmieni.
- **Linia 231:** Ustaw jasność.
- **Linia 232:** Wyślij zdarzenie.

```python
234      def _auto_check(self) -> None:
235          if self.mode != "AUTO":
236              return
237          p = auto_percent(self.ldr)
238          if p != self.pwm and (abs(p - self.pwm) >= AUTO_HYST or p in (0, 100)):
239              self.pwm = p
```

- **Linia 234:** Metoda dopasowująca jasność w trybie AUTO.
- **Linia 235:** Jeśli tryb jest inny...
- **Linia 236:** ...zakończ.
- **Linia 237:** Wylicz jasność ze światła.
- **Linia 238:** Jeśli jest inna i różnica jest dostatecznie duża albo to skraj...
- **Linia 239:** ...ustaw ją.

```python
241      def _temp_check(self) -> None:
242          t = self._temp_reading()
243          if not self.alarm and t >= self.tlim:
244              self.alarm = True
245              self._emit(f"EVT TEMP alarm=1 temp={t:.1f}")
246          elif self.alarm and t < self.tlim - TEMP_HYST:
247              self.alarm = False
248              self._emit(f"EVT TEMP alarm=0 temp={t:.1f}")
```

- **Linia 241:** Metoda sprawdzająca alarm temperatury.
- **Linia 242:** Odczytaj temperaturę.
- **Linia 243:** Jeśli alarmu nie ma, a próg został osiągnięty...
- **Linia 244:** ...włącz alarm...
- **Linia 245:** ...i wyślij zdarzenie.
- **Linia 246:** Jeśli alarm trwa, a temperatura spadła poniżej progu minus histereza...
- **Linia 247:** ...wyłącz alarm...
- **Linia 248:** ...i wyślij zdarzenie.

### Zamiana tekstu na liczbę

```python
251  def _parse_uint(text: str, maximum: int) -> int | None:
252      if not text.isdigit() or not text.isascii():
253          return None
254      value = int(text)
255      return value if value <= maximum else None
```

- **Linia 251:** Funkcja zamienia tekst na liczbę, a przy błędzie zwraca `None`.
- **Linia 252:** Jeśli tekst nie składa się z samych cyfr ASCII...
- **Linia 253:** ...zwróć `None`.
- **Linia 254:** Zamień tekst na liczbę.
- **Linia 255:** Zwróć ją, jeśli mieści się w limicie, a w przeciwnym razie `None`.

---

[← Poprzedni](34-serwer-routes.md) · [Spis treści](README.md) · [Następny →](36-serwer-avrsim.md)
