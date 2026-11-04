# Symulator: `sim/Makefile`

[← Poprzedni](53-testy-firmware-avr.md) · [Spis treści](README.md) · [Następny →](61-sim-avrsim-c.md)

**Po co ten plik:** buduje emulator `avrsim`. Działa tylko w Linuksie, na komputerze domowym, i nie jest potrzebny w szkole. Pobiera bibliotekę **simavr**, która udaje układ AVR, buduje ją i łączy z naszym programem. Składnia jest wytłumaczona w [podstawach narzędzi](04-podstawy-narzedzi.md).

## `sim/Makefile`

[Otwórz ten plik w repozytorium](../../sim/Makefile)

```makefile
  1  # Symulacja firmware w simavr - tylko Linux (komputer domowy), nie jest potrzebna w szkole.
  2  #
  3  #   make           pobiera i buduje simavr (raz), buduje build/avrsim i firmware
  4  #   make run       firmware w symulatorze; UART na pseudoterminalu build/uart
  5  #   make test      testy prawdziwego firmware (pytest z venv serwera)
  6  #   make clean     usuwa build/
  7  #   make distclean usuwa tez pobrany simavr (.deps/)
```

- **Linia 1:** Komentarz: symulacja firmware w simavr, tylko Linux, w szkole niepotrzebna.
- **Linia 2:** Pusty komentarz, odstęp.
- **Linia 3:** Instrukcja: samo `make` pobiera i buduje simavr, buduje `avrsim` i firmware.
- **Linia 4:** Instrukcja: `make run` uruchamia firmware w symulatorze, a UART jest dostępny przez `build/uart`.
- **Linia 5:** Instrukcja: `make test` uruchamia testy prawdziwego firmware.
- **Linia 6:** Instrukcja: `make clean` usuwa katalog `build`.
- **Linia 7:** Instrukcja: `make distclean` usuwa też pobrany simavr.

```makefile
  9  SIMAVR_URL    = https://github.com/buserror/simavr
 10  SIMAVR_TAG    = v1.8
 11  SIMAVR_COMMIT = f44723e8c42431136d5b4de81f789ded56d7e8fa
 12  SIMAVR        = .deps/simavr
 13  SIMAVR_LIB    = $(SIMAVR)/simavr/obj-$(shell $(CC) -dumpmachine)/libsimavr.a
```

- **Linia 9:** Adres repozytorium simavr.
- **Linia 10:** Wersja, którą pobieramy: `v1.8`.
- **Linia 11:** Dokładny identyfikator commita tej wersji. Służy do kontroli, czy pobrano właściwy kod.
- **Linia 12:** Katalog na pobrany kod.
- **Linia 13:** Ścieżka biblioteki po zbudowaniu. `$(shell ...)` pyta kompilator o nazwę platformy, która jest częścią ścieżki.

```makefile
 15  # Poprawka bledu simavr 1.8: avr_reset() odwoluje sie do irq->pool, ktorego IRQ watchdoga
 16  # nie ma (segfault przy resecie, takze od watchdoga)
 17  SIMAVR_PATCHED = $(SIMAVR)/.patched-irq-pool
```

- **Linia 15:** Komentarz: poprawka błędu w simavr 1.8. Funkcja `avr_reset()` odwołuje się do `irq->pool`, którego przerwanie watchdoga nie ma...
- **Linia 16:** ...co kończy się awarią (*segfault*) przy resecie.
- **Linia 17:** Plik-znacznik, który mówi, że poprawkę już nałożono.

```makefile
 19  # Wlasne flagi dla simavr: bez -Werror (nowe gcc) i bez libelf (ladujemy plik .hex)
 20  SIMAVR_CFLAGS = -O2 -g -fPIC -Wall -Wno-unused-parameter -Wno-unused-result \
 21                  -Wno-missing-field-initializers -Wno-sign-compare
```

- **Linia 19:** Komentarz: własne opcje dla simavr. Bez `-Werror`, by ostrzeżenia nowego kompilatora nie przerywały budowy, i bez `libelf`, bo ładujemy plik `.hex`.
- **Linia 20:** Opcje: optymalizacja `-O2`, informacje dla debugera `-g`, kod przenośny `-fPIC`, ostrzeżenia `-Wall`, a dalej wyłączenia niektórych ostrzeżeń.
- **Linia 21:** Ciąg dalszy wyłączeń ostrzeżeń.

```makefile
 23  CFLAGS = -O2 -g -std=gnu11 -Wall -Wextra -I$(SIMAVR)/simavr/sim -I$(SIMAVR)/simavr
 24  LDLIBS = -lm
 25
 26  FIRMWARE = ../firmware/build/firmware.hex
```

- **Linia 23:** Opcje dla naszego programu: `-std=gnu11` to wersja języka C, a `-I` wskazują katalogi z nagłówkami simavr.
- **Linia 24:** Dołącz bibliotekę matematyczną.
- **Linia 26:** Ścieżka do zbudowanego firmware.

### Reguły

