#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <avr/pgmspace.h>

uint8_t protocol_rx(char c);
void protocol_event_boot(PGM_P reason);
void protocol_event_btn_on(void);
void protocol_event_btn_mode(void);
void protocol_event_pot(uint8_t mode_changed);
void protocol_event_temp(int16_t temp);

#endif
