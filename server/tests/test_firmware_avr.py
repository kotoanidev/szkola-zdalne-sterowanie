"""Testy prawdziwego firmware (firmware/build/firmware.hex) uruchomionego w simavr.

Wymagają zbudowanego symulatora: make -C sim. Bez niego są pomijane.
"""

import re
import time

import pytest
import serial

from app import create_app
from app.avrsim import DEFAULT_BINARY, DEFAULT_HEX, AvrSim
from app.config import Config
from app.device import Device
from app.transport import SerialTransport

pytestmark = pytest.mark.skipif(
    not (DEFAULT_BINARY.exists() and DEFAULT_HEX.exists()),
    reason="brak sim/build/avrsim lub firmware.hex – uruchom: make -C sim",
)


class Link:
    """Terminal szeregowy podłączony do UART emulowanego ATmega8."""

    def __init__(self, port: str) -> None:
        self.port = serial.Serial(port, 9600, timeout=0.05)
        self.events: list[str] = []

    def read_line(self, timeout: float) -> str | None:
        deadline = time.monotonic() + timeout
        data = b""
        while time.monotonic() < deadline:
            data += self.port.readline()
            if data.endswith(b"\n"):
                return data.decode("ascii").strip()
        return None

    def response(self, timeout: float = 1.0) -> str:
        deadline = time.monotonic() + timeout
        while (line := self.read_line(deadline - time.monotonic())) is not None:
            if line.startswith("EVT"):
                self.events.append(line)
            else:
                return line
        raise AssertionError("brak odpowiedzi mikrokontrolera")

    def cmd(self, text: str) -> str:
        self.port.write(text.encode("ascii") + b"\n")
        return self.response()

    def raw(self, data: bytes) -> str:
        self.port.write(data)
        return self.response()

    def wait_event(self, prefix: str, timeout: float = 1.0) -> str:
        deadline = time.monotonic() + timeout
        while True:
            for i, line in enumerate(self.events):
                if line.startswith(prefix):
                    return self.events.pop(i)
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise AssertionError(f"brak zdarzenia {prefix}")
            line = self.read_line(remaining)
            if line is not None:
                self.events.append(line)


def wait_for(predicate, timeout: float = 2.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return True
        time.sleep(0.02)
    return False


@pytest.fixture
def avr(tmp_path):
    sim = AvrSim(eeprom=tmp_path / "eeprom.bin")
    yield sim
    sim.close()


@pytest.fixture
def link(avr):
    link = Link(avr.pty)
    assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=POR ver=0.9.0"
    yield link
    link.port.close()


def test_commands(link):
    assert link.cmd("PING") == "PONG"
    assert link.cmd("ver") == "VER 0.9.0"
    assert link.cmd("ON") == "OK on=1"
    assert link.cmd("TOGGLE") == "OK on=0"
    assert link.cmd(" pwm 60 ") == "OK pwm=60"
    assert link.cmd("MODE AUTO") == "OK mode=AUTO"
    assert link.cmd("PWM 40") == "OK pwm=40 mode=MANUAL"
    assert link.cmd("TLIM 30") == "OK tlim=30"


def test_compat_single_chars(link):
    assert link.raw(b"1") == "OK on=1"
    assert link.raw(b"0") == "OK on=0"


def test_errors(link):
    assert link.cmd("FOO") == "ERR UNKNOWN"
    assert link.cmd("PWM") == "ERR ARG"
    assert link.cmd("PWM 101") == "ERR ARG"
    assert link.cmd("MODE X") == "ERR ARG"
    assert link.cmd("TLIM 126") == "ERR ARG"
    assert link.cmd("PING 1") == "ERR ARG"
    assert link.cmd("A" * 32) == "ERR UNKNOWN"
    assert link.cmd("A" * 33) == "ERR TOOLONG"


def test_get_state(link):
    line = link.cmd("GET")
    assert re.fullmatch(r"STATE on=0 pwm=50 mode=MANUAL pot=512 ldr=500 temp=22\.0 "
                        r"alarm=0 tlim=50 up=\d+", line), line


def test_pwm_fade_and_gamma(link, avr):
    link.cmd("PWM 100")
    link.cmd("ON")
    time.sleep(0.15)
    assert 0 < avr.duty < 1, "jasność ma rosnąć płynnie"
    assert wait_for(lambda: avr.duty == 1.0)
    link.cmd("PWM 50")
    assert wait_for(lambda: abs(avr.duty - 256 / 1023) < 1e-3), avr.duty
    link.cmd("OFF")
    assert wait_for(lambda: avr.duty == 0)


def test_button_short_and_long(link, avr):
    avr.press_button()
    assert link.wait_event("EVT BTN") == "EVT BTN on=1"
    avr.press_button(long=True)
    assert link.wait_event("EVT BTN", 3) == "EVT BTN mode=AUTO"


def test_potentiometer_event(link, avr):
    time.sleep(0.2)  # pierwszy odczyt po starcie to punkt odniesienia, nie zmiana
    avr.set_inputs(pot=1023)
    assert link.wait_event("EVT POT") == "EVT POT pwm=100"


def test_auto_mode_follows_light(link, avr):
    link.cmd("ON")
    link.cmd("MODE AUTO")
    for ldr, pwm in ((100, 100), (900, 0), (500, 50)):
        avr.set_inputs(ldr=ldr)
        assert wait_for(lambda: f"pwm={pwm} " in link.cmd("GET")), (ldr, pwm)


def test_temperature_alarm(link, avr):
    link.cmd("TLIM 30")
    avr.set_inputs(temp_c=35)
    assert re.fullmatch(r"EVT TEMP alarm=1 temp=3\d\.\d", link.wait_event("EVT TEMP"))
    avr.set_inputs(temp_c=20)
    assert re.fullmatch(r"EVT TEMP alarm=0 temp=\d+\.\d", link.wait_event("EVT TEMP"))


def test_state_survives_power_loss(link, avr):
    link.cmd("ON")
    link.cmd("PWM 70")
    link.cmd("TLIM 40")
    time.sleep(2.6)  # zapis do EEPROM 2 s po ostatniej zmianie
    avr.reset("POR")
    assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=POR ver=0.9.0"
    state = link.cmd("GET")
    assert "on=1 pwm=70" in state and "tlim=40" in state


def test_external_reset_reason(link, avr):
    avr.reset()
    assert link.wait_event("EVT BOOT", 2) == "EVT BOOT reset=EXT ver=0.9.0"


def test_server_end_to_end(avr):
    dev = Device(SerialTransport(avr.pty), poll_interval=0.2, simulated="avr")
    dev.start()
    try:
        assert dev.wait_connected(5)
        client = create_app(Config(simulate="avr"), device=dev, sim=avr).test_client()
        assert client.get("/on").json["on"] is True
        assert client.post("/api/state", json={"pwm": 100}).json["pwm"] == 100
        assert wait_for(lambda: avr.duty == 1.0)
        avr.press_button()
        assert wait_for(lambda: dev.snapshot()["on"] is False)
        assert client.get("/api/health").json["firmware"] == "0.9.0"
    finally:
        dev.stop()
