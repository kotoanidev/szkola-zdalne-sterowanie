# Symulacja firmware w simavr - tylko Linux (komputer domowy), nie jest potrzebna w szkole.
#
#   make           pobiera i buduje simavr (raz), buduje build/avrsim i firmware
#   make run       firmware w symulatorze; UART na pseudoterminalu build/uart
#   make test      testy prawdziwego firmware (pytest z venv serwera)
#   make clean     usuwa build/
#   make distclean usuwa tez pobrany simavr (.deps/)

SIMAVR_URL    = https://github.com/buserror/simavr
SIMAVR_TAG    = v1.8
SIMAVR_COMMIT = f44723e8c42431136d5b4de81f789ded56d7e8fa
SIMAVR        = .deps/simavr
SIMAVR_LIB    = $(SIMAVR)/simavr/obj-$(shell $(CC) -dumpmachine)/libsimavr.a

# Poprawka bledu simavr 1.8: avr_reset() odwoluje sie do irq->pool, ktorego IRQ watchdoga
# nie ma (segfault przy resecie, takze od watchdoga)
SIMAVR_PATCHED = $(SIMAVR)/.patched-irq-pool

# Wlasne flagi dla simavr: bez -Werror (nowe gcc) i bez libelf (ladujemy plik .hex)
SIMAVR_CFLAGS = -O2 -g -fPIC -Wall -Wno-unused-parameter -Wno-unused-result \
                -Wno-missing-field-initializers -Wno-sign-compare

CFLAGS = -O2 -g -std=gnu11 -Wall -Wextra -I$(SIMAVR)/simavr/sim -I$(SIMAVR)/simavr
LDLIBS = -lm

FIRMWARE = ../firmware/build/firmware.hex

all: build/avrsim firmware

$(SIMAVR):
	git clone -q --depth 1 --branch $(SIMAVR_TAG) $(SIMAVR_URL) $@
	@test "$$(git -C $@ rev-parse HEAD)" = "$(SIMAVR_COMMIT)" || \
		{ echo "simavr: nieoczekiwany commit, przerywam"; rm -rf $@; exit 1; }

$(SIMAVR_PATCHED): | $(SIMAVR)
	sed -i 's/if (irq->pool->avr->resetting)/if (irq->pool \&\& irq->pool->avr->resetting)/' \
		$(SIMAVR)/simavr/sim/sim_irq.c
	grep -q 'irq->pool && irq->pool->avr->resetting' $(SIMAVR)/simavr/sim/sim_irq.c
	touch $@

$(SIMAVR_LIB): $(SIMAVR_PATCHED)
	$(MAKE) -C $(SIMAVR)/simavr obj config CFLAGS="$(SIMAVR_CFLAGS)"
	$(MAKE) -C $(SIMAVR)/simavr libsimavr CFLAGS="$(SIMAVR_CFLAGS)"

build/avrsim: avrsim.c $(SIMAVR_LIB) | build
	$(CC) $(CFLAGS) $< -o $@ $(SIMAVR_LIB) $(LDLIBS)

build:
	mkdir -p build

firmware:
	$(MAKE) -C ../firmware

run: all
	build/avrsim --link build/uart --eeprom build/eeprom.bin $(FIRMWARE)

test: all
	cd ../server && .venv/bin/python -m pytest -q tests/test_firmware_avr.py

clean:
	rm -rf build

distclean: clean
	rm -rf .deps

.PHONY: all firmware run test clean distclean
