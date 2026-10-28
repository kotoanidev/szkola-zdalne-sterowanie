// Protokół tekstowy UART – specyfikacja w docs/protokol.md
#include <string.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "pinout.h"
#include "adc.h"
#include "sensors.h"
#include "state.h"
#include "timer.h"
#include "uart.h"
#include "protocol.h"

#define LINE_MAX 32

static char line[LINE_MAX + 1];
static uint8_t len;
static uint8_t too_long;

static void kv(PGM_P key, uint32_t value)
{
    uart_puts_P(key);
    uart_put_uint(value);
}

static void kv_mode(void)
{
    uart_puts_P(PSTR(" mode="));
    uart_puts_P(state.mode == MODE_AUTO ? PSTR("AUTO") : PSTR("MANUAL"));
}

static void kv_temp(int16_t temp)
{
    uart_puts_P(PSTR(" temp="));
    uart_put_tenths(temp);
}

static void reply_P(PGM_P s)
{
    uart_puts_P(s);
    uart_endl();
}

static void reply_on(void)
{
    uart_puts_P(PSTR("OK"));
    kv(PSTR(" on="), state.on);
    uart_endl();
}

static void reply_state(void)
{
    uart_puts_P(PSTR("STATE"));
    kv(PSTR(" on="), state.on);
    kv(PSTR(" pwm="), state.pwm);
    kv_mode();
    kv(PSTR(" pot="), adc_value(ADC_CH_POT));
    kv(PSTR(" ldr="), adc_value(ADC_CH_LDR));
    kv_temp(sensors_temp_tenths());
    kv(PSTR(" alarm="), state.alarm);
    kv(PSTR(" tlim="), state.tlim);
    kv(PSTR(" up="), timer_ms() / 1000);
    uart_endl();
}

static uint8_t parse_uint(const char *s, uint8_t max, uint8_t *out)
{
    uint16_t v = 0;

    if (*s == '\0')
        return 0;
    for (; *s != '\0'; s++) {
        if (*s < '0' || *s > '9')
            return 0;
        v = v * 10 + (uint16_t)(*s - '0');
        if (v > max)
            return 0;
    }
    *out = (uint8_t)v;
    return 1;
}

static void execute(char *cmd)
{
    char *arg = strchr(cmd, ' ');
    uint8_t value;

    if (arg != NULL) {
        *arg++ = '\0';
        while (*arg == ' ')
            arg++;
    } else {
        arg = cmd + strlen(cmd);
    }

    if (strcmp_P(cmd, PSTR("PWM")) == 0) {
        if (!parse_uint(arg, 100, &value)) {
            reply_P(PSTR("ERR ARG"));
            return;
        }
        uint8_t mode_changed = state_set_pwm(value);

        uart_puts_P(PSTR("OK"));
        kv(PSTR(" pwm="), value);
        if (mode_changed)
            kv_mode();
        uart_endl();
    } else if (strcmp_P(cmd, PSTR("MODE")) == 0) {
        if (strcmp_P(arg, PSTR("AUTO")) == 0) {
            state_set_mode(MODE_AUTO);
        } else if (strcmp_P(arg, PSTR("MANUAL")) == 0) {
            state_set_mode(MODE_MANUAL);
        } else {
            reply_P(PSTR("ERR ARG"));
            return;
        }
        uart_puts_P(PSTR("OK"));
        kv_mode();
        uart_endl();
    } else if (strcmp_P(cmd, PSTR("TLIM")) == 0) {
        if (!parse_uint(arg, TEMP_LIMIT_MAX, &value)) {
            reply_P(PSTR("ERR ARG"));
            return;
        }
        state_set_tlim(value);
        uart_puts_P(PSTR("OK"));
        kv(PSTR(" tlim="), value);
        uart_endl();
    } else if (*arg != '\0') {
        // Pozostałe komendy nie przyjmują argumentów
        if (strcmp_P(cmd, PSTR("PING")) == 0 || strcmp_P(cmd, PSTR("VER")) == 0
            || strcmp_P(cmd, PSTR("ON")) == 0 || strcmp_P(cmd, PSTR("OFF")) == 0
            || strcmp_P(cmd, PSTR("TOGGLE")) == 0 || strcmp_P(cmd, PSTR("GET")) == 0)
            reply_P(PSTR("ERR ARG"));
        else
            reply_P(PSTR("ERR UNKNOWN"));
    } else if (strcmp_P(cmd, PSTR("PING")) == 0) {
        reply_P(PSTR("PONG"));
    } else if (strcmp_P(cmd, PSTR("VER")) == 0) {
        reply_P(PSTR("VER " FW_VERSION));
    } else if (strcmp_P(cmd, PSTR("ON")) == 0) {
        state_set_on(1);
        reply_on();
    } else if (strcmp_P(cmd, PSTR("OFF")) == 0) {
        state_set_on(0);
        reply_on();
    } else if (strcmp_P(cmd, PSTR("TOGGLE")) == 0) {
        state_set_on(!state.on);
        reply_on();
    } else if (strcmp_P(cmd, PSTR("GET")) == 0) {
        reply_state();
    } else {
        reply_P(PSTR("ERR UNKNOWN"));
    }
}

uint8_t protocol_rx(char c)
{
    if (c == '\r')
        return 0;

    if (c == '\n') {
        uint8_t executed = 0;

        while (len > 0 && line[len - 1] == ' ')
            len--;
        line[len] = '\0';
        if (too_long) {
            reply_P(PSTR("ERR TOOLONG"));
            executed = 1;
        } else if (len > 0) {
            char *cmd = line;
            while (*cmd == ' ')
                cmd++;
            execute(cmd);
            executed = 1;
        }
        len = 0;
        too_long = 0;
        return executed;
    }

    // Zgodność z instrukcją: pojedyncze 1 / 0 działa od razu, bez Entera
    if (len == 0 && !too_long && (c == '1' || c == '0')) {
        state_set_on(c == '1');
        reply_on();
        return 1;
    }

    if (len >= LINE_MAX) {
        too_long = 1;
        return 0;
    }
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    line[len++] = c;
    return 0;
}

void protocol_event_boot(PGM_P reason)
{
    uart_puts_P(PSTR("EVT BOOT reset="));
    uart_puts_P(reason);
    uart_puts_P(PSTR(" ver=" FW_VERSION));
    uart_endl();
}

void protocol_event_btn_on(void)
{
    uart_puts_P(PSTR("EVT BTN"));
    kv(PSTR(" on="), state.on);
    uart_endl();
}

void protocol_event_btn_mode(void)
{
    uart_puts_P(PSTR("EVT BTN"));
    kv_mode();
    uart_endl();
}

void protocol_event_pot(uint8_t mode_changed)
{
    uart_puts_P(PSTR("EVT POT"));
    kv(PSTR(" pwm="), state.pwm);
    if (mode_changed)
        kv_mode();
    uart_endl();
}

void protocol_event_temp(int16_t temp)
{
    uart_puts_P(PSTR("EVT TEMP"));
    kv(PSTR(" alarm="), state.alarm);
    kv_temp(temp);
    uart_endl();
}
