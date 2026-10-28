"""Tabela przeliczenia NTC dla firmware (src/sensors.c).

Dzielnik: NTC do VCC, rezystor R_SERIES do GND, pomiar na ADC2.
Wartości tabeli to suma 16 próbek ADC (jak zwraca adc_sum()).

    python tools/ntc_table.py [--beta 3950] [--r25 10000] [--rs 10000]
"""

import argparse
import math

T_MIN, T_MAX, T_STEP = -20, 125, 5
SAMPLES = 16


def adc_sum(temp_c: float, beta: float, r25: float, rs: float) -> int:
    t = temp_c + 273.15
    r_ntc = r25 * math.exp(beta * (1 / t - 1 / 298.15))
    return round(1024 * SAMPLES * rs / (rs + r_ntc))


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--beta", type=float, default=3950)
    p.add_argument("--r25", type=float, default=10000)
    p.add_argument("--rs", type=float, default=10000)
    a = p.parse_args()

    values = [adc_sum(t, a.beta, a.r25, a.rs) for t in range(T_MIN, T_MAX + 1, T_STEP)]
    print(f"// B = {a.beta:g}, R25 = {a.r25:g} Ω, Rs = {a.rs:g} Ω; od {T_MIN} °C co {T_STEP} °C")
    for i in range(0, len(values), 8):
        print("    " + ", ".join(str(v) for v in values[i:i + 8]) + ",")


if __name__ == "__main__":
    main()
