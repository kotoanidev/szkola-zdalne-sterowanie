// Przeliczanie pomiarów ADC: potencjometr, jasność w trybie AUTO
#include "config.h"
#include "pinout.h"
#include "adc.h"
#include "sensors.h"

uint8_t sensors_pot_percent(void)
{
    uint16_t v = adc_value(ADC_CH_POT) >> 2;

    return (uint8_t)((v * 100 + 127) / 255);
}

uint8_t sensors_auto_percent(void)
{
    uint16_t ldr = adc_value(ADC_CH_LDR);

#if LDR_INVERT
    ldr = 1023 - ldr;
#endif
    if (ldr <= LDR_DARK)
        return 100;
    if (ldr >= LDR_BRIGHT)
        return 0;
    return (uint8_t)(100 - (uint32_t)(ldr - LDR_DARK) * 100 / (LDR_BRIGHT - LDR_DARK));
}
