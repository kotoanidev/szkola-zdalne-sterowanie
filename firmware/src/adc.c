// Pomiary ADC bez blokowania: kanały po kolei, suma 16 próbek na kanał
#include <avr/io.h>
#include "adc.h"

static uint16_t sums[ADC_CHANNELS];
static uint16_t acc;
static uint8_t channel;
static uint8_t samples;
static uint8_t skip;
static uint8_t ready;

static void start(void)
{
    ADCSRA |= _BV(ADSC);
}

void adc_init(void)
{
    // Odniesienie AVCC, preskaler 8: 1 MHz / 8 = 125 kHz (zakres 50–200 kHz)
    ADMUX = _BV(REFS0);
    ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);
    skip = 1;
    start();
}

void adc_poll(void)
{
    if (ADCSRA & _BV(ADSC))
        return;

    // Pierwszy pomiar po zmianie kanału odrzucamy (ładowanie kondensatora S/H)
    if (skip) {
        skip = 0;
    } else {
        acc += ADC;
        if (++samples == ADC_SAMPLES) {
            sums[channel] = acc;
            acc = 0;
            samples = 0;
            if (++channel == ADC_CHANNELS) {
                channel = 0;
                ready = 1;
            }
            ADMUX = _BV(REFS0) | channel;
            skip = 1;
        }
    }
    start();
}

uint8_t adc_ready(void)
{
    return ready;
}

uint16_t adc_sum(uint8_t ch)
{
    return sums[ch];
}

uint16_t adc_value(uint8_t ch)
{
    return (sums[ch] + ADC_SAMPLES / 2) / ADC_SAMPLES;
}
