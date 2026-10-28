"""Prawdziwy firmware w simavr (sim/avrsim) – wirtualna płytka dla trybu SIMULATE=avr.

Interfejs jak w McuSim (set_inputs, press_button, reset, snapshot), a UART jest
pseudoterminalem, z którym serwer łączy się zwykłym SerialTransport. Tylko Linux.
"""

from __future__ import annotations

import json
import math
import subprocess
import threading
from pathlib import Path
from typing import Callable

REPO = Path(__file__).resolve().parents[2]
DEFAULT_BINARY = REPO / "sim" / "build" / "avrsim"
DEFAULT_HEX = REPO / "firmware" / "build" / "firmware.hex"
VREF_MV = 5000
LONG_PRESS_MS = 1500


class AvrSimError(Exception):
    pass


class AvrSim:
    def __init__(self, binary: Path = DEFAULT_BINARY, hex_file: Path = DEFAULT_HEX,
                 speed: float = 1.0, eeprom: Path | None = None, sensor: str = "lm35") -> None:
        for path, hint in ((binary, "make -C sim"), (hex_file, "make -C firmware")):
            if not Path(path).exists():
                raise AvrSimError(f"brak {path} – uruchom: {hint}")
        args = [str(binary), "--speed", str(speed), "--sensor", sensor]
        if eeprom is not None:
            args += ["--eeprom", str(eeprom)]
        args.append(str(hex_file))

        self.pot = 512
        self.ldr = 500
        self.temp_c = 22.0
        self.duty = 0.0
        self.status = False
        self.pty: str | None = None
        self.on_change: Callable[[dict], None] | None = None
        self.tx_log: list[str] = []
        self._lock = threading.Lock()
        self._ready = threading.Event()
        self._proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                      text=True, bufsize=1)
        self._reader = threading.Thread(target=self._read_events, name="avrsim", daemon=True)
        self._reader.start()
        if not self._ready.wait(5):
            self.close()
            raise AvrSimError("avrsim nie wystartował")

    # --- zdarzenia z symulatora --------------------------------------------------

    def _read_events(self) -> None:
        assert self._proc.stdout is not None
        for line in self._proc.stdout:
            try:
                ev = json.loads(line)
            except json.JSONDecodeError:
                continue
            kind = ev.get("ev")
            changed = False
            with self._lock:
                if kind == "ready":
                    self.pty = ev["pty"]
                    self._ready.set()
                elif kind == "led":
                    self.duty = float(ev["duty"])
                    changed = True
                elif kind == "status":
                    self.status = bool(ev["on"])
                    changed = True
                elif kind == "tx":
                    self.tx_log.append(ev["line"])
                    del self.tx_log[:-200]
            if changed and self.on_change is not None:
                self.on_change(self.snapshot())

    def _send(self, line: str) -> None:
        if self._proc.poll() is not None or self._proc.stdin is None:
            raise AvrSimError("avrsim nie działa")
        self._proc.stdin.write(line + "\n")
        self._proc.stdin.flush()

    # --- wirtualny sprzęt (jak McuSim) ------------------------------------------------

    def set_inputs(self, pot: int | None = None, ldr: int | None = None,
                   temp_c: float | None = None) -> None:
        if pot is not None:
            self.pot = max(0, min(1023, int(pot)))
            self._send(f"pot {self.pot}")
        if ldr is not None:
            self.ldr = max(0, min(1023, int(ldr)))
            self._send(f"ldr {self.ldr}")
        if temp_c is not None:
            self.temp_c = max(-20.0, min(150.0, float(temp_c)))
            self._send(f"temp {self.temp_c}")

    def press_button(self, long: bool = False) -> None:
        self._send(f"btn press {LONG_PRESS_MS if long else 100}")

    def reset(self, reason: str = "EXT") -> None:
        self._send("power" if reason == "POR" else "reset")

    def snapshot(self) -> dict:
        with self._lock:
            return {
                "pot": self.pot,
                "ldr": self.ldr,
                "temp_c": self.temp_c,
                # Jasność odczuwana z wypełnienia PWM (firmware stosuje korekcję gamma 2)
                "led": round(math.sqrt(self.duty) * 100),
                "duty": self.duty,
                "status": self.status,
            }

    def close(self) -> None:
        if self._proc.poll() is None:
            try:
                self._send("quit")
                self._proc.wait(timeout=2)
            except (AvrSimError, OSError, subprocess.TimeoutExpired):
                self._proc.kill()
                self._proc.wait()
