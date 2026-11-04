# Serwer: `mcu_sim.py`, układ udawany w Pythonie

[← Poprzedni](34-serwer-routes.md) · [Spis treści](README.md) · [Następny →](36-serwer-avrsim.md)

**Po co ten plik:** pozwala uruchomić cały serwer i stronę bez żadnego sprzętu (`SIMULATE=1`). Klasa `McuSim` zachowuje się jak firmware: rozumie te same komendy, odpowiada tym samym tekstem i wysyła te same zdarzenia. Reguły są więc takie same jak w firmware, ale zapisane w Pythonie, co ułatwia ich porównanie.

**Uwaga projektowa:** każda zmiana logiki firmware wymaga tej samej zmiany tutaj.

## `server/app/mcu_sim.py`

[Otwórz ten plik w repozytorium](../../server/app/mcu_sim.py)

```python
  1  """Model ATmega8 na poziomie protokołu – zachowuje się jak firmware, ale bez sprzętu.
  2
  3  Stałe odpowiadają firmware/include/config.h (czujnik LM35, VREF 5 V).
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
 15  FW_VERSION = "0.9.0-sim"
 16  ADC_VREF_MV = 5000
 17  POT_HYST = 2
 18  AUTO_HYST = 3
 19  LDR_DARK, LDR_BRIGHT = 200, 800
 20  TEMP_HYST = 2.0
 21  POT_PERIOD, AUTO_PERIOD, TEMP_PERIOD = 0.05, 0.2, 0.5
```

- **Linia 1:** Początek docstringa: model układu ATmega8 na poziomie protokołu. Działa jak firmware, ale bez sprzętu.
- **Linia 3:** Stałe odpowiadają plikowi `firmware/include/config.h`, a czujnik to LM35 przy napięciu odniesienia 5 V.
- **Linia 4:** Koniec docstringa.
- **Linia 6:** Nowszy zapis podpowiedzi typów.
- **Linia 8:** Biblioteka kolejek. Kolejka udaje linię nadawczą układu.
- **Linia 9:** Biblioteka wątków.
- **Linia 10:** Biblioteka czasu.
- **Linia 11:** Narzędzie do opisu typu funkcji.
- **Linia 13:** Pobiera z naszego parsera limit długości linii i największy próg.
- **Linia 15:** Wersja symulatora. Końcówka `-sim` odróżnia ją od prawdziwego firmware.
- **Linia 16:** Napięcie odniesienia w miliwoltach.
- **Linia 17:** Margines potencjometru: 2 %.
- **Linia 18:** Margines trybu AUTO: 3 %.
- **Linia 19:** Progi fotorezystora. Zapis przypisuje dwie wartości do dwóch nazw naraz.
- **Linia 20:** Histereza alarmu: 2 °C.
- **Linia 21:** Odstępy sprawdzania w sekundach: potencjometr, tryb AUTO i temperatura.

### Przeliczanie pomiarów

```python
 24  def pot_percent(raw: int) -> int:
 25      return ((raw >> 2) * 100 + 127) // 255
 26
 27
 28  def auto_percent(ldr: int) -> int:
 29      if ldr <= LDR_DARK:
 30          return 100
 31      if ldr >= LDR_BRIGHT:
 32          return 0
 33      return 100 - (ldr - LDR_DARK) * 100 // (LDR_BRIGHT - LDR_DARK)
```

- **Linia 24:** Funkcja przelicza surowy odczyt potencjometru na procenty.
- **Linia 25:** Wzór jak w firmware: odrzuć dwa bity (`>> 2`), przelicz na 0–100, z zaokrągleniem. `//` to dzielenie bez reszty.
- **Linia 28:** Funkcja wylicza jasność trybu AUTO z odczytu fotorezystora.
- **Linia 29:** Jeśli jest ciemno...
- **Linia 30:** ...jasność 100 %.
- **Linia 31:** Jeśli jest jasno...
- **Linia 32:** ...jasność 0 %.
- **Linia 33:** Pomiędzy: liczymy proporcjonalnie, jak w firmware.

### Tworzenie symulatora

```python
 36  class McuSim:
 37      def __init__(self, clock: Callable[[], float] = time.monotonic) -> None:
 38          self._clock = clock
 39          self._lock = threading.RLock()
 40          self._out: queue.Queue[str] = queue.Queue()
 41          self.eeprom = {"on": False, "pwm": 50, "mode": "MANUAL", "tlim": 50}
 42          # Wirtualny sprzęt: surowe odczyty ADC i temperatura czujnika
 43          self.pot = 512
 44          self.ldr = 500
 45          self.temp_c = 22.0
 46          self.reset("POR")
```

