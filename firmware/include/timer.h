#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void);
uint32_t timer_ms(void);
uint16_t timer_ms16(void);

#endif
