// Jasność diody głównej: Timer1, fast PWM 10-bit na OC1A, płynne przejście i korekcja gamma
#include <avr/io.h>
#include "config.h"
#include "pinout.h"
#include "pwm.h"

#define PWM_TOP 1023

static uint16_t level;
static uint16_t target;

static void apply(void)
{
    // Wypełnienie = (jasność)^2, żeby zmiana jasności była równomierna dla oka
    uint16_t duty = (uint16_t)(((uint32_t)level * level + PWM_TOP) >> 10);

    if (duty == 0) {
        // OCR1A = 0 daje w fast PWM krótkie impulsy, więc odłączamy wyjście
        TCCR1A &= ~_BV(COM1A1);
        LED_PORT &= ~_BV(LED_BIT);
    } else {
        OCR1A = duty;
        TCCR1A |= _BV(COM1A1);
    }
}

void pwm_init(void)
{
    LED_PORT &= ~_BV(LED_BIT);
    LED_DDR |= _BV(LED_BIT);
    // Tryb 7 (fast PWM 10-bit), bez preskalera: 1 MHz / 1024 ≈ 977 Hz
    TCCR1A = _BV(WGM11) | _BV(WGM10);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    apply();
}

void pwm_set_percent(uint8_t percent)
{
    if (percent > 100)
        percent = 100;
    target = (uint16_t)(((uint32_t)percent * PWM_TOP + 50) / 100);
}

void pwm_tick(void)
{
    if (level == target)
        return;
    if (level < target)
        level = (target - level > PWM_FADE_STEP) ? level + PWM_FADE_STEP : target;
    else
        level = (level - target > PWM_FADE_STEP) ? level - PWM_FADE_STEP : target;
    apply();
}
