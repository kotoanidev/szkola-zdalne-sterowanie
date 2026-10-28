"""Model ATmega8 na poziomie protokołu – zachowuje się jak firmware, ale bez sprzętu.

Stałe odpowiadają firmware/include/config.h (czujnik LM35, VREF 5 V).
"""

from __future__ import annotations

import queue
import threading
import time
from typing import Callable

from .protocol import LINE_MAX, TLIM_MAX

FW_VERSION = "0.9.0-sim"
ADC_VREF_MV = 5000
POT_HYST = 2
AUTO_HYST = 3
LDR_DARK, LDR_BRIGHT = 200, 800
TEMP_HYST = 2.0
POT_PERIOD, AUTO_PERIOD, TEMP_PERIOD = 0.05, 0.2, 0.5


def pot_percent(raw: int) -> int:
    return ((raw >> 2) * 100 + 127) // 255


def auto_percent(ldr: int) -> int:
    if ldr <= LDR_DARK:
        return 100
    if ldr >= LDR_BRIGHT:
        return 0
    return 100 - (ldr - LDR_DARK) * 100 // (LDR_BRIGHT - LDR_DARK)


class McuSim:
    def __init__(self, clock: Callable[[], float] = time.monotonic) -> None:
        self._clock = clock
        self._lock = threading.RLock()
        self._out: queue.Queue[str] = queue.Queue()
        self.eeprom = {"on": False, "pwm": 50, "mode": "MANUAL", "tlim": 50}
        # Wirtualny sprzęt: surowe odczyty ADC i temperatura czujnika
        self.pot = 512
        self.ldr = 500
        self.temp_c = 22.0
        self.reset("POR")

    # --- wirtualny sprzęt -------------------------------------------------

    def reset(self, reason: str = "EXT") -> None:
        with self._lock:
            self.on = self.eeprom["on"]
            self.mode = self.eeprom["mode"]
            self.pwm = self.eeprom["pwm"]
            self.tlim = self.eeprom["tlim"]
            self.alarm = False
            self._line = ""
            self._too_long = False
            self._pot_last: int | None = None
            now = self._clock()
            self._boot = now
            self._next = {"pot": now, "auto": now, "temp": now}
            self._emit(f"EVT BOOT reset={reason} ver={FW_VERSION}")

    def press_button(self, long: bool = False) -> None:
        with self._lock:
            if long:
                self.mode = "MANUAL" if self.mode == "AUTO" else "AUTO"
                self._save()
                self._emit(f"EVT BTN mode={self.mode}")
            else:
                self.on = not self.on
                self._save()
                self._emit(f"EVT BTN on={int(self.on)}")

    def set_inputs(self, pot: int | None = None, ldr: int | None = None,
                   temp_c: float | None = None) -> None:
        with self._lock:
            if pot is not None:
                self.pot = max(0, min(1023, int(pot)))
            if ldr is not None:
                self.ldr = max(0, min(1023, int(ldr)))
            if temp_c is not None:
                self.temp_c = max(-20.0, min(150.0, float(temp_c)))

    @property
    def brightness(self) -> int:
        return self.pwm if self.on else 0

    def snapshot(self) -> dict:
        with self._lock:
            return {
                "pot": self.pot,
                "ldr": self.ldr,
                "temp_c": self.temp_c,
                "led": self.brightness,
                "status_alarm": self.alarm,
            }

    # --- UART -------------------------------------------------------------

    def feed(self, data: str) -> None:
        with self._lock:
            for c in data:
                self._rx(c)

    def read_line(self, timeout: float = 0.0) -> str | None:
        try:
            return self._out.get(timeout=timeout) if timeout > 0 else self._out.get_nowait()
        except queue.Empty:
            return None

    def _emit(self, line: str) -> None:
        self._out.put(line)

    def _rx(self, c: str) -> None:
        if c == "\r":
            return
        if c == "\n":
            line = self._line.strip()
            if self._too_long:
                self._emit("ERR TOOLONG")
            elif line:
                self._execute(line)
            self._line = ""
            self._too_long = False
            return
        if not self._line and not self._too_long and c in "10":
            self._set_on(c == "1")
            self._emit(f"OK on={int(self.on)}")
            return
        if len(self._line) >= LINE_MAX:
            self._too_long = True
            return
        self._line += c.upper()

    def _execute(self, line: str) -> None:
        cmd, _, arg = line.partition(" ")
        arg = arg.strip()

        if cmd == "PWM":
            value = _parse_uint(arg, 100)
            if value is None:
                self._emit("ERR ARG")
                return
            mode_changed = self.mode != "MANUAL"
            self._set_pwm(value)
            self._emit(f"OK pwm={value}" + (" mode=MANUAL" if mode_changed else ""))
        elif cmd == "MODE":
            if arg not in ("AUTO", "MANUAL"):
                self._emit("ERR ARG")
                return
            self.mode = arg
            self._save()
            self._emit(f"OK mode={self.mode}")
        elif cmd == "TLIM":
            value = _parse_uint(arg, TLIM_MAX)
            if value is None:
                self._emit("ERR ARG")
                return
            self.tlim = value
            self._save()
            self._emit(f"OK tlim={value}")
        elif cmd in ("PING", "VER", "ON", "OFF", "TOGGLE", "GET") and arg:
            self._emit("ERR ARG")
        elif cmd == "PING":
            self._emit("PONG")
        elif cmd == "VER":
            self._emit(f"VER {FW_VERSION}")
        elif cmd in ("ON", "OFF", "TOGGLE"):
            self._set_on(not self.on if cmd == "TOGGLE" else cmd == "ON")
            self._emit(f"OK on={int(self.on)}")
        elif cmd == "GET":
            self._emit(self._state_line())
        else:
            self._emit("ERR UNKNOWN")

    def _state_line(self) -> str:
        up = int(self._clock() - self._boot)
        return (f"STATE on={int(self.on)} pwm={self.pwm} mode={self.mode} pot={self.pot} "
                f"ldr={self.ldr} temp={self._temp_reading():.1f} alarm={int(self.alarm)} "
                f"tlim={self.tlim} up={up}")

    # --- logika jak w firmware ---------------------------------------------

    def _set_on(self, on: bool) -> None:
        self.on = on
        self._save()

    def _set_pwm(self, value: int) -> None:
        self.mode = "MANUAL"
        self.pwm = value
        self._save()

    def _save(self) -> None:
        self.eeprom["on"] = self.on
        self.eeprom["mode"] = self.mode
        self.eeprom["tlim"] = self.tlim
        if self.mode == "MANUAL":
            self.eeprom["pwm"] = self.pwm

    def _temp_reading(self) -> float:
        # Kwantyzacja jak w ADC (suma 16 próbek, LM35 10 mV/°C)
        raw_sum = round(self.temp_c * 10 * 16384 / ADC_VREF_MV)
        raw_sum = max(0, min(16368, raw_sum))
        return ((raw_sum * ADC_VREF_MV + 8192) >> 14) / 10

    def tick(self) -> None:
        """Okresowe zadania firmware: potencjometr, tryb AUTO, alarm temperatury."""
        with self._lock:
            now = self._clock()
            if now >= self._next["pot"]:
                self._next["pot"] = now + POT_PERIOD
                self._pot_check()
            if now >= self._next["auto"]:
                self._next["auto"] = now + AUTO_PERIOD
                self._auto_check()
            if now >= self._next["temp"]:
                self._next["temp"] = now + TEMP_PERIOD
                self._temp_check()

    def _pot_check(self) -> None:
        p = pot_percent(self.pot)
        if self._pot_last is None:
            self._pot_last = p
            return
        if p == self._pot_last:
            return
        if abs(p - self._pot_last) < POT_HYST and p not in (0, 100):
            return
        self._pot_last = p
        mode_changed = self.mode != "MANUAL"
        self._set_pwm(p)
        self._emit(f"EVT POT pwm={p}" + (" mode=MANUAL" if mode_changed else ""))

    def _auto_check(self) -> None:
        if self.mode != "AUTO":
            return
        p = auto_percent(self.ldr)
        if p != self.pwm and (abs(p - self.pwm) >= AUTO_HYST or p in (0, 100)):
            self.pwm = p

    def _temp_check(self) -> None:
        t = self._temp_reading()
        if not self.alarm and t >= self.tlim:
            self.alarm = True
            self._emit(f"EVT TEMP alarm=1 temp={t:.1f}")
        elif self.alarm and t < self.tlim - TEMP_HYST:
            self.alarm = False
            self._emit(f"EVT TEMP alarm=0 temp={t:.1f}")


def _parse_uint(text: str, maximum: int) -> int | None:
    if not text.isdigit() or not text.isascii():
        return None
    value = int(text)
    return value if value <= maximum else None