- **Linia 36:** Początek klasy.
- **Linia 37:** Funkcja tworząca obiekt. Zegar można podmienić, co bardzo pomaga w testach.
- **Linia 38:** Zapamiętaj zegar.
- **Linia 39:** Blokada, którą ten sam wątek może zająć kilka razy.
- **Linia 40:** Kolejka linii wysyłanych przez układ.
- **Linia 41:** Symulowana pamięć EEPROM: słownik z wartościami domyślnymi.
- **Linia 42:** Komentarz: wirtualny sprzęt to surowe odczyty ADC i temperatura czujnika.
- **Linia 43:** Potencjometr w połowie zakresu.
- **Linia 44:** Fotorezystor w połowie zakresu.
- **Linia 45:** Temperatura 22 °C.
- **Linia 46:** Wykonaj reset jak po włączeniu zasilania.

### Wirtualny sprzęt

```python
 48      # --- wirtualny sprzęt -------------------------------------------------
 49
 50      def reset(self, reason: str = "EXT") -> None:
 51          with self._lock:
 52              self.on = self.eeprom["on"]
 53              self.mode = self.eeprom["mode"]
 54              self.pwm = self.eeprom["pwm"]
 55              self.tlim = self.eeprom["tlim"]
 56              self.alarm = False
 57              self._line = ""
 58              self._too_long = False
 59              self._pot_last: int | None = None
 60              now = self._clock()
 61              self._boot = now
 62              self._next = {"pot": now, "auto": now, "temp": now}
 63              self._emit(f"EVT BOOT reset={reason} ver={FW_VERSION}")
```

- **Linia 48:** Komentarz oddzielający tę część.
- **Linia 50:** Metoda symulująca reset układu. Dostaje przyczynę.
- **Linia 51:** Zajmij blokadę.
- **Linia 52:** Wczytaj stan wyjścia z EEPROM.
- **Linia 53:** Wczytaj tryb.
- **Linia 54:** Wczytaj jasność.
- **Linia 55:** Wczytaj próg alarmu.
- **Linia 56:** Alarm wyłączony.
- **Linia 57:** Pusty bufor odbieranej linii.
- **Linia 58:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 59:** Brak zapamiętanego położenia potencjometru.
- **Linia 60:** Pobierz aktualny czas.
- **Linia 61:** Zapamiętaj chwilę startu.
- **Linia 62:** Wszystkie zadania okresowe mają się wykonać od razu.
- **Linia 63:** Wyślij zdarzenie startu.

```python
 65      def press_button(self, long: bool = False) -> None:
 66          with self._lock:
 67              if long:
 68                  self.mode = "MANUAL" if self.mode == "AUTO" else "AUTO"
 69                  self._save()
 70                  self._emit(f"EVT BTN mode={self.mode}")
 71              else:
 72                  self.on = not self.on
 73                  self._save()
 74                  self._emit(f"EVT BTN on={int(self.on)}")
```

- **Linia 65:** Metoda symulująca naciśnięcie przycisku.
- **Linia 66:** Zajmij blokadę.
- **Linia 67:** Jeśli naciśnięcie jest długie...
- **Linia 68:** ...zmień tryb na przeciwny.
- **Linia 69:** Zapisz w EEPROM.
- **Linia 70:** Wyślij zdarzenie.
- **Linia 71:** W przeciwnym razie, dla krótkiego:
- **Linia 72:** Odwróć stan wyjścia.
- **Linia 73:** Zapisz w EEPROM.
- **Linia 74:** Wyślij zdarzenie. `int(self.on)` zamienia prawdę na 1, a fałsz na 0.

```python
 76      def set_inputs(self, pot: int | None = None, ldr: int | None = None,
 77                     temp_c: float | None = None) -> None:
 78          with self._lock:
 79              if pot is not None:
 80                  self.pot = max(0, min(1023, int(pot)))
 81              if ldr is not None:
 82                  self.ldr = max(0, min(1023, int(ldr)))
 83              if temp_c is not None:
 84                  self.temp_c = max(-20.0, min(150.0, float(temp_c)))
```

