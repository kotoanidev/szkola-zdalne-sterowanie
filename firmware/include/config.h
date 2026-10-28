// Ustawienia firmware. F_CPU definiuje tylko Makefile.
#ifndef CONFIG_H
#define CONFIG_H

#define FW_VERSION          "0.9.0"

// Czujnik temperatury na ADC2 – typ do ustalenia (docs/PLAN_PROJEKTU.md, sekcja 10)
#define TEMP_SENSOR_LM35    1   // 10 mV/°C
#define TEMP_SENSOR_NTC     2   // NTC 10 kΩ (B = 3950) do VCC, 10 kΩ do GND
#ifndef TEMP_SENSOR
#define TEMP_SENSOR         TEMP_SENSOR_LM35
#endif

// Napięcie odniesienia ADC (AVCC) w mV – do korekty po pomiarze multimetrem
#define ADC_VREF_MV         5000

// Domyślny próg alarmu temperatury [°C] i histereza wyłączenia [0,1 °C]
#define TEMP_LIMIT_DEFAULT  50
#define TEMP_LIMIT_MAX      125
#define TEMP_HYST_TENTHS    20

// 0 – potencjometr niepodłączony (ADC0 ignorowany)
#define POT_ENABLE          1
// Minimalna zmiana położenia potencjometru [%], która zmienia jasność
#define POT_HYST            2

// Tryb AUTO: odczyt ADC1 (więcej światła = wyższe napięcie) przy ciemności i przy pełnym świetle
#define LDR_DARK            200
#define LDR_BRIGHT          800
// 1 – fotorezystor podłączony odwrotnie (do GND)
#define LDR_INVERT          0
#define AUTO_HYST           3

// Płynne przejście: przyrost jasności (skala 0–1023) na 1 ms, pełna zmiana ok. 0,5 s
#define PWM_FADE_STEP       2

#define BTN_DEBOUNCE_MS     20
#define BTN_LONG_MS         1000

// Opóźnienie zapisu stanu w EEPROM po ostatniej zmianie [ms]
#define SAVE_DELAY_MS       2000

// Odkomentuj, jeśli w terminalu są „krzaki” (korekta oscylatora RC bez zmiany fusebitów)
// #define OSCCAL_VALUE     0xA8

#endif
