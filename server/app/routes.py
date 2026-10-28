"""Strona WWW, API JSON i strumień SSE."""

from __future__ import annotations

import json
import queue
import time

from flask import Blueprint, Response, current_app, jsonify, request

from .device import Device, DeviceError, NotConnected

bp = Blueprint("main", __name__)

SSE_KEEPALIVE = 15


def _device() -> Device:
    return current_app.extensions["device"]


def _sim():
    return current_app.extensions.get("sim")


def _error(message: str, status: int):
    return jsonify({"error": message}), status


@bp.errorhandler(NotConnected)
def _not_connected(e):
    return _error(str(e), 503)


@bp.errorhandler(DeviceError)
def _device_error(e):
    return _error(str(e), 502)


@bp.errorhandler(ValueError)
def _bad_value(e):
    return _error(str(e), 400)


@bp.get("/")
def index():
    return current_app.send_static_file("index.html")


# Zgodność z instrukcją (etap 3): http://[IP_MALINY]:5000/on
@bp.get("/on")
def on():
    return jsonify(_device().set_state(on=True))


@bp.get("/off")
def off():
    return jsonify(_device().set_state(on=False))


@bp.get("/api/state")
def get_state():
    device = _device()
    if request.args.get("refresh") == "1":
        return jsonify(device.refresh())
    return jsonify(device.snapshot())


@bp.post("/api/state")
def post_state():
    data = request.get_json(silent=True)
    if not isinstance(data, dict) or not data:
        return _error("oczekiwano obiektu JSON, np. {\"on\": true}", 400)
    unknown = set(data) - {"on", "pwm", "mode", "tlim"}
    if unknown:
        return _error(f"nieznane pola: {', '.join(sorted(unknown))}", 400)
    changes = {}
    if "on" in data:
        if not isinstance(data["on"], bool):
            return _error("on musi być true albo false", 400)
        changes["on"] = data["on"]
    for key in ("pwm", "tlim"):
        if key in data:
            if not isinstance(data[key], int) or isinstance(data[key], bool):
                return _error(f"{key} musi być liczbą całkowitą", 400)
            changes[key] = data[key]
    if "mode" in data:
        if not isinstance(data["mode"], str):
            return _error("mode musi być tekstem", 400)
        changes["mode"] = data["mode"].upper()
    return jsonify(_device().set_state(**changes))


@bp.get("/api/health")
def health():
    device = _device()
    return jsonify({
        "ok": True,
        "connected": device.connected,
        "simulated": bool(device.simulated),
        "simulator": device.simulated or None,
        "port": device.transport.port,
        "firmware": device.firmware,
        "server_uptime_s": int(time.time() - device.started_at),
    })


@bp.get("/api/log")
def get_log():
    return jsonify(_device().recent_log())


def _sse(event: str, data) -> str:
    return f"event: {event}\ndata: {json.dumps(data, ensure_ascii=False)}\n\n"


@bp.get("/api/events")
def events():
    device = _device()
    q = device.subscribe()
    sim = _sim()

    def stream():
        try:
            yield _sse("init", {
                "state": device.snapshot(),
                "log": device.recent_log(),
                "simulated": bool(device.simulated),
                "simulator": device.simulated or None,
                "firmware": device.firmware,
                "sim": sim.snapshot() if sim else None,
            })
            while True:
                try:
                    kind, data = q.get(timeout=SSE_KEEPALIVE)
                except queue.Empty:
                    yield ": keepalive\n\n"
                    continue
                yield _sse(kind, data)
        finally:
            device.unsubscribe(q)

    return Response(stream(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"})


# --- wirtualna płytka, tylko w trybie symulacji ------------------------------

@bp.get("/api/sim")
def get_sim():
    sim = _sim()
    if sim is None:
        return _error("serwer nie działa w trybie symulacji", 404)
    return jsonify(sim.snapshot())


@bp.post("/api/sim")
def post_sim():
    sim = _sim()
    if sim is None:
        return _error("serwer nie działa w trybie symulacji", 404)
    data = request.get_json(silent=True)
    if not isinstance(data, dict):
        return _error("oczekiwano obiektu JSON", 400)
    try:
        sim.set_inputs(pot=data.get("pot"), ldr=data.get("ldr"), temp_c=data.get("temp"))
    except (TypeError, ValueError):
        return _error("pot i ldr: 0–1023, temp: liczba", 400)
    button = data.get("button")
    if button in ("short", "long"):
        sim.press_button(long=button == "long")
    elif button is not None:
        return _error("button: short albo long", 400)
    if data.get("reset"):
        sim.reset("POR" if data.get("reset") == "power" else "EXT")
    _device().request_refresh()
    return jsonify(sim.snapshot())
