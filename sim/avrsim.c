// Symulacja firmware ATmega32A (płytka ATB 1.05) w simavr: UART na pseudoterminalu, wirtualny
// przycisk, potencjometr, fotorezystor i czujnik DS18B20 na 1-Wire.
//
//   avrsim [--speed 1.0] [--freq 1000000] [--link ŚCIEŻKA] [--eeprom PLIK] firmware.hex
//
// Sterowanie liniami na stdin: btn press [ms] | btn down | btn up | pot <0-1023> | ldr <0-1023>
// | temp <°C> | temp none | mv <kanał> <mV> | reset | power | state | quit
// Zdarzenia jako JSON (po jednym w linii) na stdout: ready, led, status, tx, btn, reset, state.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_io.h"
#include "sim_time.h"
#include "avr_adc.h"
#include "avr_eeprom.h"
#include "avr_ioport.h"
#include "avr_uart.h"

#define VREF_MV 5000
#define ADC_INPUTS 2

// Diody na ATB świecą stanem niskim – jak LED_ACTIVE_LOW w firmware/include/pinout.h
#define LED_ACTIVE_LOW 1
#define LED_BIT    5   // PD5 / OC1A
#define STATUS_BIT 0   // PC0
#define BTN_BIT    2   // PD2
#define OW_BIT     3   // PD3, DS18B20

// Rejestry ATmega32 w przestrzeni danych (adres I/O + 0x20)
#define REG_OCR1AL 0x4A
#define REG_OCR1AH 0x4B
#define REG_TCCR1A 0x4F
#define REG_PORTD  0x32
#define REG_MCUCSR 0x54
#define BIT_COM1A1 7
#define BIT_COM1A0 6
#define BIT_PORF   0
#define BIT_EXTRF  1

#define LED_REPORT_NS 20000000ULL

static avr_t *avr;
static int pty_master = -1;
static int pty_slave = -1;
static const char *link_path;
static const char *eeprom_path;
static double speed = 1.0;
// Zegar jak F_CPU firmware: 1 MHz (RC) albo 11059200 (kwarc ATB)
static uint32_t freq = 1000000;
static uint64_t t0;
static volatile sig_atomic_t stop_requested;

static int uart_xon;
static uint8_t pending[256];
static size_t pending_len, pending_pos;
static char tx_line[128];
static size_t tx_len;

static avr_irq_t *uart_in;
static avr_irq_t *button_irq;
static avr_irq_t *adc_irq[ADC_INPUTS];
static uint32_t adc_mv[ADC_INPUTS] = { 2500, 2500 };
static int button_down;

// DS18B20: stan czujnika i automat protokołu 1-Wire
enum { OW_IDLE, OW_ROM, OW_FUNC, OW_TX };
static avr_irq_t *ow_irq;
static int ow_present = 1;
static int16_t ow_raw = 22 * 16;
static int16_t ow_converted = 85 * 16;   // wartość po włączeniu zasilania
static int ow_state = OW_IDLE;
static int ow_master_low;
static int ow_slave_low;
static avr_cycle_count_t ow_fall;
static uint8_t ow_in, ow_bits;
static uint8_t ow_tx[9];
static int ow_tx_bit;

static double led_duty = -1;
static uint64_t led_reported_ns;

static uint64_t now_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static double sim_time(void)
{
    return (double)avr->cycle / freq;
}

static void emit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
}

// Logi simavr idą na stderr, żeby nie mieszały się z JSON na stdout
static void logger(avr_t *a, const int level, const char *fmt, va_list ap)
{
    (void)a;
    if (level <= LOG_WARNING)
        vfprintf(stderr, fmt, ap);
}

static void sleep_noop(avr_t *a, avr_cycle_count_t how_long)
{
    (void)a;
    (void)how_long;
}

// --- UART <-> pseudoterminal -------------------------------------------------

static void uart_out_hook(avr_irq_t *irq, uint32_t value, void *param)
{
    uint8_t c = (uint8_t)value;
    (void)irq;
    (void)param;

    // Gdy nikt nie czyta pseudoterminala, nadmiar znaków przepada jak na prawdziwym łączu
    if (write(pty_master, &c, 1) < 0 && errno != EAGAIN)
        perror("pty write");

    if (c == '\n') {
        tx_line[tx_len] = '\0';
        emit("{\"ev\":\"tx\",\"t\":%.3f,\"line\":\"%s\"}", sim_time(), tx_line);
        tx_len = 0;
    } else if (c >= 0x20 && c < 0x7F && c != '"' && c != '\\' && tx_len < sizeof(tx_line) - 1) {
        tx_line[tx_len++] = (char)c;
    }
}

