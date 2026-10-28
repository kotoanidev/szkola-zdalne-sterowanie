"""Serwer Flask: mostek między przeglądarką a ATmega8 (docs/protokol.md, rozdz. 5)."""

from __future__ import annotations

import atexit

from flask import Flask

from .config import Config
from .device import Device
from .mcu_sim import McuSim
from .transport import SerialTransport, SimTransport


def build_device(config: Config) -> tuple[Device, object | None]:
    if config.simulate == "avr":
        from .avrsim import AvrSim

        avr = AvrSim()
        atexit.register(avr.close)
        device = Device(SerialTransport(avr.pty), config.poll_interval, simulated="avr")
        avr.on_change = device.publish_sim
        return device, avr
    if config.simulate:
        sim = McuSim()
        return Device(SimTransport(sim), config.poll_interval, simulated="python"), sim
    return Device(SerialTransport(config.serial_port), config.poll_interval), None


def create_app(config: Config | None = None, device: Device | None = None,
               sim: object | None = None) -> Flask:
    config = config or Config.from_env()
    if device is None:
        device, sim = build_device(config)
        device.start()

    app = Flask(__name__)
    app.json.ensure_ascii = False
    app.json.sort_keys = False
    app.extensions["device"] = device
    app.extensions["sim"] = sim

    from .routes import bp
    app.register_blueprint(bp)
    return app
