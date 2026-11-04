# Firmware: Makefile, czyli przepis na budowę programu

[← Poprzedni](04-podstawy-narzedzi.md) · [Spis treści](README.md) · [Następny →](11-firmware-naglowki.md)

**Po co ten plik:** kompilacja wymaga wielu długich poleceń. `Makefile` zapisuje je raz, a potem wystarczy wpisać `make`. Składnia jest wytłumaczona w [podstawach narzędzi](04-podstawy-narzedzi.md).

## `firmware/Makefile`

[Otwórz ten plik w repozytorium](../../firmware/Makefile)

### Opis działania

```makefile
  1  # Firmware ATmega8: kompilacja (avr-gcc) i wgrywanie przez USBasp (avrdude).
  2  #
  3  #   make             kompilacja -> build/firmware.hex
  4  #   make flash       wgranie do ATmega8 przez USBasp
  5  #   make demo        program demonstracyjny etapu 1 -> build/demo.hex
  6  #   make flash-demo  wgranie programu demonstracyjnego
  7  #   make check       test polaczenia z programatorem (odczyt sygnatury)
  8  #   make read-fuses  odczyt fusebitow (TYLKO odczyt)
  9  #   make toolchain   pokazuje, ktory kompilator i avrdude sa uzywane
 10  #   make clean
 11  #
 12  # Celowo nie ma celu do ZAPISU fusebitow - patrz CLAUDE.md.
 13
```

- **Linia 1:** Komentarz (każda linia od `#` jest pomijana). Mówi, że plik dotyczy kompilacji programu kompilatorem `avr-gcc` i wgrywania go programem `avrdude` przez programator USBasp.
- **Linia 2:** Pusty komentarz, odstęp dla czytelności.
- **Linia 3:** Instrukcja: samo `make` kompiluje i tworzy plik `build/firmware.hex`.
- **Linia 4:** Instrukcja: `make flash` wgrywa program do układu ATmega8.
- **Linia 5:** Instrukcja: `make demo` buduje mały program pokazowy z etapu 1.
- **Linia 6:** Instrukcja: `make flash-demo` wgrywa ten program pokazowy.
- **Linia 7:** Instrukcja: `make check` sprawdza połączenie z programatorem przez odczyt sygnatury, czyli numeru identyfikującego układ.
- **Linia 8:** Instrukcja: `make read-fuses` odczytuje fusebity, czyli ustawienia układu. Podkreślono, że to tylko odczyt.
- **Linia 9:** Instrukcja: `make toolchain` pokazuje, którego kompilatora i którego `avrdude` użyje Make.
- **Linia 10:** Instrukcja: `make clean` usuwa wyniki kompilacji.
- **Linia 11:** Pusty komentarz.
- **Linia 12:** Ważne ostrzeżenie: celowo nie ma polecenia do zapisu fusebitów, bo zła wartość mogłaby zablokować układ. Szczegóły są w `CLAUDE.md`.

### Podstawowe ustawienia

```makefile
 14  MCU     = atmega8
 15  F_CPU   = 1000000UL
 16  TARGET  = firmware
 17  BUILD   = build
 18  SRC     = $(wildcard src/*.c)
 19  OBJ     = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
 20
```

- **Linia 14:** Zmienna `MCU` to typ układu. Kompilator musi go znać, bo każdy układ ma inne rejestry.
- **Linia 15:** Zmienna `F_CPU` to częstotliwość zegara: 1 000 000 Hz, czyli 1 MHz. `UL` oznacza dużą liczbę bez znaku. Na tej wartości opierają się opóźnienia i prędkość UART.
- **Linia 16:** Zmienna `TARGET` to nazwa końcowego pliku, czyli `firmware`.
- **Linia 17:** Zmienna `BUILD` to katalog na wyniki kompilacji, czyli `build`.
- **Linia 18:** `SRC` to lista źródeł. Funkcja `wildcard` znajduje wszystkie pliki `.c` w katalogu `src`, dzięki czemu nowego pliku nie trzeba tu dopisywać.
- **Linia 19:** `OBJ` to lista plików pośrednich. `patsubst` zamienia każdą nazwę `src/coś.c` na `build/coś.o`.

