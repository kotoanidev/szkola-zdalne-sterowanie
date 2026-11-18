# Serwer: `avrsim.py`, sterowanie prawdziwym firmware w symulatorze

[← Poprzedni](35-serwer-mcu-sim.md) · [Spis treści](README.md) · [Następny →](37-serwer-wdrozenie.md)

**Po co ten plik:** tryb `SIMULATE=avr` uruchamia prawdziwy plik `firmware.hex` w emulatorze **simavr**. Emulator to osobny program (`sim/avrsim`). Ta klasa uruchamia go, wysyła mu polecenia przez jego wejście i czyta zdarzenia z jego wyjścia.

Serwer łączy się z emulowanym układem przez **pseudoterminal**, czyli programową imitację portu szeregowego, dokładnie tak jak z prawdziwym sprzętem.

## `server/app/avrsim.py`

[Otwórz ten plik w repozytorium](../../server/app/avrsim.py)

```python
  1  """Prawdziwy firmware w simavr (sim/avrsim) – wirtualna płytka dla trybu SIMULATE=avr.
  2
  3  Interfejs jak w McuSim (set_inputs, press_button, reset, snapshot), a UART jest
  4  pseudoterminalem, z którym serwer łączy się zwykłym SerialTransport. Tylko Linux.
  5  """
  6
  7  from __future__ import annotations
  8
  9  import json
 10  import math
 11  import subprocess
 12  import threading
 13  from pathlib import Path
 14  from typing import Callable
 15
 16  REPO = Path(__file__).resolve().parents[2]
 17  DEFAULT_BINARY = REPO / "sim" / "build" / "avrsim"
 18  DEFAULT_HEX = REPO / "firmware" / "build" / "firmware.hex"
 19  VREF_MV = 5000
 20  LONG_PRESS_MS = 1500
```

- **Linia 1:** Początek docstringa: prawdziwy firmware w simavr, czyli wirtualna płytka dla trybu `SIMULATE=avr`.
- **Linia 3:** Metody są takie same jak w `McuSim`, a UART jest...
- **Linia 4:** ...pseudoterminalem, z którym serwer łączy się zwykłym `SerialTransport`. Działa to tylko w Linuksie.
- **Linia 5:** Koniec docstringa.
- **Linia 7:** Nowszy zapis podpowiedzi typów.
- **Linia 9:** Biblioteka JSON. Emulator wysyła zdarzenia w tym formacie.
- **Linia 10:** Biblioteka matematyczna, tu pierwiastek.
- **Linia 11:** Biblioteka do uruchamiania innych programów.
- **Linia 12:** Biblioteka wątków.
- **Linia 13:** Narzędzie do pracy ze ścieżkami plików.
- **Linia 14:** Narzędzie do opisu typu funkcji.
- **Linia 16:** Główny katalog repozytorium: ścieżka tego pliku, a potem dwa poziomy w górę.
- **Linia 17:** Domyślna ścieżka programu emulatora.
- **Linia 18:** Domyślna ścieżka firmware.
- **Linia 19:** Napięcie odniesienia w miliwoltach.
- **Linia 20:** Długie naciśnięcie przycisku trwa 1500 ms.

```python
 23  class AvrSimError(Exception):
 24      pass
```

- **Linia 23:** Własny rodzaj błędu dla problemów z emulatorem.
- **Linia 24:** Klasa nie ma dodatkowej treści.

### Uruchomienie

```python
 27  class AvrSim:
 28      def __init__(self, binary: Path = DEFAULT_BINARY, hex_file: Path = DEFAULT_HEX,
 29                   speed: float = 1.0, eeprom: Path | None = None) -> None:
 30          for path, hint in ((binary, "make -C sim"), (hex_file, "make -C firmware")):
 31              if not Path(path).exists():
 32                  raise AvrSimError(f"brak {path} – uruchom: {hint}")
 33          args = [str(binary), "--speed", str(speed)]
 34          if eeprom is not None:
 35              args += ["--eeprom", str(eeprom)]
 36          args.append(str(hex_file))
```

- **Linia 27:** Początek klasy.
- **Linia 28:** Funkcja tworząca obiekt. Dostaje ścieżki emulatora i firmware...
- **Linia 29:** ...szybkość symulacji i plik na EEPROM.
- **Linia 30:** Dla każdej z dwóch ścieżek, wraz z podpowiedzią, jak zbudować brakujący plik...
- **Linia 31:** ...jeśli pliku nie ma...
- **Linia 32:** ...zgłoś błąd z podpowiedzią.
- **Linia 33:** Zbuduj listę argumentów programu.
- **Linia 34:** Jeśli podano plik EEPROM...
- **Linia 35:** ...dopisz go do listy.
- **Linia 36:** Na końcu dopisz ścieżkę firmware.

