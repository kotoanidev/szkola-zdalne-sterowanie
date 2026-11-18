# Firmware: Makefile, czyli przepis na budowę programu

[← Poprzedni](04-podstawy-narzedzi.md) · [Spis treści](README.md) · [Następny →](11-firmware-naglowki.md)

**Po co ten plik:** kompilacja wymaga wielu długich poleceń. `Makefile` zapisuje je raz, a potem wystarczy wpisać `make`. Składnia jest wytłumaczona w [podstawach narzędzi](04-podstawy-narzedzi.md).

## `firmware/Makefile`

[Otwórz ten plik w repozytorium](../../firmware/Makefile)

### Opis działania

```makefile
  1  # Firmware ATmega32A (plytka ATB 1.05, od etapu 2): kompilacja (avr-gcc) i wgrywanie przez USBasp.
  2  #
  3  #   make             kompilacja -> build/firmware.hex
  4  #   make flash       wgranie do ATmega32A przez USBasp
  5  #   make clean all F_CPU=11059200UL   gdy fusebity wybieraja kwarc 11,0592 MHz (docs/sprzet.md)
  6  #   make demo        program demonstracyjny etapu 1 dla ATmega8 -> build/demo.hex
  7  #   make flash-demo  wgranie programu demonstracyjnego do ATmega8
  8  #   make check       test polaczenia z programatorem (odczyt sygnatury)
  9  #   make read-fuses  odczyt fusebitow (TYLKO odczyt)
 10  #   make toolchain   pokazuje, ktory kompilator i avrdude sa uzywane
 11  #   make clean
 12  #
 13  # Celowo nie ma celu do ZAPISU fusebitow - patrz CLAUDE.md.
 14
```

- **Linia 1:** Komentarz (każda linia od `#` jest pomijana). Mówi, że plik dotyczy programu dla ATmega32A na płytce ATB 1.05 (od etapu 2): kompilacji kompilatorem `avr-gcc` i wgrywania programem `avrdude` przez programator USBasp.
- **Linia 2:** Pusty komentarz, odstęp dla czytelności.
- **Linia 3:** Instrukcja: samo `make` kompiluje i tworzy plik `build/firmware.hex`.
- **Linia 4:** Instrukcja: `make flash` wgrywa program do układu ATmega32A.
- **Linia 5:** Instrukcja: gdy fusebity wybierają kwarc 11,0592 MHz, budujemy od nowa z tą częstotliwością. `clean` jest potrzebne, bo zmiana `F_CPU` nie wymusza sama ponownej kompilacji.
- **Linia 6:** Instrukcja: `make demo` buduje mały program pokazowy z etapu 1 – nadal dla ATmega8.
- **Linia 7:** Instrukcja: `make flash-demo` wgrywa ten program pokazowy do ATmega8.
- **Linia 8:** Instrukcja: `make check` sprawdza połączenie z programatorem przez odczyt sygnatury, czyli numeru identyfikującego układ.
- **Linia 9:** Instrukcja: `make read-fuses` odczytuje fusebity, czyli ustawienia układu. Podkreślono, że to tylko odczyt.
- **Linia 10:** Instrukcja: `make toolchain` pokazuje, którego kompilatora i którego `avrdude` użyje Make.
- **Linia 11:** Instrukcja: `make clean` usuwa wyniki kompilacji.
- **Linia 12:** Pusty komentarz.
- **Linia 13:** Ważne ostrzeżenie: celowo nie ma polecenia do zapisu fusebitów, bo zła wartość mogłaby zablokować układ. Szczegóły są w `CLAUDE.md`.

### Podstawowe ustawienia

```makefile
 15  MCU     = atmega32
 16  F_CPU   = 1000000UL
 17  # Etap 1 (plytka stykowa) zostaje na ATmega8 z fabrycznym RC 1 MHz
 18  DEMO_MCU = atmega8
 19  TARGET  = firmware
 20  BUILD   = build
 21  SRC     = $(wildcard src/*.c)
 22  OBJ     = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
 23
```

