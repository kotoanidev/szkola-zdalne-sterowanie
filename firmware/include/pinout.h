// Przypisanie pinów ATmega32A na płytce ATB 1.05 – musi być zgodne z docs/sprzet.md (zmiana = oba pliki).
#ifndef PINOUT_H
#define PINOUT_H

#include <avr/io.h>

// Diody LED na ATB świecą stanem niskim (anoda do VCC) – do potwierdzenia testem z docs/sprzet.md
#define LED_ACTIVE_LOW 1

// Dioda główna: PD5 / OC1A, PWM z Timer1 (przewód do LED8, zworka J25 PC7 zdjęta)
#define LED_DDR      DDRD
#define LED_PORT     PORTD
#define LED_BIT      PD5

// Dioda statusu: PC0 -> LED1 (zworka J25)
#define STATUS_DDR   DDRC
#define STATUS_PORT  PORTC
#define STATUS_BIT   PC0

// Przycisk K1 (J26 KEY1) do GND z wewnętrznym pull-upem: PD2
#define BTN_DDR      DDRD
#define BTN_PORT     PORTD
#define BTN_PIN      PIND
#define BTN_BIT      PD2

// Czujnik DS18B20 (1-Wire, pull-up na płytce): PD3
#define OW_DDR       DDRD
#define OW_PORT      PORTD
#define OW_PIN       PIND
#define OW_BIT       PD3

// Kanały ADC: PA0 – potencjometr P1 (POT), PA1 – fotorezystor (poza płytką); zworki J30 0–1 zdjęte
#define ADC_CH_POT   0
#define ADC_CH_LDR   1

#endif