```python
 38          self.pot = 512
 39          self.ldr = 500
 40          self.temp_c = 22.0
 41          self.duty = 0.0
 42          self.status = False
 43          self.pty: str | None = None
 44          self.on_change: Callable[[dict], None] | None = None
 45          self.tx_log: list[str] = []
 46          self._lock = threading.Lock()
 47          self._ready = threading.Event()
```

- **Linia 38:** Potencjometr w połowie zakresu.
- **Linia 39:** Fotorezystor w połowie zakresu.
- **Linia 40:** Temperatura 22 °C.
- **Linia 41:** Wypełnienie PWM, na razie 0.
- **Linia 42:** Dioda statusu, na razie zgaszona.
- **Linia 43:** Ścieżka pseudoterminala, jeszcze nieznana.
- **Linia 44:** Funkcja wywoływana przy zmianie stanu, na razie brak.
- **Linia 45:** Lista linii wysłanych przez układ.
- **Linia 46:** Blokada chroniąca wspólne dane.
- **Linia 47:** Flaga: emulator jest gotowy.

```python
 48          self._proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
 49                                        text=True, bufsize=1)
 50          self._reader = threading.Thread(target=self._read_events, name="avrsim", daemon=True)
 51          self._reader.start()
 52          if not self._ready.wait(5):
 53              self.close()
 54              raise AvrSimError("avrsim nie wystartował")
 55          self.set_inputs(self.pot, self.ldr, self.temp_c)
```

- **Linia 48:** Uruchom emulator z połączonym wejściem i wyjściem...
- **Linia 49:** ...tekstowo i z buforowaniem liniowym, czyli każda linia idzie od razu.
- **Linia 50:** Utwórz wątek czytający zdarzenia emulatora.
- **Linia 51:** Uruchom ten wątek.
- **Linia 52:** Czekaj najwyżej 5 s na zdarzenie gotowości. Jeśli go nie ma...
- **Linia 53:** ...zamknij emulator...
- **Linia 54:** ...i zgłoś błąd.
- **Linia 55:** Przekaż emulatorowi początkowe wartości wejść.

### Zdarzenia od emulatora

```python
 57      # --- zdarzenia z symulatora --------------------------------------------------
 58
 59      def _read_events(self) -> None:
 60          assert self._proc.stdout is not None
 61          for line in self._proc.stdout:
 62              try:
 63                  ev = json.loads(line)
 64              except json.JSONDecodeError:
 65                  continue
 66              kind = ev.get("ev")
 67              changed = False
 68              with self._lock:
 69                  if kind == "ready":
 70                      self.pty = ev["pty"]
 71                      self._ready.set()
 72                  elif kind == "led":
 73                      self.duty = float(ev["duty"])
 74                      changed = True
 75                  elif kind == "status":
 76                      self.status = bool(ev["on"])
 77                      changed = True
 78                  elif kind == "tx":
 79                      self.tx_log.append(ev["line"])
 80                      del self.tx_log[:-200]
 81              if changed and self.on_change is not None:
 82                  self.on_change(self.snapshot())
```

- **Linia 57:** Komentarz oddzielający tę część.
- **Linia 59:** Funkcja wątku czytającego.
- **Linia 60:** Sprawdzenie, że wyjście emulatora istnieje. Pomaga narzędziom sprawdzającym typy.
- **Linia 61:** Czytaj emulator linia po linii. Pętla kończy się, gdy emulator się zamknie.
- **Linia 62:** Próbuj.
- **Linia 63:** Zamień tekst JSON na słownik.
- **Linia 64:** Jeśli to nie jest poprawny JSON...
- **Linia 65:** ...pomiń tę linię.
- **Linia 66:** Odczytaj rodzaj zdarzenia.
- **Linia 67:** Na razie stan się nie zmienił.
- **Linia 68:** Zajmij blokadę.
- **Linia 69:** Zdarzenie gotowości:
- **Linia 70:** Zapamiętaj ścieżkę pseudoterminala.
- **Linia 71:** Podnieś flagę gotowości.
- **Linia 72:** Zdarzenie zmiany diody:
- **Linia 73:** Zapisz wypełnienie PWM.
- **Linia 74:** Oznacz zmianę.
- **Linia 75:** Zdarzenie diody statusu:
- **Linia 76:** Zapisz jej stan.
- **Linia 77:** Oznacz zmianę.
- **Linia 78:** Zdarzenie wysłanej linii:
- **Linia 79:** Dopisz ją do listy.
- **Linia 80:** Usuń starsze wpisy, zostaw 200 ostatnich.
- **Linia 81:** Jeśli stan się zmienił i ktoś chce o tym wiedzieć...
- **Linia 82:** ...wywołaj jego funkcję z aktualnym stanem.

```python
 84      def _send(self, line: str) -> None:
 85          if self._proc.poll() is not None or self._proc.stdin is None:
 86              raise AvrSimError("avrsim nie działa")
 87          self._proc.stdin.write(line + "\n")
 88          self._proc.stdin.flush()
```

