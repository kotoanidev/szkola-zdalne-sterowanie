# Testy: `test_mcu_sim.py`, sprawdzanie symulatora w Pythonie

[← Poprzedni](51-testy-api.md) · [Spis treści](README.md) · [Następny →](53-testy-firmware-avr.md)

**Po co ten plik:** symulator ma zachowywać się jak firmware. Te testy sprawdzają to zgodnie z umową z `docs/protokol.md`. Korzystają ze sztucznego zegara, dzięki czemu „mija” tylko tyle czasu, ile każe test, i nic nie trzeba naprawdę czekać.

## `server/tests/test_mcu_sim.py`

[Otwórz ten plik w repozytorium](../../server/tests/test_mcu_sim.py)

```python
  1  """Symulator musi zachowywać się jak firmware (docs/protokol.md)."""
  2
  3  from app.mcu_sim import McuSim, auto_percent, pot_percent
```

- **Linia 1:** Docstring: symulator musi zachowywać się jak firmware.
- **Linia 3:** Pobierz symulator i dwie funkcje przeliczające.

```python
  6  def send(sim, text):
  7      sim.feed(text)
  8      lines = []
  9      while (line := sim.read_line()) is not None:
 10          lines.append(line)
 11      return lines
```

- **Linia 6:** Funkcja pomocnicza: wyślij tekst do symulatora i zbierz wszystkie odpowiedzi.
- **Linia 7:** Wyślij tekst.
- **Linia 8:** Pusta lista na odpowiedzi.
- **Linia 9:** Dopóki symulator ma do oddania jakąś linię. `:=` jednocześnie przypisuje i sprawdza.
- **Linia 10:** Dopisz linię do listy.
- **Linia 11:** Zwróć listę.

### Komendy

```python
 14  def test_boot_event(clock):
 15      sim = McuSim(clock=clock)
 16      assert sim.read_line().startswith("EVT BOOT reset=POR ver=")
```

- **Linia 14:** Test: zdarzenie startu.
- **Linia 15:** Utwórz symulator.
- **Linia 16:** Pierwsza linia musi zaczynać się od tego tekstu.

```python
 19  def test_compat_single_chars(sim):
 20      assert send(sim, "1") == ["OK on=1"]
 21      assert send(sim, "0\r\n") == ["OK on=0"]
 22      assert send(sim, "10") == ["OK on=1", "OK on=0"]
```

- **Linia 19:** Test: tryb zgodności z instrukcją.
- **Linia 20:** Znak `1` działa od razu, bez końca linii.
- **Linia 21:** Znak `0` też, a `\r\n` jest pomijane.
- **Linia 22:** Dwa znaki to dwie odpowiedzi.

```python
 25  def test_basic_commands(sim):
 26      assert send(sim, "ping\n") == ["PONG"]
 27      assert send(sim, "VER\n")[0].startswith("VER ")
 28      assert send(sim, "on\n") == ["OK on=1"]
 29      assert send(sim, "TOGGLE\n") == ["OK on=0"]
 30      assert send(sim, "  pwm 60  \n") == ["OK pwm=60"]
 31      assert send(sim, "MODE AUTO\n") == ["OK mode=AUTO"]
 32      assert send(sim, "TLIM 30\n") == ["OK tlim=30"]
```

- **Linia 25:** Test: podstawowe komendy.
- **Linia 26:** Komenda małymi literami też działa.
- **Linia 27:** Odpowiedź na `VER` zaczyna się od `VER `.
- **Linia 28:** Komenda `on` włącza wyjście.
- **Linia 29:** `TOGGLE` je odwraca.
- **Linia 30:** Dodatkowe spacje nie przeszkadzają.
- **Linia 31:** Zmiana trybu.
- **Linia 32:** Zmiana progu alarmu.

```python
 35  def test_errors(sim):
 36      assert send(sim, "FOO\n") == ["ERR UNKNOWN"]
 37      assert send(sim, "PWM\n") == ["ERR ARG"]
 38      assert send(sim, "PWM 101\n") == ["ERR ARG"]
 39      assert send(sim, "PWM -1\n") == ["ERR ARG"]
 40      assert send(sim, "MODE X\n") == ["ERR ARG"]
 41      assert send(sim, "TLIM 126\n") == ["ERR ARG"]
 42      assert send(sim, "PING 1\n") == ["ERR ARG"]
 43      assert send(sim, "A" * 32 + "\n") == ["ERR UNKNOWN"]
 44      assert send(sim, "A" * 33 + "\n") == ["ERR TOOLONG"]
 45      assert send(sim, "\n\n") == []
```