- **Linia 76:** Metoda ustawiająca wirtualne wejścia. Parametry są opcjonalne.
- **Linia 77:** Ciąg dalszy listy parametrów.
- **Linia 78:** Zajmij blokadę.
- **Linia 79:** Jeśli podano potencjometr...
- **Linia 80:** ...zapisz wartość przyciętą do zakresu 0–1023.
- **Linia 81:** Jeśli podano fotorezystor...
- **Linia 82:** ...zapisz wartość przyciętą do zakresu 0–1023.
- **Linia 83:** Jeśli podano temperaturę...
- **Linia 84:** ...zapisz wartość przyciętą do zakresu od -20 do 150 °C.

```python
 86      @property
 87      def brightness(self) -> int:
 88          return self.pwm if self.on else 0
 89
 90      def snapshot(self) -> dict:
 91          with self._lock:
 92              return {
 93                  "pot": self.pot,
 94                  "ldr": self.ldr,
 95                  "temp_c": self.temp_c,
 96                  "led": self.brightness,
 97                  "status_alarm": self.alarm,
 98              }
```

- **Linia 86:** Dekorator: metodę czyta się jak pole.
- **Linia 87:** Metoda zwraca jasność diody.
- **Linia 88:** Jasność, gdy wyjście jest włączone, a w przeciwnym razie 0.
- **Linia 90:** Metoda zwraca stan wirtualnej płytki.
- **Linia 91:** Zajmij blokadę.
- **Linia 92:** Zwróć słownik:
- **Linia 93:** Potencjometr.
- **Linia 94:** Fotorezystor.
- **Linia 95:** Temperatura.
- **Linia 96:** Jasność diody.
- **Linia 97:** Czy dioda statusu sygnalizuje alarm.
- **Linia 98:** Koniec słownika.

### Symulowany UART

```python
100      # --- UART -------------------------------------------------------------
101
102      def feed(self, data: str) -> None:
103          with self._lock:
104              for c in data:
105                  self._rx(c)
106
107      def read_line(self, timeout: float = 0.0) -> str | None:
108          try:
109              return self._out.get(timeout=timeout) if timeout > 0 else self._out.get_nowait()
110          except queue.Empty:
111              return None
112
113      def _emit(self, line: str) -> None:
114          self._out.put(line)
```

- **Linia 100:** Komentarz oddzielający tę część.
- **Linia 102:** Metoda przyjmująca tekst od serwera.
- **Linia 103:** Zajmij blokadę.
- **Linia 104:** Przejdź po kolejnych znakach.
- **Linia 105:** Obsłuż każdy znak.
- **Linia 107:** Metoda oddająca jedną linię wysłaną przez układ.
- **Linia 108:** Próbuj.
- **Linia 109:** Jeśli podano czas, czekaj najwyżej tyle. W przeciwnym razie weź od razu, bez czekania.
- **Linia 110:** Jeśli kolejka jest pusta...
- **Linia 111:** ...zwróć `None`.
- **Linia 113:** Metoda „wysyłająca” linię.
- **Linia 114:** Włóż ją do kolejki.

```python
116      def _rx(self, c: str) -> None:
117          if c == "\r":
118              return
119          if c == "\n":
120              line = self._line.strip()
121              if self._too_long:
122                  self._emit("ERR TOOLONG")
123              elif line:
124                  self._execute(line)
125              self._line = ""
126              self._too_long = False
127              return
128          if not self._line and not self._too_long and c in "10":
129              self._set_on(c == "1")
130              self._emit(f"OK on={int(self.on)}")
131              return
132          if len(self._line) >= LINE_MAX:
133              self._too_long = True
134              return
135          self._line += c.upper()
```

- **Linia 116:** Metoda obsługująca jeden znak, odpowiednik `protocol_rx` w firmware.
- **Linia 117:** Jeśli to `\r`...
- **Linia 118:** ...pomiń go.
- **Linia 119:** Jeśli to koniec linii:
- **Linia 120:** Weź zebraną linię bez spacji na brzegach.
- **Linia 121:** Jeśli była zbyt długa...
- **Linia 122:** ...odeślij błąd.
- **Linia 123:** W przeciwnym razie, jeśli nie jest pusta...
- **Linia 124:** ...wykonaj komendę.
- **Linia 125:** Wyzeruj bufor.
- **Linia 126:** Wyzeruj znacznik zbyt długiej linii.
- **Linia 127:** Zakończ.
- **Linia 128:** Jeśli bufor jest pusty, a znak to `1` lub `0`...
- **Linia 129:** ...włącz lub wyłącz wyjście.
- **Linia 130:** Odeślij nowy stan.
- **Linia 131:** Zakończ.
- **Linia 132:** Jeśli linia osiągnęła limit długości...
- **Linia 133:** ...oznacz, że jest zbyt długa...
- **Linia 134:** ...i zakończ.
- **Linia 135:** Dopisz znak do bufora, zamieniając go na wielką literę.

