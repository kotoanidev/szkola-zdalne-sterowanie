"""Jedyny obiekt obsługujący łącze z mikrokontrolerem.

Wątek odczytu rozdziela zdarzenia EVT od odpowiedzi na komendy, a wątek nadzorcy
łączy się (i ponownie łączy) z mikrokontrolerem oraz co chwilę odczytuje stan komendą GET.
"""

from __future__ import annotations

import logging
import queue
import threading
import time
from collections import deque
from typing import Callable, Protocol

from .protocol import MODES, PWM_MAX, TLIM_MAX, Message, fields_to_state, parse_line
from .transport import TransportError

log = logging.getLogger(__name__)

RESPONSE_TIMEOUT = 0.5
RECONNECT_INTERVAL = 2.0
LOG_SIZE = 50


class Transport(Protocol):
    port: str | None
    description: str

    def open(self) -> None: ...
    def close(self) -> None: ...
    def write_line(self, line: str) -> None: ...
    def read_line(self, timeout: float) -> str | None: ...


class DeviceError(Exception):
    """Błąd po stronie mikrokontrolera (ERR ...) – HTTP 502."""


class NotConnected(Exception):
    """Brak połączenia lub odpowiedzi mikrokontrolera – HTTP 503."""


class Device:
    def __init__(self, transport: Transport, poll_interval: float = 1.0,
                 simulated: str = "", clock: Callable[[], float] = time.time) -> None:
        self.transport = transport
        self.poll_interval = poll_interval
        self.simulated = simulated
        self._clock = clock
        self._cmd_lock = threading.Lock()
        self._state_lock = threading.Lock()
        self._responses: queue.Queue[Message] = queue.Queue()
        self._subscribers: set[queue.Queue] = set()
        self._log: deque[dict] = deque(maxlen=LOG_SIZE)
        self._link_open = threading.Event()
        self._connected = threading.Event()
        self._refresh = threading.Event()
        self._stop = threading.Event()
        self._threads: list[threading.Thread] = []
        self._state: dict = {
            "connected": False,
            "on": False,
            "pwm": 0,
            "mode": "MANUAL",
            "pot": 0,
            "ldr": 0,
            "temp": None,
            "alarm": False,
            "tlim": 0,
            "uptime_s": 0,
        }
        self.firmware: str | None = None
        self.started_at = self._clock()

    # --- cykl życia ---------------------------------------------------------

    def start(self) -> None:
        for target, name in ((self._reader, "uart-reader"), (self._supervisor, "uart-supervisor")):
            t = threading.Thread(target=target, name=name, daemon=True)
            t.start()
            self._threads.append(t)

    def stop(self) -> None:
        self._stop.set()
        for t in self._threads:
            t.join(timeout=2)
        self.transport.close()

    @property
    def connected(self) -> bool:
        return self._connected.is_set()

    def wait_connected(self, timeout: float) -> bool:
        return self._connected.wait(timeout)

    # --- stan, dziennik, subskrypcje SSE --------------------------------------

    def snapshot(self) -> dict:
        with self._state_lock:
            return dict(self._state)

    def recent_log(self) -> list[dict]:
        with self._state_lock:
            return list(self._log)

    def subscribe(self) -> queue.Queue:
        q: queue.Queue = queue.Queue(maxsize=100)
        with self._state_lock:
            self._subscribers.add(q)
        return q

    def unsubscribe(self, q: queue.Queue) -> None:
        with self._state_lock:
            self._subscribers.discard(q)

    def _publish(self, kind: str, data: dict) -> None:
        with self._state_lock:
            subscribers = list(self._subscribers)
        for q in subscribers:
            try:
                q.put_nowait((kind, data))
            except queue.Full:
                pass

    def publish_sim(self, snapshot: dict) -> None:
        self._publish("sim", snapshot)

    def _update(self, changes: dict) -> None:
        with self._state_lock:
            new = {**self._state, **changes}
            if new == self._state:
                return
            self._state = new
            snapshot = dict(new)
        self._publish("state", snapshot)

    def _add_log(self, text: str, source: str) -> None:
        entry = {"t": self._clock(), "text": text, "source": source}
        with self._state_lock:
            self._log.append(entry)
        self._publish("log", entry)

    # --- komendy -----------------------------------------------------------

    def command(self, line: str, timeout: float = RESPONSE_TIMEOUT) -> Message:
        if not self._link_open.is_set():
            raise NotConnected("brak połączenia z mikrokontrolerem")
        with self._cmd_lock:
            while not self._responses.empty():
                self._responses.get_nowait()
            try:
                self.transport.write_line(line)
            except TransportError as e:
                self._disconnect(str(e))
                raise NotConnected(str(e)) from e
            try:
                msg = self._responses.get(timeout=timeout)
            except queue.Empty:
                raise NotConnected(f"brak odpowiedzi na {line}") from None
        if msg.kind == "ERR":
            raise DeviceError(f"{line}: ERR {msg.name}")
        if msg.kind in ("OK", "STATE"):
            self._update(fields_to_state(msg.fields))
        return msg

    def refresh(self) -> dict:
        self.command("GET")
        return self.snapshot()

    def request_refresh(self) -> None:
        self._refresh.set()

    def set_state(self, on: bool | None = None, pwm: int | None = None,
                  mode: str | None = None, tlim: int | None = None) -> dict:
        """Zmienia stan; kolejność: on, pwm, mode, tlim (tryb AUTO po PWM ma pierwszeństwo)."""
        if pwm is not None and not 0 <= pwm <= PWM_MAX:
            raise ValueError("pwm poza zakresem 0–100")
        if mode is not None and mode not in MODES:
            raise ValueError("mode musi być MANUAL albo AUTO")
        if tlim is not None and not 0 <= tlim <= TLIM_MAX:
            raise ValueError(f"tlim poza zakresem 0–{TLIM_MAX}")
        commands = []
        if on is not None:
            commands.append("ON" if on else "OFF")
        if pwm is not None:
            commands.append(f"PWM {pwm}")
        if mode is not None:
            commands.append(f"MODE {mode}")
        if tlim is not None:
            commands.append(f"TLIM {tlim}")
        for line in commands:
            self.command(line)
            self._add_log(line, "web")
        return self.snapshot()

    # --- wątki ---------------------------------------------------------------

    def _reader(self) -> None:
        while not self._stop.is_set():
            if not self._link_open.is_set():
                self._stop.wait(0.05)
                continue
            try:
                line = self.transport.read_line(0.2)
            except TransportError as e:
                self._disconnect(str(e))
                continue
            if not line:
                continue
            msg = parse_line(line)
            if msg.is_event:
                self._handle_event(msg, line)
            elif msg.kind:
                self._responses.put(msg)

    def _handle_event(self, msg: Message, line: str) -> None:
        self._add_log(line, "mcu")
        if msg.name == "BOOT":
            self.firmware = msg.fields.get("ver", self.firmware)
            self._refresh.set()
        else:
            self._update(fields_to_state(msg.fields))
            self._refresh.set()

    def _supervisor(self) -> None:
        next_poll = 0.0
        while not self._stop.is_set():
            if not self.connected:
                self._try_connect()
                if not self.connected:
                    self._stop.wait(RECONNECT_INTERVAL)
                    continue
                next_poll = 0.0
            now = time.monotonic()
            if self._refresh.is_set() or now >= next_poll:
                self._refresh.clear()
                next_poll = now + self.poll_interval
                try:
                    self.command("GET")
                except NotConnected as e:
                    log.warning("GET: %s", e)
                except DeviceError as e:
                    log.warning("%s", e)
            self._refresh.wait(max(0.0, next_poll - time.monotonic()))

    def _try_connect(self) -> None:
        try:
            self.transport.open()
        except TransportError as e:
            log.debug("połączenie: %s", e)
            return
        self._link_open.set()
        # Pierwsza linia po otwarciu portu bywa uszkodzona, więc PING ponawiamy
        for _ in range(3):
            try:
                if self.command("PING").kind == "PONG":
                    break
            except (NotConnected, DeviceError):
                continue
        else:
            self._disconnect("mikrokontroler nie odpowiada na PING")
            return
        try:
            ver = self.command("VER")
            self.firmware = ver.name or self.firmware
        except (NotConnected, DeviceError):
            pass
        log.info("połączono: %s (firmware %s)", self.transport.port, self.firmware)
        self._add_log(f"Połączono z mikrokontrolerem ({self.transport.port})", "server")
        self._update({"connected": True})
        self._connected.set()

    def _disconnect(self, reason: str) -> None:
        if not self._link_open.is_set():
            return
        self._link_open.clear()
        was_ready = self._connected.is_set()
        self._connected.clear()
        self.transport.close()
        if not was_ready:
            log.info("nieudane połączenie: %s", reason)
            return
        log.warning("rozłączono: %s", reason)
        self._add_log(f"Utracono połączenie: {reason}", "server")
        self._update({"connected": False})
