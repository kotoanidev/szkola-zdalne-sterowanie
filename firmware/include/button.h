#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

enum {
    BTN_NONE,
    BTN_SHORT,
    BTN_LONG
};

void button_init(void);
uint8_t button_tick(void);

#endif
