"""Parser linii protokołu UART (docs/protokol.md)."""

from __future__ import annotations

from dataclasses import dataclass, field

MODES = ("MANUAL", "AUTO")
PWM_MAX = 100
TLIM_MAX = 125
LINE_MAX = 32


@dataclass
class Message:
    kind: str
    name: str = ""
    fields: dict[str, str] = field(default_factory=dict)

    @property
    def is_event(self) -> bool:
        return self.kind == "EVT"


def parse_line(line: str) -> Message:
    parts = line.strip().split()
    if not parts:
        return Message("")
    kind = parts[0].upper()
    rest = parts[1:]
    name = ""
    # EVT, ERR i VER mają nazwę przed parami klucz=wartość
    if kind in ("EVT", "ERR", "VER") and rest and "=" not in rest[0]:
        name = rest[0]
        rest = rest[1:]
    fields = {}
    for item in rest:
        key, sep, value = item.partition("=")
        if sep:
            fields[key.lower()] = value
    return Message(kind, name, fields)


def fields_to_state(fields: dict[str, str]) -> dict:
    """Zamienia pola z OK / STATE / EVT na wartości stanu w formacie API."""
    state: dict = {}
    for key, value in fields.items():
        try:
            if key in ("on", "alarm"):
                state[key] = value == "1"
            elif key in ("pwm", "pot", "ldr", "tlim"):
                state[key] = int(value)
            elif key == "temp":
                state[key] = float(value)
            elif key == "up":
                state["uptime_s"] = int(value)
            elif key == "mode" and value in MODES:
                state[key] = value
        except ValueError:
            continue
    return state
