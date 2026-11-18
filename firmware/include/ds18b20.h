#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

// Brak ważnego odczytu (czujnik nie odpowiada albo błąd CRC)
#define TEMP_NONE INT16_MIN

void ds18b20_tick(void);
int16_t ds18b20_tenths(void);

#endif
