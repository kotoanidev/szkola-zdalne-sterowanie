#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_CHANNELS 3
#define ADC_SAMPLES  16

void adc_init(void);
void adc_poll(void);
uint8_t adc_ready(void);
uint16_t adc_sum(uint8_t channel);
uint16_t adc_value(uint8_t channel);

#endif