```python
137      def _execute(self, line: str) -> None:
138          cmd, _, arg = line.partition(" ")
139          arg = arg.strip()
```

- **Linia 137:** Metoda wykonująca komendę, odpowiednik `execute` w firmware.
- **Linia 138:** Podziel linię na komendę i argument. Podkreślenie to sama spacja, której nie potrzebujemy.
- **Linia 139:** Usuń ewentualne spacje z argumentu.

```python
141          if cmd == "PWM":
142              value = _parse_uint(arg, 100)
143              if value is None:
144                  self._emit("ERR ARG")
145                  return
146              mode_changed = self.mode != "MANUAL"
147              self._set_pwm(value)
148              self._emit(f"OK pwm={value}" + (" mode=MANUAL" if mode_changed else ""))
```

- **Linia 141:** Komenda `PWM`:
- **Linia 142:** Zamień argument na liczbę z limitem 100.
- **Linia 143:** Jeśli się nie udało...
- **Linia 144:** ...odeślij błąd...
- **Linia 145:** ...i zakończ.
- **Linia 146:** Zapamiętaj, czy tryb zmieni się na ręczny.
- **Linia 147:** Ustaw jasność.
- **Linia 148:** Odeślij odpowiedź, a jeśli tryb się zmienił, dopisz o tym informację.

```python
149          elif cmd == "MODE":
150              if arg not in ("AUTO", "MANUAL"):
151                  self._emit("ERR ARG")
152                  return
153              self.mode = arg
154              self._save()
155              self._emit(f"OK mode={self.mode}")
156          elif cmd == "TLIM":
157              value = _parse_uint(arg, TLIM_MAX)
158              if value is None:
159                  self._emit("ERR ARG")
160                  return
161              self.tlim = value
162              self._save()
163              self._emit(f"OK tlim={value}")
```

- **Linia 149:** Komenda `MODE`:
- **Linia 150:** Jeśli argument nie jest dozwolony...
- **Linia 151:** ...odeślij błąd...
- **Linia 152:** ...i zakończ.
- **Linia 153:** Ustaw tryb.
- **Linia 154:** Zapisz w EEPROM.
- **Linia 155:** Odeślij odpowiedź.
- **Linia 156:** Komenda `TLIM`:
- **Linia 157:** Zamień argument na liczbę z limitem 125.
- **Linia 158:** Jeśli się nie udało...
- **Linia 159:** ...odeślij błąd...
- **Linia 160:** ...i zakończ.
- **Linia 161:** Ustaw próg.
- **Linia 162:** Zapisz w EEPROM.
- **Linia 163:** Odeślij odpowiedź.

```python
164          elif cmd in ("PING", "VER", "ON", "OFF", "TOGGLE", "GET") and arg:
165              self._emit("ERR ARG")
166          elif cmd == "PING":
167              self._emit("PONG")
168          elif cmd == "VER":
169              self._emit(f"VER {FW_VERSION}")
170          elif cmd in ("ON", "OFF", "TOGGLE"):
171              self._set_on(not self.on if cmd == "TOGGLE" else cmd == "ON")
172              self._emit(f"OK on={int(self.on)}")
173          elif cmd == "GET":
174              self._emit(self._state_line())
175          else:
176              self._emit("ERR UNKNOWN")
```

- **Linia 164:** Jeśli komenda nie przyjmuje argumentu, a jakiś podano...
- **Linia 165:** ...odeślij `ERR ARG`.
- **Linia 166:** Komenda `PING`...
- **Linia 167:** ...odpowiedź to `PONG`.
- **Linia 168:** Komenda `VER`...
- **Linia 169:** ...odpowiedź to wersja.
- **Linia 170:** Komendy `ON`, `OFF`, `TOGGLE`:
- **Linia 171:** Dla `TOGGLE` weź stan odwrotny, a dla pozostałych prawdę, jeśli komenda to `ON`.
- **Linia 172:** Odeślij nowy stan.
- **Linia 173:** Komenda `GET`...
- **Linia 174:** ...odeślij pełny stan.
- **Linia 175:** Żadna komenda nie pasuje:
- **Linia 176:** Odeślij `ERR UNKNOWN`.