static void uart_xon_hook(avr_irq_t *irq, uint32_t value, void *param)
{
    (void)irq;
    (void)value;
    (void)param;
    uart_xon = 1;
}

static void uart_xoff_hook(avr_irq_t *irq, uint32_t value, void *param)
{
    (void)irq;
    (void)param;
    if (value)
        uart_xon = 0;
}

static void feed_uart(void)
{
    while (uart_xon && pending_pos < pending_len)
        avr_raise_irq(uart_in, pending[pending_pos++]);
    if (pending_pos == pending_len)
        pending_pos = pending_len = 0;
}

static void read_pty(void)
{
    ssize_t n;

    if (pending_len > 0)
        return;
    n = read(pty_master, pending, sizeof(pending));
    if (n > 0)
        pending_len = (size_t)n;
}

static void open_pty(void)
{
    struct termios t;
    const char *name;

    pty_master = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_master < 0 || grantpt(pty_master) < 0 || unlockpt(pty_master) < 0) {
        perror("pty");
        exit(1);
    }
    name = ptsname(pty_master);
    // Trzymamy stronę slave otwartą, żeby odłączenie klienta nie zamykało łącza
    pty_slave = open(name, O_RDWR | O_NOCTTY);
    if (pty_slave < 0 || tcgetattr(pty_slave, &t) < 0) {
        perror("pty slave");
        exit(1);
    }
    cfmakeraw(&t);
    cfsetspeed(&t, B9600);
    tcsetattr(pty_slave, TCSANOW, &t);
    fcntl(pty_master, F_SETFL, fcntl(pty_master, F_GETFL) | O_NONBLOCK);

    if (link_path) {
        unlink(link_path);
        if (symlink(name, link_path) < 0)
            perror("symlink");
    }
    emit("{\"ev\":\"ready\",\"pty\":\"%s\",\"link\":\"%s\"}", name, link_path ? link_path : "");
}

// --- wirtualny sprzęt ----------------------------------------------------------

static void status_hook(avr_irq_t *irq, uint32_t value, void *param)
{
    (void)irq;
    (void)param;
    emit("{\"ev\":\"status\",\"t\":%.3f,\"on\":%u}", sim_time(),
         (value ? 1 : 0) ^ LED_ACTIVE_LOW);
}

// Jasność diody głównej = część okresu, w której dioda przewodzi
static double read_led_duty(void)
{
    uint8_t tccr = avr->data[REG_TCCR1A];
    double high;

    if (!(tccr & (1 << BIT_COM1A1))) {
        high = (avr->data[REG_PORTD] >> LED_BIT) & 1;
    } else {
        uint16_t ocr = (uint16_t)(avr->data[REG_OCR1AL] | (avr->data[REG_OCR1AH] << 8));
        double d = ocr >= 1023 ? 1.0 : ocr / 1023.0;
        high = (tccr & (1 << BIT_COM1A0)) ? 1.0 - d : d;
    }
    return LED_ACTIVE_LOW ? 1.0 - high : high;
}

static void report_led(int force)
{
    double duty = read_led_duty();
    uint64_t now = now_ns();

    if (!force && (fabs(duty - led_duty) < 1e-9 || now - led_reported_ns < LED_REPORT_NS))
        return;
    led_duty = duty;
    led_reported_ns = now;
    emit("{\"ev\":\"led\",\"t\":%.3f,\"duty\":%.4f}", sim_time(), duty);
}

// Przycisk (PD2) i linia 1-Wire (PD3) to zewnętrzne wymuszenie stanu pinów; samo podniesienie
// IRQ zostałoby nadpisane przy następnym zapisie PORTD. Obie linie mają pull-up.
static void portd_external(void)
{
    avr_ioport_external_t ext = {
        .name = 'D',
        .mask = (1 << BTN_BIT) | (1 << OW_BIT),
        .value = (button_down ? 0 : 1 << BTN_BIT) | (ow_slave_low ? 0 : 1 << OW_BIT),
    };

    avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL('D'), &ext);
}

static void drive_button(int pressed)
{
    button_down = pressed;
    portd_external();
    avr_raise_irq(button_irq, pressed ? 0 : 1);
}

