#ifndef STATE_H
#define STATE_H

#include <stdint.h>

enum {
    MODE_MANUAL,
    MODE_AUTO
};

typedef struct {
    uint8_t on;
    uint8_t pwm;
    uint8_t mode;
    uint8_t tlim;
    uint8_t alarm;
} state_t;

extern state_t state;

void state_init(void);
void state_tick(void);
void state_set_on(uint8_t on);
uint8_t state_set_pwm(uint8_t pwm);
void state_set_mode(uint8_t mode);
void state_set_tlim(uint8_t tlim);
void state_auto_pwm(uint8_t pwm);

#endif