```python
178      def _state_line(self) -> str:
179          up = int(self._clock() - self._boot)
180          return (f"STATE on={int(self.on)} pwm={self.pwm} mode={self.mode} pot={self.pot} "
181                  f"ldr={self.ldr} temp={self._temp_reading():.1f} alarm={int(self.alarm)} "
182                  f"tlim={self.tlim} up={up}")
```

- **Linia 178:** Metoda budująca linię `STATE`.
- **Linia 179:** Czas pracy w sekundach: teraz minus chwila startu, jako liczba całkowita.
- **Linia 180:** Zwróć tekst, sklejony z trzech kawałków w nawiasach. Pierwszy kawałek: wyjście, jasność, tryb, potencjometr.
- **Linia 181:** Drugi kawałek: fotorezystor, temperatura z jedną cyfrą po przecinku i alarm.
- **Linia 182:** Trzeci kawałek: próg i czas pracy.

### Logika jak w firmware

```python
184      # --- logika jak w firmware ---------------------------------------------
185
186      def _set_on(self, on: bool) -> None:
187          self.on = on
188          self._save()
189
190      def _set_pwm(self, value: int) -> None:
191          self.mode = "MANUAL"
192          self.pwm = value
193          self._save()
194
195      def _save(self) -> None:
196          self.eeprom["on"] = self.on
197          self.eeprom["mode"] = self.mode
198          self.eeprom["tlim"] = self.tlim
199          if self.mode == "MANUAL":
200              self.eeprom["pwm"] = self.pwm
201
202      def _temp_reading(self) -> float:
203          # Kwantyzacja jak w ADC (suma 16 próbek, LM35 10 mV/°C)
204          raw_sum = round(self.temp_c * 10 * 16384 / ADC_VREF_MV)
205          raw_sum = max(0, min(16368, raw_sum))
206          return ((raw_sum * ADC_VREF_MV + 8192) >> 14) / 10
```

- **Linia 184:** Komentarz oddzielający tę część.
- **Linia 186:** Metoda ustawiająca stan wyjścia.
- **Linia 187:** Zapisz stan.
- **Linia 188:** Zapisz w EEPROM.
- **Linia 190:** Metoda ustawiająca jasność.
- **Linia 191:** Przełącz na tryb ręczny.
- **Linia 192:** Zapisz jasność.
- **Linia 193:** Zapisz w EEPROM.
- **Linia 195:** Metoda zapisu do symulowanego EEPROM.
- **Linia 196:** Zapisz stan wyjścia.
- **Linia 197:** Zapisz tryb.
- **Linia 198:** Zapisz próg.
- **Linia 199:** Jeśli tryb jest ręczny...
- **Linia 200:** ...zapisz jasność. W trybie AUTO jej nie zapisujemy, tak samo jak w firmware.
- **Linia 202:** Metoda wyliczająca temperaturę „zmierzoną” przez układ.
- **Linia 203:** Komentarz: wynik jest zaokrąglany tak jak w przetworniku ADC: suma 16 próbek, 10 mV na stopień.
- **Linia 204:** Przelicz temperaturę na sumę odczytów ADC.
- **Linia 205:** Przytnij sumę do możliwego zakresu.
- **Linia 206:** Przelicz z powrotem wzorem z firmware. Wynik dzielimy przez 10, by otrzymać stopnie.

### Zadania okresowe

```python
208      def tick(self) -> None:
209          """Okresowe zadania firmware: potencjometr, tryb AUTO, alarm temperatury."""
210          with self._lock:
211              now = self._clock()
212              if now >= self._next["pot"]:
213                  self._next["pot"] = now + POT_PERIOD
214                  self._pot_check()
215              if now >= self._next["auto"]:
216                  self._next["auto"] = now + AUTO_PERIOD
217                  self._auto_check()
218              if now >= self._next["temp"]:
219                  self._next["temp"] = now + TEMP_PERIOD
220                  self._temp_check()
```

