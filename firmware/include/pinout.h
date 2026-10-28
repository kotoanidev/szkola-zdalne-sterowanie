// Przypisanie pinów ATmega8 – musi być zgodne z docs/sprzet.md (zmiana = oba pliki).
#ifndef PINOUT_H
#define PINOUT_H

#include <avr/io.h>

// Dioda główna: PB1 / OC1A (pin 15), PWM z Timer1
#define LED_DDR      DDRB
#define LED_PORT     PORTB
#define LED_BIT      PB1

// Dioda statusu: PD7 (pin 13)
#define STATUS_DDR   DDRD
#define STATUS_PORT  PORTD
#define STATUS_BIT   PD7

// Przycisk do GND z wewnętrznym pull-upem: PD2 (pin 4)
#define BTN_DDR      DDRD
#define BTN_PORT     PORTD
#define BTN_PIN      PIND
#define BTN_BIT      PD2

// Kanały ADC (PC0–PC2, piny 23–25)
#define ADC_CH_POT   0
#define ADC_CH_LDR   1
#define ADC_CH_TEMP  2

#endif