- **Linia 35:** Test: błędy.
- **Linia 36:** Nieznana komenda.
- **Linia 37:** `PWM` bez argumentu.
- **Linia 38:** Jasność poza zakresem.
- **Linia 39:** Liczba ujemna.
- **Linia 40:** Nieznany tryb.
- **Linia 41:** Próg poza zakresem.
- **Linia 42:** `PING` z niepotrzebnym argumentem.
- **Linia 43:** 32 znaki mieszczą się w limicie, więc to tylko nieznana komenda.
- **Linia 44:** 33 znaki przekraczają limit.
- **Linia 45:** Puste linie nie dają żadnej odpowiedzi.

```python
 48  def test_pwm_in_auto_switches_to_manual(sim):
 49      send(sim, "MODE AUTO\n")
 50      assert send(sim, "PWM 40\n") == ["OK pwm=40 mode=MANUAL"]
```

- **Linia 48:** Test: `PWM` w trybie AUTO przełącza na ręczny.
- **Linia 49:** Włącz tryb AUTO.
- **Linia 50:** Odpowiedź zawiera informację o zmianie trybu.

```python
 53  def test_get_state_line(sim, clock):
 54      send(sim, "ON\nPWM 60\n")
 55      clock.advance(12.4)
 56      line = send(sim, "GET\n")[0]
 57      assert line == "STATE on=1 pwm=60 mode=MANUAL pot=512 ldr=500 temp=22.0 alarm=0 tlim=50 up=12"
```

- **Linia 53:** Test: pełna linia stanu.
- **Linia 54:** Włącz wyjście i ustaw jasność.
- **Linia 55:** Przesuń zegar o 12,4 s.
- **Linia 56:** Wyślij `GET` i weź pierwszą odpowiedź.
- **Linia 57:** Cała linia musi być dokładnie taka. Czas pracy to 12 s.

### Potencjometr

```python
 60  def test_pot_event_with_hysteresis(sim, clock):
 61      sim.tick()  # odczyt bazowy
 62      sim.set_inputs(pot=pot_raw(51))
 63      clock.advance(0.05)
 64      sim.tick()
 65      assert send(sim, "") == []
 66      sim.set_inputs(pot=pot_raw(60))
 67      clock.advance(0.05)
 68      sim.tick()
 69      assert send(sim, "") == ["EVT POT pwm=60"]
 70      assert sim.pwm == 60
```

- **Linia 60:** Test: potencjometr z marginesem.
- **Linia 61:** Pierwsze sprawdzenie, czyli zapamiętanie punktu odniesienia.
- **Linia 62:** Ustaw potencjometr na 51 %, czyli blisko wartości startowej.
- **Linia 63:** Przesuń zegar o czas sprawdzenia.
- **Linia 64:** Uruchom zadania okresowe.
- **Linia 65:** Zbyt mała zmiana nie daje żadnego zdarzenia.
- **Linia 66:** Ustaw potencjometr na 60 %.
- **Linia 67:** Przesuń zegar.
- **Linia 68:** Uruchom zadania okresowe.
- **Linia 69:** Teraz musi być zdarzenie.
- **Linia 70:** I nowa jasność.

```python
 73  def test_pot_switches_auto_to_manual(sim, clock):
 74      send(sim, "MODE AUTO\n")
 75      sim.tick()
 76      sim.set_inputs(pot=0)
 77      clock.advance(0.05)
 78      sim.tick()
 79      assert send(sim, "") == ["EVT POT pwm=0 mode=MANUAL"]
```

- **Linia 73:** Test: potencjometr wyłącza tryb AUTO.
- **Linia 74:** Włącz tryb AUTO.
- **Linia 75:** Punkt odniesienia.
- **Linia 76:** Skręć potencjometr na zero.
- **Linia 77:** Przesuń zegar.
- **Linia 78:** Uruchom zadania okresowe.
- **Linia 79:** Zdarzenie zawiera informację o zmianie trybu.

### Tryb AUTO

```python
 82  def test_auto_mode_follows_light(sim, clock):
 83      send(sim, "ON\nMODE AUTO\n")
 84      sim.set_inputs(ldr=100)
 85      clock.advance(0.2)
 86      sim.tick()
 87      assert sim.pwm == 100
 88      sim.set_inputs(ldr=900)
 89      clock.advance(0.2)
 90      sim.tick()
 91      assert sim.pwm == 0
 92      assert auto_percent(500) == 50
```