- **Linia 208:** Metoda odpowiadająca pętli głównej firmware.
- **Linia 209:** Docstring: zadania okresowe, czyli potencjometr, tryb AUTO i alarm temperatury.
- **Linia 210:** Zajmij blokadę.
- **Linia 211:** Pobierz aktualny czas.
- **Linia 212:** Jeśli nadszedł czas sprawdzenia potencjometru...
- **Linia 213:** ...ustal kolejny termin...
- **Linia 214:** ...i go sprawdź.
- **Linia 215:** Jeśli nadszedł czas przeliczenia trybu AUTO...
- **Linia 216:** ...ustal kolejny termin...
- **Linia 217:** ...i przelicz.
- **Linia 218:** Jeśli nadszedł czas sprawdzenia temperatury...
- **Linia 219:** ...ustal kolejny termin...
- **Linia 220:** ...i ją sprawdź.

```python
222      def _pot_check(self) -> None:
223          p = pot_percent(self.pot)
224          if self._pot_last is None:
225              self._pot_last = p
226              return
227          if p == self._pot_last:
228              return
229          if abs(p - self._pot_last) < POT_HYST and p not in (0, 100):
230              return
231          self._pot_last = p
232          mode_changed = self.mode != "MANUAL"
233          self._set_pwm(p)
234          self._emit(f"EVT POT pwm={p}" + (" mode=MANUAL" if mode_changed else ""))
```

- **Linia 222:** Metoda sprawdzająca potencjometr.
- **Linia 223:** Przelicz odczyt na procenty.
- **Linia 224:** Jeśli to pierwszy odczyt...
- **Linia 225:** ...zapamiętaj go jako punkt odniesienia...
- **Linia 226:** ...i zakończ.
- **Linia 227:** Jeśli nic się nie zmieniło...
- **Linia 228:** ...zakończ.
- **Linia 229:** Jeśli zmiana jest mniejsza niż margines, a pokrętło nie stoi na skraju...
- **Linia 230:** ...zakończ.
- **Linia 231:** Zapamiętaj nowe położenie.
- **Linia 232:** Zapamiętaj, czy tryb się zmieni.
- **Linia 233:** Ustaw jasność.
- **Linia 234:** Wyślij zdarzenie.

```python
236      def _auto_check(self) -> None:
237          if self.mode != "AUTO":
238              return
239          p = auto_percent(self.ldr)
240          if p != self.pwm and (abs(p - self.pwm) >= AUTO_HYST or p in (0, 100)):
241              self.pwm = p
```

- **Linia 236:** Metoda dopasowująca jasność w trybie AUTO.
- **Linia 237:** Jeśli tryb jest inny...
- **Linia 238:** ...zakończ.
- **Linia 239:** Wylicz jasność ze światła.
- **Linia 240:** Jeśli jest inna i różnica jest dostatecznie duża albo to skraj...
- **Linia 241:** ...ustaw ją.

```python
243      def _temp_check(self) -> None:
244          t = self._temp_reading()
245          if not self.alarm and t >= self.tlim:
246              self.alarm = True
247              self._emit(f"EVT TEMP alarm=1 temp={t:.1f}")
248          elif self.alarm and t < self.tlim - TEMP_HYST:
249              self.alarm = False
250              self._emit(f"EVT TEMP alarm=0 temp={t:.1f}")
```

- **Linia 243:** Metoda sprawdzająca alarm temperatury.
- **Linia 244:** Odczytaj temperaturę.
- **Linia 245:** Jeśli alarmu nie ma, a próg został osiągnięty...
- **Linia 246:** ...włącz alarm...
- **Linia 247:** ...i wyślij zdarzenie.
- **Linia 248:** Jeśli alarm trwa, a temperatura spadła poniżej progu minus histereza...
- **Linia 249:** ...wyłącz alarm...
- **Linia 250:** ...i wyślij zdarzenie.

### Zamiana tekstu na liczbę

```python
253  def _parse_uint(text: str, maximum: int) -> int | None:
254      if not text.isdigit() or not text.isascii():
255          return None
256      value = int(text)
257      return value if value <= maximum else None
```

- **Linia 253:** Funkcja zamienia tekst na liczbę, a przy błędzie zwraca `None`.
- **Linia 254:** Jeśli tekst nie składa się z samych cyfr ASCII...
- **Linia 255:** ...zwróć `None`.
- **Linia 256:** Zamień tekst na liczbę.
- **Linia 257:** Zwróć ją, jeśli mieści się w limicie, a w przeciwnym razie `None`.

---

[← Poprzedni](34-serwer-routes.md) · [Spis treści](README.md) · [Następny →](36-serwer-avrsim.md)
