// Przeliczanie pomiarów ADC: temperatura, potencjometr, jasność w trybie AUTO
#include <avr/pgmspace.h>
#include "config.h"
#include "pinout.h"
#include "adc.h"
#include "sensors.h"

#if TEMP_SENSOR == TEMP_SENSOR_NTC
#define NTC_T_MIN  -20
#define NTC_T_STEP 5

// Wygenerowane: python tools/ntc_table.py (B = 3950, R25 = 10 kΩ, Rs = 10 kΩ)
static const uint16_t ntc_table[] PROGMEM = {
    1420, 1864, 2401, 3033, 3756, 4561, 5430, 6341,
    7270, 8192, 9083, 9926, 10707, 11419, 12058, 12624,
    13122, 13556, 13932, 14257, 14537, 14778, 14985, 15163,
    15316, 15447, 15561, 15658, 15743, 15816,
};
#define NTC_COUNT ((uint8_t)(sizeof(ntc_table) / sizeof(ntc_table[0])))
#endif

int16_t sensors_temp_tenths(void)
{
    uint16_t sum = adc_sum(ADC_CH_TEMP);

#if TEMP_SENSOR == TEMP_SENSOR_LM35
    // 10 mV/°C, czyli 1 mV = 0,1 °C; suma 16 próbek = 2^14 działek na VREF
    return (int16_t)(((uint32_t)sum * ADC_VREF_MV + 8192) >> 14);
#elif TEMP_SENSOR == TEMP_SENSOR_NTC
    uint16_t lo = pgm_read_word(&ntc_table[0]);
    uint8_t i;

    if (sum <= lo)
        return NTC_T_MIN * 10;
    for (i = 1; i < NTC_COUNT; i++) {
        uint16_t hi = pgm_read_word(&ntc_table[i]);
        if (sum < hi)
            return (int16_t)((NTC_T_MIN + NTC_T_STEP * (i - 1)) * 10
                             + (int16_t)((uint32_t)(sum - lo) * NTC_T_STEP * 10 / (hi - lo)));
        lo = hi;
    }
    return (NTC_T_MIN + NTC_T_STEP * (NTC_COUNT - 1)) * 10;
#else
#error "Nieznany TEMP_SENSOR (config.h)"
#endif
}

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