### Wybór kompilatora

Ta część szuka kompilatora tak, by to samo polecenie `make` działało na różnych komputerach.

```makefile
 21  # Toolchain: najpierw avr-gcc/avrdude z Arduino IDE (nowsze), w drugiej kolejnosci z PATH (np. WinAVR).
 22  # Wymuszenie innego: make AVR_BIN=C:/sciezka/bin/ AVRDUDE=C:/sciezka/avrdude.exe
 23  ARDUINO_TOOLS := $(firstword $(wildcard \
 24  	$(subst \,/,$(LOCALAPPDATA))/Arduino15/packages/arduino/tools \
 25  	$(HOME)/.arduino15/packages/arduino/tools \
 26  	$(HOME)/Library/Arduino15/packages/arduino/tools))
 27  AVR_BIN ?= $(dir $(lastword $(wildcard $(ARDUINO_TOOLS)/avr-gcc/*/bin/avr-gcc*)))
 28  AVRDUDE ?= $(firstword $(wildcard $(ARDUINO_TOOLS)/avrdude/*/bin/avrdude*) avrdude)
 29
```

- **Linia 21:** Komentarz: najpierw szukamy narzędzi z Arduino IDE, bo są nowsze, a dopiero potem w `PATH`, czyli na systemowej liście katalogów z programami (tam jest np. WinAVR).
- **Linia 22:** Komentarz: można ręcznie wskazać inny kompilator, podając zmienne w poleceniu `make`.
- **Linia 23:** `ARDUINO_TOOLS` to ścieżka do narzędzi Arduino. `:=` liczy wartość raz. `wildcard` zostawia tylko istniejące ścieżki, a `firstword` bierze pierwszą z nich. Ukośnik `\` na końcu oznacza ciąg dalszy w następnej linii.
- **Linia 24:** Pierwsze miejsce poszukiwań, Windows. `subst` zamienia ukośniki `\` na `/`, bo tak woli Make. `LOCALAPPDATA` to systemowa zmienna z folderem danych użytkownika.
- **Linia 25:** Drugie miejsce, Linux: ukryty katalog `.arduino15` w katalogu domowym (`HOME`).
- **Linia 26:** Trzecie miejsce, macOS: katalog `Library/Arduino15`. Nawiasy na końcu zamykają `wildcard` i `firstword`.
- **Linia 27:** `AVR_BIN` to katalog z kompilatorem. `?=` nie nadpisuje wartości podanej ręcznie. Znajdujemy wszystkie wersje `avr-gcc`, `lastword` wybiera ostatnią, a `dir` odcina nazwę pliku i zostawia katalog. Gdy nic nie znajdzie, wartość jest pusta i użyty zostanie kompilator z `PATH`.
- **Linia 28:** `AVRDUDE` to program do wgrywania. `firstword` wybiera pierwszego znalezionego `avrdude` z Arduino, a jeśli go nie ma, zostaje samo słowo `avrdude`, czyli program z `PATH`.

### Nazwy narzędzi

```makefile
 30  CC      = $(AVR_BIN)avr-gcc
 31  OBJCOPY = $(AVR_BIN)avr-objcopy
 32  SIZE    = $(AVR_BIN)avr-size
 33
```

- **Linia 30:** `CC` to kompilator C. Do katalogu `AVR_BIN` doklejana jest nazwa programu `avr-gcc`.
- **Linia 31:** `OBJCOPY` to narzędzie, które przerobi gotowy program na plik `.hex`.
- **Linia 32:** `SIZE` to narzędzie, które pokazuje, ile pamięci zajmuje program.

### Opcje kompilatora

```makefile
 34  CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99 -Wall -Wextra \
 35            -ffunction-sections -fdata-sections -Iinclude -MMD -MP
 36  LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections
 37
