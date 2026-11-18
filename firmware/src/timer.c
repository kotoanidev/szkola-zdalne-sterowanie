// Takt systemowy 1 ms z Timer2 (CTC)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "timer.h"

// OCR2 musi się zmieścić w 8 bitach: preskaler 8 przy 1 MHz, 64 przy kwarcu 11,0592 MHz
#if F_CPU / 8 / 1000 <= 256
#define T2_DIV 8
#define T2_CS  _BV(CS21)
#elif F_CPU / 64 / 1000 <= 256
#define T2_DIV 64
#define T2_CS  _BV(CS22)
#else
#error "Nieobsługiwane F_CPU (timer.c)"
#endif

static volatile uint32_t ms;

ISR(TIMER2_COMP_vect)
{
    ms++;
}

void timer_init(void)
{
    TCCR2 = _BV(WGM21) | T2_CS;
    OCR2 = (uint8_t)((F_CPU / T2_DIV + 500) / 1000 - 1);
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
