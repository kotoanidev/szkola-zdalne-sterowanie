"""Konfiguracja wyłącznie ze zmiennych środowiskowych."""

from __future__ import annotations

import os
from dataclasses import dataclass


def _simulator(value: str) -> str:
    """SIMULATE=1 – symulator w Pythonie, SIMULATE=avr – prawdziwy firmware w simavr."""
    value = value.strip().lower()
    if value == "avr":
        return "avr"
    return "python" if value in ("1", "true", "tak", "yes", "on", "python") else ""


@dataclass
class Config:
    host: str = "0.0.0.0"
    port: int = 5000
    serial_port: str | None = None
    simulate: str = ""
    poll_interval: float = 1.0

    @classmethod
    def from_env(cls) -> "Config":
        return cls(
            host=os.environ.get("HOST", cls.host),
            port=int(os.environ.get("PORT", cls.port)),
            serial_port=os.environ.get("SERIAL_PORT") or None,
            simulate=_simulator(os.environ.get("SIMULATE", "")),
            poll_interval=float(os.environ.get("POLL_INTERVAL", cls.poll_interval)),
        )
