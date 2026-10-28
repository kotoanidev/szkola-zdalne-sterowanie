#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

int16_t sensors_temp_tenths(void);
uint8_t sensors_pot_percent(void);
uint8_t sensors_auto_percent(void);

#endif