static void set_button(int pressed)
{
    drive_button(pressed);
    emit("{\"ev\":\"btn\",\"t\":%.3f,\"pressed\":%d}", sim_time(), pressed);
}

static avr_cycle_count_t release_button(avr_t *a, avr_cycle_count_t when, void *param)
{
    (void)a;
    (void)when;
    (void)param;
    set_button(0);
    return 0;
}

static void set_adc(int ch, uint32_t mv)
{
    if (mv > VREF_MV)
        mv = VREF_MV;
    adc_mv[ch] = mv;
    avr_raise_irq(adc_irq[ch], mv);
}

static uint32_t raw_to_mv(long raw)
{
    if (raw < 0)
        raw = 0;
    if (raw > 1023)
        raw = 1023;
    // ADC w simavr liczy mV * 1023 / VREF; środek przedziału daje dokładnie tę wartość
    return (uint32_t)(((double)raw + 0.5) * VREF_MV / 1023.0);
}

// --- DS18B20 na 1-Wire --------------------------------------------------------------
// Czasy z karty katalogowej w wariancie najmniej korzystnym dla mikrokontrolera: bit 0 trzyma
// linię tylko 15 µs, obecność trwa od 60 do 120 µs po zwolnieniu linii przez reset.

static void ow_drive(int low)
{
    ow_slave_low = low;
    portd_external();
    avr_raise_irq(ow_irq, low ? 0 : 1);
}

static avr_cycle_count_t ow_drive_timer(avr_t *a, avr_cycle_count_t when, void *param)
{
    (void)a;
    (void)when;
    ow_drive(param != NULL);
    return 0;
}

static void ow_schedule(uint32_t us, int low)
{
    avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, us), ow_drive_timer,
                             low ? (void *)1 : NULL);
}

static uint8_t crc8(const uint8_t *data, int n)
{
    uint8_t crc = 0;
    int i, b;

    for (i = 0; i < n; i++) {
        crc ^= data[i];
        for (b = 0; b < 8; b++)
            crc = (crc & 1) ? (uint8_t)((crc >> 1) ^ 0x8C) : (uint8_t)(crc >> 1);
    }
    return crc;
}

static void ow_command(uint8_t cmd)
{
    if (ow_state == OW_ROM) {
        ow_state = cmd == 0xCC ? OW_FUNC : OW_IDLE;   // tylko SKIP ROM
    } else if (cmd == 0x44) {
        ow_converted = ow_raw;
        ow_state = OW_IDLE;
    } else if (cmd == 0xBE) {
        uint8_t pad[9] = { (uint8_t)ow_converted, (uint8_t)((uint16_t)ow_converted >> 8),
                           0x4B, 0x46, 0x7F, 0xFF, 0x0C, 0x10, 0 };
        pad[8] = crc8(pad, 8);
        memcpy(ow_tx, pad, sizeof(ow_tx));
        ow_tx_bit = 0;
        ow_state = OW_TX;
    } else {
        ow_state = OW_IDLE;
    }
}

static void ow_violation(const char *what, double us)
{
    fprintf(stderr, "avrsim: 1-Wire: %s (%.1f us)\n", what, us);
    ow_state = OW_IDLE;
}

static void ow_master_edge(int low)
{
    double us = (double)(avr->cycle - ow_fall) * 1e6 / freq;

    if (low) {
        ow_fall = avr->cycle;
        if (ow_present && ow_state == OW_TX) {
            if (!((ow_tx[ow_tx_bit / 8] >> (ow_tx_bit % 8)) & 1)) {
                ow_drive(1);
                ow_schedule(15, 0);
            }
            if (++ow_tx_bit == (int)sizeof(ow_tx) * 8)
                ow_state = OW_IDLE;
        }
        return;
    }
    if (!ow_present)
        return;
    if (us >= 480) {
        ow_state = OW_ROM;
        ow_bits = 0;
        ow_schedule(60, 1);
        ow_schedule(120, 0);
    } else if (ow_state == OW_ROM || ow_state == OW_FUNC) {
        if (us >= 15 && us < 60) {
            ow_violation("bit zapisu poza dozwolonym czasem", us);
            return;
        }
        ow_in = (uint8_t)((ow_in >> 1) | (us < 15 ? 0x80 : 0));
        if (++ow_bits == 8) {
            ow_bits = 0;
            ow_command(ow_in);
        }
    } else if (us >= 15) {
        ow_violation("szczelina odczytu dłuższa niż 15 us", us);
    }
}