- **Linia 82:** Test: tryb AUTO reaguje na światło.
- **Linia 83:** Włącz wyjście i tryb AUTO.
- **Linia 84:** Ciemno.
- **Linia 85:** Przesuń zegar o czas przeliczenia.
- **Linia 86:** Uruchom zadania okresowe.
- **Linia 87:** Jasność musi wynosić 100 %.
- **Linia 88:** Jasno.
- **Linia 89:** Przesuń zegar.
- **Linia 90:** Uruchom zadania okresowe.
- **Linia 91:** Jasność musi wynosić 0 %.
- **Linia 92:** Dla środka zakresu funkcja daje 50 %.

### Alarm temperatury

```python
 95  def test_temperature_alarm_hysteresis(sim, clock):
 96      send(sim, "TLIM 30\n")
 97      sim.set_inputs(temp_c=31)
 98      clock.advance(0.5)
 99      sim.tick()
100      assert send(sim, "") == ["EVT TEMP alarm=1 temp=31.0"]
101      sim.set_inputs(temp_c=29)
102      clock.advance(0.5)
103      sim.tick()
104      assert send(sim, "") == []
105      sim.set_inputs(temp_c=27.5)
106      clock.advance(0.5)
107      sim.tick()
108      assert send(sim, "") == ["EVT TEMP alarm=0 temp=27.5"]
```

- **Linia 95:** Test: alarm z histerezą.
- **Linia 96:** Ustaw próg na 30 °C.
- **Linia 97:** Temperatura powyżej progu.
- **Linia 98:** Przesuń zegar.
- **Linia 99:** Uruchom zadania okresowe.
- **Linia 100:** Alarm się włączył.
- **Linia 101:** 29 °C jest poniżej progu, ale w marginesie 2 °C.
- **Linia 102:** Przesuń zegar.
- **Linia 103:** Uruchom zadania okresowe.
- **Linia 104:** Alarm dalej trwa, bo nie ma żadnego zdarzenia.
- **Linia 105:** 27,5 °C jest poniżej progu minus margines.
- **Linia 106:** Przesuń zegar.
- **Linia 107:** Uruchom zadania okresowe.
- **Linia 108:** Alarm się wyłączył.

### Przycisk i pamięć

```python
111  def test_button(sim):
112      sim.press_button()
113      assert send(sim, "") == ["EVT BTN on=1"]
114      sim.press_button(long=True)
115      assert send(sim, "") == ["EVT BTN mode=AUTO"]
```

- **Linia 111:** Test: przycisk.
- **Linia 112:** Krótkie naciśnięcie.
- **Linia 113:** Zdarzenie o włączeniu.
- **Linia 114:** Długie naciśnięcie.
- **Linia 115:** Zdarzenie o zmianie trybu.

```python
118  def test_eeprom_restore_after_reset(sim):
119      send(sim, "ON\nPWM 70\nTLIM 40\n")
120      sim.reset("WDT")
121      assert send(sim, "") == ["EVT BOOT reset=WDT ver=0.10.0-sim"]
122      assert (sim.on, sim.pwm, sim.tlim, sim.mode) == (True, 70, 40, "MANUAL")
```

- **Linia 118:** Test: odtworzenie stanu po resecie.
- **Linia 119:** Zmień kilka wartości.
- **Linia 120:** Zresetuj układ, jakby zrobił to watchdog.
- **Linia 121:** Układ zgłasza przyczynę resetu.
- **Linia 122:** Wartości wróciły z pamięci.

```python
125  def test_auto_brightness_not_saved(sim, clock):
126      send(sim, "PWM 30\nMODE AUTO\n")
127      sim.set_inputs(ldr=100)
128      clock.advance(0.2)
129      sim.tick()
130      assert sim.pwm == 100
131      sim.reset()
132      assert (sim.mode, sim.pwm) == ("AUTO", 30)
```

- **Linia 125:** Test: jasność z trybu AUTO nie jest zapisywana.
- **Linia 126:** Ustaw jasność 30 % i tryb AUTO.
- **Linia 127:** Ciemno.
- **Linia 128:** Przesuń zegar.
- **Linia 129:** Uruchom zadania okresowe.
- **Linia 130:** Tryb AUTO podniósł jasność do 100 %.
- **Linia 131:** Zresetuj układ.
- **Linia 132:** Po resecie wraca jasność 30 %.

### Funkcja pomocnicza

```python
135  def pot_raw(percent):
136      return next(raw for raw in range(1024) if pot_percent(raw) == percent)
```

- **Linia 135:** Funkcja znajduje surowy odczyt dający podany procent.
- **Linia 136:** Sprawdza kolejno wszystkie wartości od 0 do 1023 i zwraca pierwszą pasującą.

---

[← Poprzedni](51-testy-api.md) · [Spis treści](README.md) · [Następny →](53-testy-firmware-avr.md)
