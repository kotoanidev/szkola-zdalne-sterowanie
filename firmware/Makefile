# Firmware ATmega8: kompilacja (avr-gcc) i wgrywanie przez USBasp (avrdude).
#
#   make             kompilacja -> build/firmware.hex
#   make flash       wgranie do ATmega8 przez USBasp
#   make demo        program demonstracyjny etapu 1 -> build/demo.hex
#   make flash-demo  wgranie programu demonstracyjnego
#   make check       test polaczenia z programatorem (odczyt sygnatury)
#   make read-fuses  odczyt fusebitow (TYLKO odczyt)
#   make toolchain   pokazuje, ktory kompilator i avrdude sa uzywane
#   make clean
#
# Celowo nie ma celu do ZAPISU fusebitow - patrz CLAUDE.md.

MCU     = atmega8
F_CPU   = 1000000UL
TARGET  = firmware
BUILD   = build
SRC     = $(wildcard src/*.c)
OBJ     = $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

# Toolchain: najpierw avr-gcc/avrdude z Arduino IDE (nowsze), w drugiej kolejnosci z PATH (np. WinAVR).
# Wymuszenie innego: make AVR_BIN=C:/sciezka/bin/ AVRDUDE=C:/sciezka/avrdude.exe
ARDUINO_TOOLS := $(firstword $(wildcard \
	$(subst \,/,$(LOCALAPPDATA))/Arduino15/packages/arduino/tools \
	$(HOME)/.arduino15/packages/arduino/tools \
	$(HOME)/Library/Arduino15/packages/arduino/tools))
AVR_BIN ?= $(dir $(lastword $(wildcard $(ARDUINO_TOOLS)/avr-gcc/*/bin/avr-gcc*)))
AVRDUDE ?= $(firstword $(wildcard $(ARDUINO_TOOLS)/avrdude/*/bin/avrdude*) avrdude)

CC      = $(AVR_BIN)avr-gcc
OBJCOPY = $(AVR_BIN)avr-objcopy
SIZE    = $(AVR_BIN)avr-size

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99 -Wall -Wextra \
          -ffunction-sections -fdata-sections -Iinclude -MMD -MP
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections

# Zegar ISP USBasp: -B 8 (us) ~ 125 kHz; przy F_CPU = 1 MHz musi byc ponizej 250 kHz
PROGRAMMER = usbasp
ISP_B      = 8
AVRDUDE_FLAGS = -c $(PROGRAMMER) -p m8 -B $(ISP_B)

all: $(BUILD)/$(TARGET).hex size

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/demo.elf: demo/etap1.c | $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD):
	mkdir -p $(BUILD)

size: $(BUILD)/$(TARGET).elf
	$(SIZE) --mcu=$(MCU) -C $<

flash: $(BUILD)/$(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i

demo: $(BUILD)/demo.hex
	$(SIZE) --mcu=$(MCU) -C $(BUILD)/demo.elf

flash-demo: $(BUILD)/demo.hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$<:i

check:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -n

read-fuses:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:r:-:h -U hfuse:r:-:h

toolchain:
	@echo CC      = $(CC)
	@echo AVRDUDE = $(AVRDUDE)
	$(CC) --version

clean:
	rm -rf $(BUILD)

-include $(OBJ:.o=.d) $(BUILD)/demo.d

.PHONY: all size flash demo flash-demo check read-fuses toolchain clean
