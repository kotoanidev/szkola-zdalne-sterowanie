// Sterownik oświetlenia LED: ATmega8 @ 1 MHz, sterowanie przez UART, przycisk i potencjometr
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "config.h"
#include "pinout.h"
#include "adc.h"
#include "button.h"
#include "protocol.h"
#include "pwm.h"
#include "sensors.h"
#include "state.h"
#include "timer.h"
#include "uart.h"

#define POT_PERIOD_MS   50
#define AUTO_PERIOD_MS  200
#define TEMP_PERIOD_MS  500
#define ACTIVITY_MS     50

static uint8_t pot_timer;
static uint8_t auto_timer;
static uint16_t temp_timer;
static uint8_t activity_ms;
#if POT_ENABLE
static uint8_t pot_last = 0xFF;
#endif

static PGM_P reset_reason(uint8_t flags)
{
    if (flags & _BV(WDRF))
        return PSTR("WDT");
    if (flags & _BV(BORF))
        return PSTR("BOR");
    if (flags & _BV(EXTRF))
        return PSTR("EXT");
    return PSTR("POR");
}

static uint8_t diff(uint8_t a, uint8_t b)
{
    return a > b ? a - b : b - a;
}

// Status: krótki błysk co ok. 1 s, miganie 4 Hz przy alarmie, błysk przy komendzie
static void status_tick(uint16_t now)
{
    uint8_t lit;

    if (state.alarm)
        lit = (now & 0x80) != 0;
    else
        lit = (now & 0x3FF) < 30;
    if (activity_ms > 0) {
        activity_ms--;
        lit = 1;
    }
    if (lit)
        STATUS_PORT |= _BV(STATUS_BIT);
    else
        STATUS_PORT &= ~_BV(STATUS_BIT);
}

static void pot_check(void)
{
#if POT_ENABLE
    uint8_t p = sensors_pot_percent();

    // Pierwszy odczyt to punkt odniesienia – po restarcie liczy się stan z EEPROM
    if (pot_last == 0xFF) {
        pot_last = p;
        return;
    }
    if (p == pot_last)
        return;
    if (diff(p, pot_last) < POT_HYST && p != 0 && p != 100)
        return;
    pot_last = p;
    protocol_event_pot(state_set_pwm(p));
#endif
}

static void auto_check(void)
{
    uint8_t p;

    if (state.mode != MODE_AUTO)
        return;
    p = sensors_auto_percent();
    if (p == state.pwm)
        return;
    if (diff(p, state.pwm) >= AUTO_HYST || p == 0 || p == 100)
        state_auto_pwm(p);
}

static void temp_check(void)
{
    int16_t t = sensors_temp_tenths();
    int16_t limit = (int16_t)state.tlim * 10;

    if (!state.alarm && t >= limit) {
        state.alarm = 1;
        protocol_event_temp(t);
    } else if (state.alarm && t < limit - TEMP_HYST_TENTHS) {
        state.alarm = 0;
        protocol_event_temp(t);
    }
}

static void tick(uint16_t now)
{
    uint8_t b = button_tick();

    if (b == BTN_SHORT) {
        state_set_on(!state.on);
        protocol_event_btn_on();
    } else if (b == BTN_LONG) {
        state_set_mode(state.mode == MODE_AUTO ? MODE_MANUAL : MODE_AUTO);
        protocol_event_btn_mode();
    }
    pwm_tick();
    state_tick();
    status_tick(now);

    if (!adc_ready())
        return;
    if (++pot_timer >= POT_PERIOD_MS) {
        pot_timer = 0;
        pot_check();
    }
    if (++auto_timer >= AUTO_PERIOD_MS) {
        auto_timer = 0;
        auto_check();
    }
    if (++temp_timer >= TEMP_PERIOD_MS) {
        temp_timer = 0;
        temp_check();
    }
}

int main(void)
{
    uint8_t reset_flags = MCUCSR;
    uint16_t last;

    MCUCSR = 0;
    wdt_enable(WDTO_500MS);
#ifdef OSCCAL_VALUE
    OSCCAL = OSCCAL_VALUE;
#endif

    STATUS_DDR |= _BV(STATUS_BIT);
    button_init();
    pwm_init();
    adc_init();
    timer_init();
    uart_init();
    state_init();
    sei();

    protocol_event_boot(reset_reason(reset_flags));
    last = timer_ms16();

    for (;;) {
        char c;
        uint16_t now;

        wdt_reset();
        while (uart_read(&c)) {
            if (protocol_rx(c))
                activity_ms = ACTIVITY_MS;
        }
        adc_poll();
        now = timer_ms16();
        while (last != now)
            tick(++last);
    }
}