```

- **Linia 34:** `CFLAGS` to opcje kompilacji: `-mmcu=` podaje typ układu, `-DF_CPU=` definiuje w kodzie stałą `F_CPU`, `-Os` każe optymalizować rozmiar programu, `-std=gnu99` wybiera wersję języka C, a `-Wall -Wextra` włącza ostrzeżenia o podejrzanym kodzie. Ukośnik na końcu oznacza ciąg dalszy.
- **Linia 35:** Dalsze opcje: `-ffunction-sections -fdata-sections` umieszczają każdą funkcję i zmienną w osobnej sekcji, by można było wyrzucić nieużywane. `-Iinclude` każe szukać nagłówków w katalogu `include`. `-MMD -MP` tworzą pliki `.d`, które zapisują, od jakich nagłówków zależy dany plik `.c`.
- **Linia 36:** `LDFLAGS` to opcje łączenia, czyli linkowania. `-Wl,--gc-sections` każe linkerowi usunąć nieużywane sekcje, co oszczędza pamięć flash.

### Ustawienia programatora

```makefile
 38  # Zegar ISP USBasp: -B 8 (us) ~ 125 kHz; przy F_CPU = 1 MHz musi byc ponizej 250 kHz
 39  PROGRAMMER = usbasp
 40  ISP_B      = 8
 41  AVRDUDE_FLAGS = -c $(PROGRAMMER) -p m8 -B $(ISP_B)
 42
```

- **Linia 38:** Komentarz: USBasp musi taktować układ wolno. Przy zegarze układu 1 MHz zegar programowania musi być poniżej 250 kHz.
- **Linia 39:** `PROGRAMMER` to typ programatora, czyli `usbasp`.
- **Linia 40:** `ISP_B` to okres zegara programowania w mikrosekundach. 8 µs daje około 125 kHz.
- **Linia 41:** `AVRDUDE_FLAGS` to wspólne opcje `avrdude`: `-c` typ programatora, `-p m8` typ układu (ATmega8), `-B` szybkość.

### Reguły budowy

```makefile
 43  all: $(BUILD)/$(TARGET).hex size
 44
 45  $(BUILD)/%.o: src/%.c | $(BUILD)
 46  	$(CC) $(CFLAGS) -c $< -o $@
 47
 48  $(BUILD)/$(TARGET).elf: $(OBJ)
 49  	$(CC) $(LDFLAGS) $^ -o $@
 50
 51  $(BUILD)/demo.elf: demo/etap1.c | $(BUILD)
 52  	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@
 53
 54  $(BUILD)/%.hex: $(BUILD)/%.elf
 55  	$(OBJCOPY) -O ihex -R .eeprom $< $@
 56
 57  $(BUILD):
 58  	mkdir -p $(BUILD)
 59
