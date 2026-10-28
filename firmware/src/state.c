// Stan urządzenia i jego zapis w EEPROM (z opóźnieniem, po jednym bajcie, bez blokowania)
#include <avr/eeprom.h>
#include "config.h"
#include "pwm.h"
#include "state.h"

#define EE_MAGIC 0x5A

typedef struct {
    uint8_t magic;
    uint8_t on;
    uint8_t pwm;
    uint8_t mode;
    uint8_t tlim;
    uint8_t check;
} saved_t;

state_t state;

static saved_t EEMEM ee_saved;
static saved_t saved;
static uint16_t save_delay;
static uint8_t save_pos;
static uint8_t saving;

static uint8_t checksum(const saved_t *s)
{
    return (uint8_t)(s->magic ^ s->on ^ s->pwm ^ s->mode ^ s->tlim ^ 0xA5);
}

static void apply(void)
{
    pwm_set_percent(state.on ? state.pwm : 0);
}

static void changed(void)
{
    saved.on = state.on;
    // Jasność z trybu AUTO nie jest zapisywana, po restarcie liczona jest od nowa
    if (state.mode == MODE_MANUAL)
        saved.pwm = state.pwm;
    saved.mode = state.mode;
    saved.tlim = state.tlim;
    saved.check = checksum(&saved);
    save_delay = SAVE_DELAY_MS;
    apply();
}

void state_init(void)
{
    eeprom_read_block(&saved, &ee_saved, sizeof(saved));
    if (saved.magic != EE_MAGIC || saved.check != checksum(&saved) || saved.on > 1
        || saved.pwm > 100 || saved.mode > MODE_AUTO || saved.tlim > TEMP_LIMIT_MAX) {
        saved.magic = EE_MAGIC;
        saved.on = 0;
        saved.pwm = 50;
        saved.mode = MODE_MANUAL;
        saved.tlim = TEMP_LIMIT_DEFAULT;
        saved.check = checksum(&saved);
    }
    state.on = saved.on;
    state.pwm = saved.pwm;
    state.mode = saved.mode;
    state.tlim = saved.tlim;
    state.alarm = 0;
    apply();
}

void state_tick(void)
{
    if (save_delay > 0 && --save_delay == 0) {
        save_pos = 0;
        saving = 1;
    }
    if (saving && eeprom_is_ready()) {
        uint8_t *ee = (uint8_t *)&ee_saved + save_pos;
        uint8_t value = ((const uint8_t *)&saved)[save_pos];

        if (eeprom_read_byte(ee) != value)
            eeprom_write_byte(ee, value);
        if (++save_pos == (uint8_t)sizeof(saved))
            saving = 0;
    }
}

void state_set_on(uint8_t on)
{
    state.on = on ? 1 : 0;
    changed();
}

uint8_t state_set_pwm(uint8_t pwm)
{
    uint8_t mode_changed = state.mode != MODE_MANUAL;

    state.mode = MODE_MANUAL;
    state.pwm = pwm;
    changed();
    return mode_changed;
}

void state_set_mode(uint8_t mode)
{
    state.mode = mode;
    changed();
}

void state_set_tlim(uint8_t tlim)
{
    state.tlim = tlim;
    changed();
}

void state_auto_pwm(uint8_t pwm)
{
    state.pwm = pwm;
    apply();
}
