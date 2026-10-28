"""Łącze z mikrokontrolerem: prawdziwy port szeregowy albo symulator."""

from __future__ import annotations

import time

import serial
from serial.tools import list_ports

from .mcu_sim import McuSim

BAUD = 9600

# VID:PID popularnych konwerterów USB-UART (docs/sprzet.md)
KNOWN_CONVERTERS = {
    (0x1A86, 0x7523): "CH340",
    (0x10C4, 0xEA60): "CP2102",
    (0x0403, 0x6001): "FT232R",
    (0x067B, 0x2303): "PL2303",
}


class TransportError(Exception):
    pass


def find_converter() -> str | None:
    for port in sorted(list_ports.comports(), key=lambda p: p.device):
        if (port.vid, port.pid) in KNOWN_CONVERTERS:
            return port.device
    return None


class SerialTransport:
    def __init__(self, port: str | None = None, baud: int = BAUD) -> None:
        self.requested_port = port
        self.baud = baud
        self.port: str | None = None
        self._serial: serial.Serial | None = None
        self._buffer = b""

    @property
    def description(self) -> str:
        return self.port or self.requested_port or "auto"

    def open(self) -> None:
        port = self.requested_port or find_converter()
        if port is None:
            raise TransportError("nie znaleziono konwertera USB-UART")
        try:
            self._serial = serial.Serial(port, self.baud, timeout=0.1, write_timeout=1)
            self._serial.reset_input_buffer()
        except (serial.SerialException, OSError) as e:
            raise TransportError(str(e)) from e
        self.port = port
        self._buffer = b""

    def close(self) -> None:
        if self._serial is not None:
            try:
                self._serial.close()
            except (serial.SerialException, OSError):
                pass
        self._serial = None

    def write_line(self, line: str) -> None:
        if self._serial is None:
            raise TransportError("port zamknięty")
        try:
            self._serial.write(line.encode("ascii") + b"\n")
        except (serial.SerialException, OSError) as e:
            raise TransportError(str(e)) from e

    def read_line(self, timeout: float) -> str | None:
        if self._serial is None:
            raise TransportError("port zamknięty")
        deadline = time.monotonic() + timeout
        while b"\n" not in self._buffer:
            try:
                chunk = self._serial.read(max(1, self._serial.in_waiting))
            except (serial.SerialException, OSError, TypeError) as e:
                raise TransportError(str(e)) from e
            if chunk:
                self._buffer += chunk
            elif time.monotonic() >= deadline:
                return None
        line, _, self._buffer = self._buffer.partition(b"\n")
        return line.decode("ascii", errors="replace").strip()


class SimTransport:
    """Symulator podpięty jak port szeregowy; zadania okresowe MCU wykonuje przy odczycie."""

    port = "symulator"
    description = "symulator"

    def __init__(self, sim: McuSim) -> None:
        self.sim = sim

    def open(self) -> None:
        pass

    def close(self) -> None:
        pass

    def write_line(self, line: str) -> None:
        self.sim.feed(line + "\n")

    def read_line(self, timeout: float) -> str | None:
        deadline = time.monotonic() + timeout
        while True:
            self.sim.tick()
            line = self.sim.read_line(timeout=min(0.05, max(0.0, deadline - time.monotonic())))
            if line is not None or time.monotonic() >= deadline:
                return line