- **Linia 15:** Zmienna `MCU` to typ układu: `atmega32` (ATmega32A ma ten sam zestaw rejestrów). Kompilator musi go znać, bo każdy układ ma inne rejestry.
- **Linia 16:** Zmienna `F_CPU` to częstotliwość zegara: 1 000 000 Hz, czyli 1 MHz. `UL` oznacza dużą liczbę bez znaku. Na tej wartości opierają się opóźnienia, takt 1 ms, zegar ADC i prędkość UART. Można ją nadpisać w poleceniu: `make F_CPU=11059200UL`.
- **Linia 17:** Komentarz: program etapu 1 zostaje na ATmega8 z fabrycznym zegarem 1 MHz.
- **Linia 18:** `DEMO_MCU` to typ układu dla programu pokazowego: `atmega8`.
- **Linia 19:** Zmienna `TARGET` to nazwa końcowego pliku, czyli `firmware`.
- **Linia 20:** Zmienna `BUILD` to katalog na wyniki kompilacji, czyli `build`.
- **Linia 21:** `SRC` to lista źródeł. Funkcja `wildcard` znajduje wszystkie pliki `.c` w katalogu `src`, dzięki czemu nowego pliku nie trzeba tu dopisywać.
- **Linia 22:** `OBJ` to lista plików pośrednich. `patsubst` zamienia każdą nazwę `src/coś.c` na `build/coś.o`.

### Wybór kompilatora

Ta część szuka kompilatora tak, by to samo polecenie `make` działało na różnych komputerach.

```makefile
 24  # Toolchain: najpierw avr-gcc/avrdude z Arduino IDE (nowsze), w drugiej kolejnosci z PATH (np. WinAVR).
 25  # Wymuszenie innego: make AVR_BIN=C:/sciezka/bin/ AVRDUDE=C:/sciezka/avrdude.exe
 26  ARDUINO_TOOLS := $(firstword $(wildcard \
 27  	$(subst \,/,$(LOCALAPPDATA))/Arduino15/packages/arduino/tools \
 28  	$(HOME)/.arduino15/packages/arduino/tools \
 29  	$(HOME)/Library/Arduino15/packages/arduino/tools))
 30  AVR_BIN ?= $(dir $(lastword $(wildcard $(ARDUINO_TOOLS)/avr-gcc/*/bin/avr-gcc*)))
 31  AVRDUDE ?= $(firstword $(wildcard $(ARDUINO_TOOLS)/avrdude/*/bin/avrdude*) avrdude)
 32
```

- **Linia 24:** Komentarz: najpierw szukamy narzędzi z Arduino IDE, bo są nowsze, a dopiero potem w `PATH`, czyli na systemowej liście katalogów z programami (tam jest np. WinAVR).
- **Linia 25:** Komentarz: można ręcznie wskazać inny kompilator, podając zmienne w poleceniu `make`.
- **Linia 26:** `ARDUINO_TOOLS` to ścieżka do narzędzi Arduino. `:=` liczy wartość raz. `wildcard` zostawia tylko istniejące ścieżki, a `firstword` bierze pierwszą z nich. Ukośnik `\` na końcu oznacza ciąg dalszy w następnej linii.
- **Linia 27:** Pierwsze miejsce poszukiwań, Windows. `subst` zamienia ukośniki `\` na `/`, bo tak woli Make. `LOCALAPPDATA` to systemowa zmienna z folderem danych użytkownika.
- **Linia 28:** Drugie miejsce, Linux: ukryty katalog `.arduino15` w katalogu domowym (`HOME`).
- **Linia 29:** Trzecie miejsce, macOS: katalog `Library/Arduino15`. Nawiasy na końcu zamykają `wildcard` i `firstword`.
- **Linia 30:** `AVR_BIN` to katalog z kompilatorem. `?=` nie nadpisuje wartości podanej ręcznie. Znajdujemy wszystkie wersje `avr-gcc`, `lastword` wybiera ostatnią, a `dir` odcina nazwę pliku i zostawia katalog. Gdy nic nie znajdzie, wartość jest pusta i użyty zostanie kompilator z `PATH`.
- **Linia 31:** `AVRDUDE` to program do wgrywania. `firstword` wybiera pierwszego znalezionego `avrdude` z Arduino, a jeśli go nie ma, zostaje samo słowo `avrdude`, czyli program z `PATH`.

### Nazwy narzędzi

```makefile
 33  CC      = $(AVR_BIN)avr-gcc
 34  OBJCOPY = $(AVR_BIN)avr-objcopy
 35  SIZE    = $(AVR_BIN)avr-size
 36
```

- **Linia 33:** `CC` to kompilator C. Do katalogu `AVR_BIN` doklejana jest nazwa programu `avr-gcc`.
- **Linia 34:** `OBJCOPY` to narzędzie, które przerobi gotowy program na plik `.hex`.
- **Linia 35:** `SIZE` to narzędzie, które pokazuje, ile pamięci zajmuje program.

### Opcje kompilatora

```makefile
 37  CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99 -Wall -Wextra \
 38            -ffunction-sections -fdata-sections -Iinclude -MMD -MP
 39  LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections
 40  DEMO_FLAGS = -mmcu=$(DEMO_MCU) -DF_CPU=1000000UL -Os -std=gnu99 -Wall -Wextra -MMD -MP
 41
