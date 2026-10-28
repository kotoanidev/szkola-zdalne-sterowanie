// Demonstracja etapu 1: miganie diody i przełączanie przyciskiem, tylko DDRx/PORTx/PINx.
// make demo && make flash-demo
#include <avr/io.h>
#include <util/delay.h>
#include "pinout.h"

int main(void)
{
    uint8_t on = 0;
    uint8_t prev = 1;
    uint8_t i;

    LED_DDR |= _BV(LED_BIT);
    STATUS_DDR |= _BV(STATUS_BIT);
    BTN_DDR &= ~_BV(BTN_BIT);
    BTN_PORT |= _BV(BTN_BIT);

    // Na start trzy mignięcia diody głównej
    for (i = 0; i < 6; i++) {
        LED_PORT ^= _BV(LED_BIT);
        _delay_ms(250);
    }

    for (i = 0;; i++) {
        uint8_t now = (BTN_PIN & _BV(BTN_BIT)) ? 1 : 0;

        if (prev && !now) {
            on = !on;
            if (on)
                LED_PORT |= _BV(LED_BIT);
            else
                LED_PORT &= ~_BV(LED_BIT);
        }
        prev = now;

        // Dioda statusu miga co 0,5 s, pętla co 10 ms (opóźnienie tłumi drgania styków)
        if (i == 50) {
            i = 0;
            STATUS_PORT ^= _BV(STATUS_BIT);
        }
        _delay_ms(10);
    }
}
