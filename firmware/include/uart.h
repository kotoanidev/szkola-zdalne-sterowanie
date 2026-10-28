#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <avr/pgmspace.h>

#define UART_BAUD 9600UL

void uart_init(void);
uint8_t uart_read(char *c);
void uart_putc(char c);
void uart_puts_P(PGM_P s);
void uart_put_uint(uint32_t value);
void uart_put_tenths(int16_t value);
void uart_endl(void);

#endif