// Mikrokontroler ściąga linię, ustawiając pin jako wyjście (PORT = 0)
static void ddrd_hook(avr_irq_t *irq, uint32_t value, void *param)
{
    int low = (value >> OW_BIT) & 1;
    (void)irq;
    (void)param;

    if (low != ow_master_low) {
        ow_master_low = low;
        ow_master_edge(low);
    }
}

static void set_temp(const char *arg, double c)
{
    if (strcmp(arg, "none") == 0) {
        ow_present = 0;
        return;
    }
    if (c < -55)
        c = -55;
    if (c > 125)
        c = 125;
    ow_present = 1;
    ow_raw = (int16_t)lround(c * 16);
}

static void apply_inputs(void)
{
    int ch;

    ow_state = OW_IDLE;
    ow_master_low = 0;
    ow_slave_low = 0;
    drive_button(0);
    for (ch = 0; ch < ADC_INPUTS; ch++)
        avr_raise_irq(adc_irq[ch], adc_mv[ch]);
}

static void do_reset(int bit, const char *reason)
{
    avr_reset(avr);
    // avr_reset() zeruje licznik cykli, więc czas rzeczywisty liczymy od nowa
    t0 = now_ns();
    avr->data[REG_MCUCSR] |= (uint8_t)(1 << bit);
    apply_inputs();
    emit("{\"ev\":\"reset\",\"t\":%.3f,\"reason\":\"%s\"}", sim_time(), reason);
}

// --- EEPROM w pliku (stan po „wyłączeniu zasilania”) ------------------------------

static void eeprom_io(int save)
{
    uint8_t buf[512];
    avr_eeprom_desc_t d = { .ee = buf, .offset = 0, .size = sizeof(buf) };
    FILE *f;

    if (!eeprom_path)
        return;
    if (save) {
        if (avr_ioctl(avr, AVR_IOCTL_EEPROM_GET, &d) < 0 || !(f = fopen(eeprom_path, "wb")))
            return;
        fwrite(d.ee, 1, d.size, f);
        fclose(f);
    } else if ((f = fopen(eeprom_path, "rb")) != NULL) {
        d.size = (uint32_t)fread(buf, 1, sizeof(buf), f);
        fclose(f);
        if (d.size > 0)
            avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
    }
}

// --- sterowanie z stdin ---------------------------------------------------------

static void command(char *line)
{
    char cmd[16] = "", arg[16] = "";
    double v1 = 0, v2 = 0;
    int n = sscanf(line, "%15s %lf %lf", cmd, &v1, &v2);

    if (n <= 0)
        return;
    if (strcmp(cmd, "btn") == 0) {
        sscanf(line, "%*s %15s %lf", arg, &v1);
        if (strcmp(arg, "down") == 0) {
            set_button(1);
        } else if (strcmp(arg, "up") == 0) {
            set_button(0);
        } else {
            double ms = (strcmp(arg, "press") == 0 && v1 > 0) ? v1 : 100;
            set_button(1);
            avr_cycle_timer_register(avr, avr_usec_to_cycles(avr, (uint32_t)(ms * 1000)),
                                     release_button, NULL);
        }
    } else if (strcmp(cmd, "pot") == 0 && n >= 2) {
        set_adc(0, raw_to_mv((long)v1));
    } else if (strcmp(cmd, "ldr") == 0 && n >= 2) {
        set_adc(1, raw_to_mv((long)v1));
    } else if (strcmp(cmd, "temp") == 0 && sscanf(line, "%*s %15s", arg) == 1) {
        set_temp(arg, v1);
    } else if (strcmp(cmd, "mv") == 0 && n >= 3 && v1 >= 0 && v1 < ADC_INPUTS) {
        set_adc((int)v1, (uint32_t)v2);
    } else if (strcmp(cmd, "reset") == 0) {
        do_reset(BIT_EXTRF, "EXT");
    } else if (strcmp(cmd, "power") == 0) {
        eeprom_io(1);
        ow_converted = 85 * 16;
        do_reset(BIT_PORF, "POR");
    } else if (strcmp(cmd, "state") == 0) {
        emit("{\"ev\":\"state\",\"t\":%.3f,\"duty\":%.4f,\"pot_mv\":%u,\"ldr_mv\":%u,"
             "\"temp_c\":%.4f,\"ds18b20\":%d}",
             sim_time(), read_led_duty(), adc_mv[0], adc_mv[1], ow_raw / 16.0, ow_present);
    } else if (strcmp(cmd, "quit") == 0) {
        stop_requested = 1;
    } else {
        fprintf(stderr, "avrsim: nieznana komenda: %s\n", line);
    }
}