- **Linia 84:** Metoda wysyłająca polecenie do emulatora.
- **Linia 85:** Jeśli emulator się zakończył albo nie ma wejścia...
- **Linia 86:** ...zgłoś błąd.
- **Linia 87:** Wpisz polecenie z końcem linii.
- **Linia 88:** Wymuś natychmiastowe wysłanie.

### Wirtualny sprzęt

```python
 90      # --- wirtualny sprzęt (jak McuSim) ------------------------------------------------
 91
 92      def set_inputs(self, pot: int | None = None, ldr: int | None = None,
 93                     temp_c: float | None = None) -> None:
 94          if pot is not None:
 95              self.pot = max(0, min(1023, int(pot)))
 96              self._send(f"pot {self.pot}")
 97          if ldr is not None:
 98              self.ldr = max(0, min(1023, int(ldr)))
 99              self._send(f"ldr {self.ldr}")
100          if temp_c is not None:
101              self.temp_c = max(-55.0, min(125.0, float(temp_c)))
102              self._send(f"temp {self.temp_c}")
103
104      def press_button(self, long: bool = False) -> None:
105          self._send(f"btn press {LONG_PRESS_MS if long else 100}")
106
107      def reset(self, reason: str = "EXT") -> None:
108          self._send("power" if reason == "POR" else "reset")
```

- **Linia 90:** Komentarz oddzielający tę część.
- **Linia 92:** Metoda ustawiająca wejścia. Parametry są opcjonalne.
- **Linia 93:** Ciąg dalszy listy parametrów.
- **Linia 94:** Jeśli podano potencjometr...
- **Linia 95:** ...zapisz wartość przyciętą do zakresu 0–1023...
- **Linia 96:** ...i przekaż ją emulatorowi.
- **Linia 97:** Jeśli podano fotorezystor...
- **Linia 98:** ...zapisz wartość przyciętą do zakresu 0–1023...
- **Linia 99:** ...i przekaż ją emulatorowi.
- **Linia 100:** Jeśli podano temperaturę...
- **Linia 101:** ...zapisz wartość przyciętą do zakresu czujnika DS18B20, od -55 do 125 °C...
- **Linia 102:** ...i przekaż ją emulatorowi.
- **Linia 104:** Metoda naciskająca wirtualny przycisk.
- **Linia 105:** Wyślij polecenie z czasem naciśnięcia: 1500 ms dla długiego, 100 ms dla krótkiego.
- **Linia 107:** Metoda resetująca układ.
- **Linia 108:** Dla przyczyny `POR` symuluj zanik zasilania, a dla innej reset zewnętrzny.

```python
110      def snapshot(self) -> dict:
111          with self._lock:
112              return {
113                  "pot": self.pot,
114                  "ldr": self.ldr,
115                  "temp_c": self.temp_c,
116                  # Jasność odczuwana z wypełnienia PWM (firmware stosuje korekcję gamma 2)
117                  "led": round(math.sqrt(self.duty) * 100),
118                  "duty": self.duty,
119                  "status": self.status,
120              }
```

- **Linia 110:** Metoda zwracająca stan wirtualnej płytki.
- **Linia 111:** Zajmij blokadę.
- **Linia 112:** Zwróć słownik:
- **Linia 113:** Potencjometr.
- **Linia 114:** Fotorezystor.
- **Linia 115:** Temperatura.
- **Linia 116:** Komentarz: jasność odczuwaną liczymy z wypełnienia, odwracając korekcję gamma.
- **Linia 117:** Firmware podnosi jasność do kwadratu, więc tu liczymy pierwiastek. Wynik w procentach.
- **Linia 118:** Wypełnienie PWM.
- **Linia 119:** Stan diody statusu.
- **Linia 120:** Koniec słownika.

### Zamykanie

```python
122      def close(self) -> None:
123          if self._proc.poll() is None:
124              try:
125                  self._send("quit")
126                  self._proc.wait(timeout=2)
127              except (AvrSimError, OSError, subprocess.TimeoutExpired):
128                  self._proc.kill()
129                  self._proc.wait()
```

- **Linia 122:** Metoda zamykająca emulator.
- **Linia 123:** Jeśli emulator nadal działa...
- **Linia 124:** Próbuj zamknąć go grzecznie.
- **Linia 125:** Wyślij polecenie `quit`.
- **Linia 126:** Poczekaj na zakończenie, najwyżej 2 s.
- **Linia 127:** Jeśli coś pójdzie nie tak...
- **Linia 128:** ...zakończ go siłą.
- **Linia 129:** Poczekaj, aż system po nim posprząta.

---

[← Poprzedni](35-serwer-mcu-sim.md) · [Spis treści](README.md) · [Następny →](37-serwer-wdrozenie.md)
