// Takt systemowy 1 ms z Timer2 (CTC)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "timer.h"

static volatile uint32_t ms;

ISR(TIMER2_COMP_vect)
{
    ms++;
}

void timer_init(void)
{
    TCCR2 = _BV(WGM21) | _BV(CS21);
    OCR2 = (uint8_t)(F_CPU / 8 / 1000 - 1);
    TIMSK |= _BV(OCIE2);
}

uint32_t timer_ms(void)
{
    uint32_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = ms;
    }
    return t;
}

uint16_t timer_ms16(void)
{
    uint16_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = (uint16_t)ms;
    }
    return t;
}