```

- **Linia 37:** `CFLAGS` to opcje kompilacji: `-mmcu=` podaje typ układu, `-DF_CPU=` definiuje w kodzie stałą `F_CPU`, `-Os` każe optymalizować rozmiar programu, `-std=gnu99` wybiera wersję języka C, a `-Wall -Wextra` włącza ostrzeżenia o podejrzanym kodzie. Ukośnik na końcu oznacza ciąg dalszy.
- **Linia 38:** Dalsze opcje: `-ffunction-sections -fdata-sections` umieszczają każdą funkcję i zmienną w osobnej sekcji, by można było wyrzucić nieużywane. `-Iinclude` każe szukać nagłówków w katalogu `include`. `-MMD -MP` tworzą pliki `.d`, które zapisują, od jakich nagłówków zależy dany plik `.c`.
- **Linia 39:** `LDFLAGS` to opcje łączenia, czyli linkowania. `-Wl,--gc-sections` każe linkerowi usunąć nieużywane sekcje, co oszczędza pamięć flash.
- **Linia 40:** `DEMO_FLAGS` to osobne opcje dla programu etapu 1: układ ATmega8 i zawsze 1 MHz, niezależnie od `F_CPU` programu głównego.

### Ustawienia programatora

```makefile
 41
 42  # Zegar ISP USBasp: -B 8 (us) ~ 125 kHz; przy F_CPU = 1 MHz musi byc ponizej 250 kHz
 43  PROGRAMMER = usbasp
 44  ISP_B      = 8
 45  AVRDUDE_FLAGS = -c $(PROGRAMMER) -p m32 -B $(ISP_B)
 46  DEMO_AVRDUDE_FLAGS = -c $(PROGRAMMER) -p m8 -B $(ISP_B)
 47
```

- **Linia 42:** Komentarz: USBasp musi taktować układ wolno. Przy zegarze układu 1 MHz zegar programowania musi być poniżej 250 kHz.
- **Linia 43:** `PROGRAMMER` to typ programatora, czyli `usbasp`.
- **Linia 44:** `ISP_B` to okres zegara programowania w mikrosekundach. 8 µs daje około 125 kHz.
- **Linia 45:** `AVRDUDE_FLAGS` to wspólne opcje `avrdude`: `-c` typ programatora, `-p m32` typ układu (ATmega32/32A), `-B` szybkość.
- **Linia 46:** `DEMO_AVRDUDE_FLAGS` to te same opcje dla programu etapu 1, z typem układu `-p m8` (ATmega8).

### Reguły budowy

```makefile
 48  all: $(BUILD)/$(TARGET).hex size
 49
 50  $(BUILD)/%.o: src/%.c | $(BUILD)
 51  	$(CC) $(CFLAGS) -c $< -o $@
 52
 53  $(BUILD)/$(TARGET).elf: $(OBJ)
 54  	$(CC) $(LDFLAGS) $^ -o $@
 55
 56  $(BUILD)/demo.elf: demo/etap1.c | $(BUILD)
 57  	$(CC) $(DEMO_FLAGS) $< -o $@
 58
 59  $(BUILD)/%.hex: $(BUILD)/%.elf
 60  	$(OBJCOPY) -O ihex -R .eeprom $< $@
 61
 62  $(BUILD):
 63  	mkdir -p $(BUILD)
 64
