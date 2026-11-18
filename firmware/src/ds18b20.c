// Czujnik DS18B20 na 1-Wire: pomiar co 1 s, w jednym takcie 1 ms co najwyżej reset albo jeden bajt
#include <avr/io.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <util/delay.h>
#include "pinout.h"
#include "ds18b20.h"

#define CONVERT_MS 750
#define PERIOD_MS  1000
// Bit trzeba odczytać najpóźniej 15 µs po zboczu; przy 1 MHz jedna instrukcja to 1–2 µs
#define SAMPLE_US  (F_CPU > 4000000UL ? 9 : 2)

#define CMD_SKIP_ROM  0xCC
#define CMD_CONVERT   0x44
#define CMD_READ      0xBE

// Linia z pull-upem na płytce: stan niski = pin jako wyjście (PORT = 0), zwolnienie = wejście
#define bus_low()     (OW_DDR |= _BV(OW_BIT))
#define bus_release() (OW_DDR &= ~_BV(OW_BIT))
#define bus_high()    ((OW_PIN & _BV(OW_BIT)) != 0)

static int16_t tenths = TEMP_NONE;
static uint16_t wait_ms;
static uint8_t step;
static uint8_t scratch[9];

static uint8_t bus_reset(void)
{
    uint8_t presence;

    bus_low();
    _delay_us(480);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bus_release();
        _delay_us(70);
        presence = !bus_high();
    }
    _delay_us(410);
    // Linia musi wrócić do stanu wysokiego, inaczej jest zwarta do masy
    return presence && bus_high();
}

// Szczelina czasowa: zapis bitu; zapis jedynki jest jednocześnie odczytem
static uint8_t bus_bit(uint8_t bit)
{
    uint8_t r;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bus_low();
        _delay_us(1);
        if (bit)
            bus_release();
        _delay_us(SAMPLE_US);
        r = bus_high();
        _delay_us(55);
        bus_release();
    }
    _delay_us(2);
    return r;
}

// Wysyła bajt (LSB pierwszy) i zwraca odczytane bity; 0xFF oznacza sam odczyt
static uint8_t bus_byte(uint8_t b)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
        b = (uint8_t)((b >> 1) | (bus_bit(b & 1) ? 0x80 : 0));
    return b;
}

static int16_t decode(void)
{
    uint8_t crc = 0;
    uint8_t i;
    int16_t raw;

    // CRC liczone razem z ostatnim bajtem (CRC) daje 0
    for (i = 0; i < sizeof(scratch); i++)
        crc = _crc_ibutton_update(crc, scratch[i]);
    if (crc != 0)
        return TEMP_NONE;
    // 1/16 °C -> 0,1 °C z zaokrągleniem
    raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    return (int16_t)((raw * 10 + (raw < 0 ? -8 : 8)) / 16);
}

void ds18b20_tick(void)
{
    uint8_t s;

    if (wait_ms > 0) {
        wait_ms--;
        return;
    }
    s = step++;
    switch (s) {
    case 0:
    case 3:
        if (!bus_reset()) {
            tenths = TEMP_NONE;
            step = 0;
            wait_ms = PERIOD_MS;
        }
        break;
    case 1:
    case 4:
        // Na linii jest jeden czujnik, więc bez adresu
        bus_byte(CMD_SKIP_ROM);
        break;
    case 2:
        bus_byte(CMD_CONVERT);
        wait_ms = CONVERT_MS;
        break;
    case 5:
        bus_byte(CMD_READ);
        break;
    default:
        s -= 6;
        scratch[s] = bus_byte(0xFF);
        if (s == sizeof(scratch) - 1) {
            tenths = decode();
            step = 0;
            wait_ms = PERIOD_MS - CONVERT_MS;
        }
        break;
    }
}

int16_t ds18b20_tenths(void)
{
    return tenths;
}
