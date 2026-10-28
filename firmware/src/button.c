// Przycisk z programowym debouncingiem (integrator próbkowany co 1 ms)
#include <avr/io.h>
#include "config.h"
#include "pinout.h"
#include "button.h"

static uint8_t integrator;
static uint8_t pressed;
static uint8_t long_sent;
static uint16_t held_ms;

void button_init(void)
{
    BTN_DDR &= ~_BV(BTN_BIT);
    BTN_PORT |= _BV(BTN_BIT);
}

uint8_t button_tick(void)
{
    if (!(BTN_PIN & _BV(BTN_BIT))) {
        if (integrator < BTN_DEBOUNCE_MS)
            integrator++;
    } else if (integrator > 0) {
        integrator--;
    }

    if (!pressed && integrator == BTN_DEBOUNCE_MS) {
        pressed = 1;
        long_sent = 0;
        held_ms = 0;
    } else if (pressed && integrator == 0) {
        pressed = 0;
        // Krótkie naciśnięcie zgłaszamy przy puszczeniu, żeby odróżnić je od długiego
        if (!long_sent)
            return BTN_SHORT;
    }

    if (pressed && !long_sent && ++held_ms >= BTN_LONG_MS) {
        long_sent = 1;
        return BTN_LONG;
    }
    return BTN_NONE;
}