```

- **Linia 48:** Pierwsza reguła, więc to ją wykona samo `make`. Cel `all` wymaga pliku `.hex` i wypisania zajętości pamięci (`size`).
- **Linia 50:** Reguła: jak zrobić plik `build/coś.o` z pliku `src/coś.c`. Znak `%` to „coś”. Po `|` jest katalog, który musi istnieć, ale jego data nie wymusza ponownej kompilacji.
- **Linia 51:** Polecenie kompilacji: `-c` oznacza „tylko skompiluj, nie łącz”, `$<` to plik `.c`, a `-o $@` zapisuje wynik pod nazwą celu.
- **Linia 53:** Reguła: program `firmware.elf` powstaje ze wszystkich plików `.o`.
- **Linia 54:** Polecenie łączenia: `$^` to wszystkie pliki `.o`, a `$@` to `firmware.elf`.
- **Linia 56:** Reguła: program pokazowy `demo.elf` powstaje z jednego pliku `demo/etap1.c`.
- **Linia 57:** Kompilacja i łączenie jednym poleceniem z opcjami `DEMO_FLAGS` dla ATmega8. Nagłówek `pinout.h` program pokazowy bierze z własnego katalogu `demo/`, więc ma piny z etapu 1.
- **Linia 59:** Reguła: każdy plik `.hex` powstaje z pliku `.elf` o tej samej nazwie.
- **Linia 60:** `avr-objcopy` tworzy plik w formacie Intel HEX (`-O ihex`) i pomija dane EEPROM (`-R .eeprom`).
- **Linia 62:** Reguła tworząca katalog `build`.
- **Linia 63:** `mkdir -p` tworzy katalog, a jeśli już istnieje, nie zgłasza błędu.

### Polecenia dla człowieka

```makefile
 65  size: $(BUILD)/$(TARGET).elf
 66  	$(SIZE) --mcu=$(MCU) -C $<
 67
 68  flash: $(BUILD)/$(TARGET).hex
 69  	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i
 70
 71  demo: $(BUILD)/demo.hex
 72  	$(SIZE) --mcu=$(DEMO_MCU) -C $(BUILD)/demo.elf
 73
 74  flash-demo: $(BUILD)/demo.hex
 75  	$(AVRDUDE) $(DEMO_AVRDUDE_FLAGS) -U flash:w:$<:i
 76
 77  check:
 78  	$(AVRDUDE) $(AVRDUDE_FLAGS) -n
 79
 80  read-fuses:
 81  	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:r:-:h -U hfuse:r:-:h
 82
 83  toolchain:
 84  	@echo CC      = $(CC)
 85  	@echo AVRDUDE = $(AVRDUDE)
 86  	$(CC) --version
 87
 88  clean:
 89  	rm -rf $(BUILD)
 90
```

- **Linia 65:** Cel `size` wymaga gotowego pliku `.elf`.
- **Linia 66:** Wypisuje, jaki procent pamięci układu zajmuje program.
- **Linia 68:** Cel `flash` wgrywa program, a wcześniej w razie potrzeby go buduje.
- **Linia 69:** Uruchamia `avrdude`. `-U flash:w:$<:i` oznacza: pamięć flash, zapis (`w`), plik `$<`, format Intel HEX (`i`).
- **Linia 71:** Cel `demo` buduje program pokazowy.
- **Linia 72:** Wypisuje jego rozmiar względem pamięci ATmega8.
- **Linia 74:** Cel `flash-demo` wgrywa program pokazowy.
- **Linia 75:** Podobne polecenie jak w linii 69, ale z opcjami dla ATmega8 i plikiem `demo.hex`.
- **Linia 77:** Cel `check` nie ma zależności.
- **Linia 78:** Uruchamia `avrdude` z opcją `-n`, czyli „niczego nie zapisuj”. Program łączy się z układem i odczytuje sygnaturę.
- **Linia 80:** Cel `read-fuses`.
- **Linia 81:** Odczytuje dwa bajty fusebitów: `lfuse` i `hfuse`. `r` to odczyt, `-` oznacza wypisanie na ekran, a `h` zapis szesnastkowy.
- **Linia 83:** Cel `toolchain`.
- **Linia 84:** `@echo` wypisuje, którego kompilatora używamy. `@` ukrywa samo polecenie.
- **Linia 85:** Wypisuje, którego `avrdude` używamy.
- **Linia 86:** Każe kompilatorowi podać swoją wersję.
- **Linia 88:** Cel `clean`.
- **Linia 89:** Usuwa cały katalog `build` razem z zawartością.

### Końcówka

```makefile
 91  -include $(OBJ:.o=.d) $(BUILD)/demo.d
 92
 93  .PHONY: all size flash demo flash-demo check read-fuses toolchain clean
```

- **Linia 91:** Dołącza pliki `.d` z zależnościami od nagłówków, dzięki czemu zmiana nagłówka powoduje ponowną kompilację właściwych plików. `-` oznacza, że przy pierwszej kompilacji brak tych plików nie jest błędem. `$(OBJ:.o=.d)` zamienia końcówki `.o` na `.d`.
- **Linia 93:** `.PHONY` mówi, że te nazwy to czynności, a nie pliki. Gdyby w katalogu istniał plik o nazwie `clean`, Make i tak wykona polecenie.

---

[← Poprzedni](04-podstawy-narzedzi.md) · [Spis treści](README.md) · [Następny →](11-firmware-naglowki.md)