static void read_stdin(void)
{
    static char buf[256];
    static size_t len;
    ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - len);
    char *start, *nl;

    if (n == 0) {
        stop_requested = 1;
        return;
    }
    if (n < 0)
        return;
    len += (size_t)n;
    buf[len] = '\0';
    start = buf;
    while ((nl = strchr(start, '\n')) != NULL) {
        *nl = '\0';
        command(start);
        start = nl + 1;
    }
    len = strlen(start);
    memmove(buf, start, len);
    if (len == sizeof(buf) - 1)
        len = 0;
}

static void on_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void usage(const char *prog)
{
    fprintf(stderr, "użycie: %s [--speed X] [--freq HZ] [--link ŚCIEŻKA] [--eeprom PLIK] "
                    "firmware.hex\n", prog);
    exit(2);
}

int main(int argc, char *argv[])
{
    elf_firmware_t fw;
    const char *hex = NULL;
    int i, status = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
            speed = atof(argv[++i]);
        else if (strcmp(argv[i], "--freq") == 0 && i + 1 < argc)
            freq = (uint32_t)atol(argv[++i]);
        else if (strcmp(argv[i], "--link") == 0 && i + 1 < argc)
            link_path = argv[++i];
        else if (strcmp(argv[i], "--eeprom") == 0 && i + 1 < argc)
            eeprom_path = argv[++i];
        else if (argv[i][0] != '-' && !hex)
            hex = argv[i];
        else
            usage(argv[0]);
    }
    if (!hex)
        usage(argv[0]);

    avr_global_logger_set(logger);
    memset(&fw, 0, sizeof(fw));
    strcpy(fw.mmcu, "atmega32");
    fw.frequency = freq;
    sim_setup_firmware(hex, 0, &fw, argv[0]);

    avr = avr_make_mcu_by_name(fw.mmcu);
    if (!avr) {
        fprintf(stderr, "avrsim: brak rdzenia atmega32 w simavr\n");
        return 1;
    }
    avr_init(avr);
    avr_load_firmware(avr, &fw);
    avr->vcc = avr->avcc = avr->aref = VREF_MV;
    avr->sleep = sleep_noop;
    avr->data[REG_MCUCSR] |= 1 << BIT_PORF;
    eeprom_io(0);

    {
        uint32_t flags = 0;
        avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
        flags &= ~(uint32_t)AVR_UART_FLAG_STDIO;
        avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
    }
    uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
                            uart_out_hook, NULL);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XON),
                            uart_xon_hook, NULL);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUT_XOFF),
                            uart_xoff_hook, NULL);
    button_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN0 + BTN_BIT);
    ow_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN0 + OW_BIT);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_DIRECTION_ALL),
                            ddrd_hook, NULL);
    avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'),
                                          IOPORT_IRQ_PIN0 + STATUS_BIT),
                            status_hook, NULL);
    for (i = 0; i < ADC_INPUTS; i++)
        adc_irq[i] = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0 + i);
    apply_inputs();

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    open_pty();
    report_led(1);

    t0 = now_ns();
    while (!stop_requested) {
        struct pollfd fds[2] = {
            { .fd = STDIN_FILENO, .events = POLLIN },
            { .fd = pty_master, .events = POLLIN },
        };
        avr_cycle_count_t target = speed > 0
            ? (avr_cycle_count_t)((double)(now_ns() - t0) * 1e-9 * speed * freq)
            : avr->cycle + 50000;
        avr_cycle_count_t budget = avr->cycle + 100000;

        while (avr->cycle < target && avr->cycle < budget) {
            int state = avr_run(avr);
            if (state == cpu_Done || state == cpu_Crashed) {
                emit("{\"ev\":\"halt\",\"t\":%.3f,\"state\":%d}", sim_time(), state);
                stop_requested = 1;
                status = 1;
                break;
            }
        }
        feed_uart();
        report_led(0);
        if (poll(fds, 2, avr->cycle >= target ? 1 : 0) > 0) {
            if (fds[0].revents & (POLLIN | POLLHUP))
                read_stdin();
            if (fds[1].revents & POLLIN)
                read_pty();
        }
    }

    eeprom_io(1);
    if (link_path)
        unlink(link_path);
    avr_terminate(avr);
    return status;
}
