// UART 9600 bps 8N1 (U2X) na przerwaniach, z buforami kołowymi
#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

// Rozmiary muszą być potęgami dwójki
#define RX_SIZE 32
#define TX_SIZE 128
#define RX_MASK (RX_SIZE - 1)
#define TX_MASK (TX_SIZE - 1)

#define UBRR_VALUE ((F_CPU + 4 * UART_BAUD) / (8 * UART_BAUD) - 1)

static volatile char rx_buf[RX_SIZE];
static volatile uint8_t rx_head, rx_tail;
static volatile char tx_buf[TX_SIZE];
static volatile uint8_t tx_head, tx_tail;

ISR(USART_RXC_vect)
{
    char c = UDR;
    uint8_t next = (rx_head + 1) & RX_MASK;

    // Przy przepełnieniu znak przepada, komenda dostanie ERR
    if (next != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}

ISR(USART_UDRE_vect)
{
    if (tx_head != tx_tail) {
        UDR = tx_buf[tx_tail];
        tx_tail = (tx_tail + 1) & TX_MASK;
    } else {
        UCSRB &= ~_BV(UDRIE);
    }
}

void uart_init(void)
{
    UBRRH = (uint8_t)(UBRR_VALUE >> 8);
    UBRRL = (uint8_t)UBRR_VALUE;
    UCSRA = _BV(U2X);
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
    UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
}

uint8_t uart_read(char *c)
{
    if (rx_head == rx_tail)
        return 0;
    *c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & RX_MASK;
    return 1;
}

void uart_putc(char c)
{
    uint8_t next = (tx_head + 1) & TX_MASK;

    // Pełny bufor: czekamy, aż przerwanie wyśle znak (ok. 1 ms)
    while (next == tx_tail) {
    }
    tx_buf[tx_head] = c;
    tx_head = next;
    UCSRB |= _BV(UDRIE);
}

void uart_puts_P(PGM_P s)
{
    char c;

    while ((c = pgm_read_byte(s++)) != 0)
        uart_putc(c);
}

void uart_put_uint(uint32_t value)
{
    char digits[10];
    uint8_t n = 0;
    uint16_t small;

    // Dzielenie 32-bitowe jest przy 1 MHz wolne, więc używamy go tylko dla dużych liczb
    while (value > 0xFFFF) {
        digits[n++] = (char)('0' + value % 10);
        value /= 10;
    }
    small = (uint16_t)value;
    do {
        digits[n++] = (char)('0' + small % 10);
        small /= 10;
    } while (small > 0);
    while (n > 0)
        uart_putc(digits[--n]);
}

void uart_put_tenths(int16_t value)
{
    uint16_t v;

    if (value < 0) {
        uart_putc('-');
        v = (uint16_t)(-value);
    } else {
        v = (uint16_t)value;
    }
    uart_put_uint(v / 10);
    uart_putc('.');
    uart_putc((char)('0' + v % 10));
}

void uart_endl(void)
{
    uart_putc('\r');
    uart_putc('\n');
}