```

- **Linia 43:** Pierwsza reguła, więc to ją wykona samo `make`. Cel `all` wymaga pliku `.hex` i wypisania zajętości pamięci (`size`).
- **Linia 45:** Reguła: jak zrobić plik `build/coś.o` z pliku `src/coś.c`. Znak `%` to „coś”. Po `|` jest katalog, który musi istnieć, ale jego data nie wymusza ponownej kompilacji.
- **Linia 46:** Polecenie kompilacji: `-c` oznacza „tylko skompiluj, nie łącz”, `$<` to plik `.c`, a `-o $@` zapisuje wynik pod nazwą celu.
- **Linia 48:** Reguła: program `firmware.elf` powstaje ze wszystkich plików `.o`.
- **Linia 49:** Polecenie łączenia: `$^` to wszystkie pliki `.o`, a `$@` to `firmware.elf`.
- **Linia 51:** Reguła: program pokazowy `demo.elf` powstaje z jednego pliku `demo/etap1.c`.
- **Linia 52:** Kompilacja i łączenie jednym poleceniem, dlatego są tu obie grupy opcji.
- **Linia 54:** Reguła: każdy plik `.hex` powstaje z pliku `.elf` o tej samej nazwie.
- **Linia 55:** `avr-objcopy` tworzy plik w formacie Intel HEX (`-O ihex`) i pomija dane EEPROM (`-R .eeprom`).
- **Linia 57:** Reguła tworząca katalog `build`.
- **Linia 58:** `mkdir -p` tworzy katalog, a jeśli już istnieje, nie zgłasza błędu.

### Polecenia dla człowieka

```makefile
 60  size: $(BUILD)/$(TARGET).elf
 61  	$(SIZE) --mcu=$(MCU) -C $<
 62
 63  flash: $(BUILD)/$(TARGET).hex
 64  	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i
 65
 66  demo: $(BUILD)/demo.hex
 67  	$(SIZE) --mcu=$(MCU) -C $(BUILD)/demo.elf
 68
 69  flash-demo: $(BUILD)/demo.hex
 70  	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i
 71
 72  check:
 73  	$(AVRDUDE) $(AVRDUDE_FLAGS) -n
 74
 75  read-fuses:
 76  	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:r:-:h -U hfuse:r:-:h
 77
 78  toolchain:
 79  	@echo CC      = $(CC)
 80  	@echo AVRDUDE = $(AVRDUDE)
 81  	$(CC) --version
 82
 83  clean:
 84  	rm -rf $(BUILD)
 85
```

- **Linia 60:** Cel `size` wymaga gotowego pliku `.elf`.
- **Linia 61:** Wypisuje, jaki procent pamięci układu zajmuje program.
- **Linia 63:** Cel `flash` wgrywa program, a wcześniej w razie potrzeby go buduje.
- **Linia 64:** Uruchamia `avrdude`. `-U flash:w:$<:i` oznacza: pamięć flash, zapis (`w`), plik `$<`, format Intel HEX (`i`).
- **Linia 66:** Cel `demo` buduje program pokazowy.
- **Linia 67:** Wypisuje jego rozmiar.
- **Linia 69:** Cel `flash-demo` wgrywa program pokazowy.
- **Linia 70:** To samo polecenie co w linii 64, tylko plikiem jest `demo.hex`.
- **Linia 72:** Cel `check` nie ma zależności.
- **Linia 73:** Uruchamia `avrdude` z opcją `-n`, czyli „niczego nie zapisuj”. Program łączy się z układem i odczytuje sygnaturę.
- **Linia 75:** Cel `read-fuses`.
- **Linia 76:** Odczytuje dwa bajty fusebitów: `lfuse` i `hfuse`. `r` to odczyt, `-` oznacza wypisanie na ekran, a `h` zapis szesnastkowy.
- **Linia 78:** Cel `toolchain`.
- **Linia 79:** `@echo` wypisuje, którego kompilatora używamy. `@` ukrywa samo polecenie.
- **Linia 80:** Wypisuje, którego `avrdude` używamy.
- **Linia 81:** Każe kompilatorowi podać swoją wersję.
- **Linia 83:** Cel `clean`.
- **Linia 84:** Usuwa cały katalog `build` razem z zawartością.

### Końcówka

```makefile
 86  -include $(OBJ:.o=.d) $(BUILD)/demo.d
 87
 88  .PHONY: all size flash demo flash-demo check read-fuses toolchain clean
```

- **Linia 86:** Dołącza pliki `.d` z zależnościami od nagłówków, dzięki czemu zmiana nagłówka powoduje ponowną kompilację właściwych plików. `-` oznacza, że przy pierwszej kompilacji brak tych plików nie jest błędem. `$(OBJ:.o=.d)` zamienia końcówki `.o` na `.d`.
- **Linia 88:** `.PHONY` mówi, że te nazwy to czynności, a nie pliki. Gdyby w katalogu istniał plik o nazwie `clean`, Make i tak wykona polecenie.

---

[← Poprzedni](04-podstawy-narzedzi.md) · [Spis treści](README.md) · [Następny →](11-firmware-naglowki.md)