```makefile
 28  all: build/avrsim firmware
 29
 30  $(SIMAVR):
 31  	git clone -q --depth 1 --branch $(SIMAVR_TAG) $(SIMAVR_URL) $@
 32  	@test "$$(git -C $@ rev-parse HEAD)" = "$(SIMAVR_COMMIT)" || \
 33  		{ echo "simavr: nieoczekiwany commit, przerywam"; rm -rf $@; exit 1; }
```

- **Linia 28:** Domyślny cel: zbuduj emulator i firmware.
- **Linia 30:** Reguła pobrania simavr.
- **Linia 31:** Pobierz tylko wybraną wersję, bez pełnej historii. `-q` oznacza brak zbędnych komunikatów.
- **Linia 32:** Sprawdź, czy pobrany commit jest tym oczekiwanym. `@` ukrywa polecenie. Podwójne `$$` przekazuje znak `$` do powłoki. `||` uruchamia dalszą część przy niepowodzeniu.
- **Linia 33:** Przy niezgodności: wypisz komunikat, usuń pobrany katalog i zakończ z błędem.

```makefile
 35  $(SIMAVR_PATCHED): | $(SIMAVR)
 36  	sed -i 's/if (irq->pool->avr->resetting)/if (irq->pool \&\& irq->pool->avr->resetting)/' \
 37  		$(SIMAVR)/simavr/sim/sim_irq.c
 38  	grep -q 'irq->pool && irq->pool->avr->resetting' $(SIMAVR)/simavr/sim/sim_irq.c
 39  	touch $@
```

- **Linia 35:** Reguła nakładająca poprawkę. Wymaga pobranego katalogu.
- **Linia 36:** `sed` zamienia w tekście warunek na bezpieczny, który najpierw sprawdza, czy `irq->pool` istnieje. Ukośnik na końcu oznacza ciąg dalszy.
- **Linia 37:** Plik, w którym dokonywana jest zamiana.
- **Linia 38:** `grep -q` sprawdza, czy zamiana się udała. Jeśli nie, budowa się przerwie.
- **Linia 39:** `touch` tworzy plik-znacznik.

```makefile
 41  $(SIMAVR_LIB): $(SIMAVR_PATCHED)
 42  	$(MAKE) -C $(SIMAVR)/simavr obj config CFLAGS="$(SIMAVR_CFLAGS)"
 43  	$(MAKE) -C $(SIMAVR)/simavr libsimavr CFLAGS="$(SIMAVR_CFLAGS)"
 44
 45  build/avrsim: avrsim.c $(SIMAVR_LIB) | build
 46  	$(CC) $(CFLAGS) $< -o $@ $(SIMAVR_LIB) $(LDLIBS)
```

- **Linia 41:** Reguła budująca bibliotekę simavr. Wymaga nałożonej poprawki.
- **Linia 42:** Uruchom `make` w katalogu simavr: przygotowanie, z naszymi opcjami.
- **Linia 43:** Uruchom `make` w katalogu simavr: budowa biblioteki.
- **Linia 45:** Reguła budująca nasz program. Wymaga pliku źródłowego, biblioteki i katalogu `build`.
- **Linia 46:** Skompiluj i połącz: `$<` to `avrsim.c`, a na końcu są biblioteki.

```makefile
 48  build:
 49  	mkdir -p build
 50
 51  firmware:
 52  	$(MAKE) -C ../firmware
```

- **Linia 48:** Reguła tworząca katalog `build`.
- **Linia 49:** Utwórz katalog.
- **Linia 51:** Cel budujący firmware.
- **Linia 52:** Uruchom `make` w katalogu `firmware`.

```makefile
 54  run: all
 55  	build/avrsim --link build/uart --eeprom build/eeprom.bin $(FIRMWARE)
 56
 57  test: all
 58  	cd ../server && .venv/bin/python -m pytest -q tests/test_firmware_avr.py
```

- **Linia 54:** Cel `run`, który wymaga zbudowania wszystkiego.
- **Linia 55:** Uruchom emulator: dowiązanie do pseudoterminala jako `build/uart`, plik na EEPROM i firmware.
- **Linia 57:** Cel `test`, który wymaga zbudowania wszystkiego.
- **Linia 58:** Wejdź do katalogu serwera i uruchom testy firmware. `&&` oznacza: drugie polecenie tylko po udanym pierwszym. `-q` skraca wynik.

```makefile
 60  clean:
 61  	rm -rf build
 62
 63  distclean: clean
 64  	rm -rf .deps
 65
 66  .PHONY: all firmware run test clean distclean
```

- **Linia 60:** Cel `clean`.
- **Linia 61:** Usuń katalog `build`.
- **Linia 63:** Cel `distclean`, który najpierw wykonuje `clean`.
- **Linia 64:** Usuń pobrany simavr.
- **Linia 66:** Te nazwy to czynności, a nie pliki.

---

[← Poprzedni](53-testy-firmware-avr.md) · [Spis treści](README.md) · [Następny →](61-sim-avrsim-c.md)
