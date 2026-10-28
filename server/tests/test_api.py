import json
import time


def wait_for(predicate, timeout=2.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return True
        time.sleep(0.02)
    return False


def test_on_off_compat(client, device):
    _, sim = device
    r = client.get("/on")
    assert r.status_code == 200
    assert r.json["on"] is True and sim.on
    r = client.get("/off")
    assert r.json["on"] is False and not sim.on


def test_get_state(client):
    r = client.get("/api/state?refresh=1")
    assert r.status_code == 200
    data = r.json
    for key in ("connected", "on", "pwm", "mode", "pot", "ldr", "temp", "alarm", "tlim", "uptime_s"):
        assert key in data
    assert data["connected"] is True
    assert data["pot"] == 512


def test_post_state(client, device):
    _, sim = device
    r = client.post("/api/state", json={"on": True, "pwm": 60, "tlim": 45})
    assert r.status_code == 200
    assert (r.json["on"], r.json["pwm"], r.json["tlim"]) == (True, 60, 45)
    assert (sim.on, sim.pwm, sim.tlim) == (True, 60, 45)
    r = client.post("/api/state", json={"mode": "auto"})
    assert r.json["mode"] == "AUTO"


def test_post_state_validation(client):
    assert client.post("/api/state", json={"pwm": 101}).status_code == 400
    assert client.post("/api/state", json={"pwm": "60"}).status_code == 400
    assert client.post("/api/state", json={"on": 1}).status_code == 400
    assert client.post("/api/state", json={"mode": "FAST"}).status_code == 400
    assert client.post("/api/state", json={"color": "red"}).status_code == 400
    assert client.post("/api/state", data="xyz").status_code == 400
    assert client.post("/api/state", json={}).status_code == 400


def test_health(client):
    r = client.get("/api/health")
    assert r.json["ok"] and r.json["connected"] and r.json["simulated"]
    assert r.json["firmware"].startswith("0.9.0")


def test_local_button_reaches_state(client, device):
    dev, sim = device
    client.get("/off")
    sim.press_button()
    assert wait_for(lambda: dev.snapshot()["on"] is True)
    assert any(e["text"] == "EVT BTN on=1" for e in client.get("/api/log").json)


def test_sim_endpoints(client, device):
    dev, _ = device
    r = client.post("/api/sim", json={"ldr": 700, "temp": 23.5})
    assert r.status_code == 200 and r.json["ldr"] == 700
    assert wait_for(lambda: dev.snapshot()["ldr"] == 700 and dev.snapshot()["temp"] == 23.5)
    assert client.post("/api/sim", json={"button": "double"}).status_code == 400


def test_sse_init_and_updates(client):
    r = client.get("/api/events", buffered=False)
    assert r.mimetype == "text/event-stream"
    chunks = r.response
    first = next(chunks).decode()
    assert first.startswith("event: init\n")
    init = json.loads(first.split("data: ", 1)[1])
    assert init["simulated"] is True and "state" in init
    client.post("/api/state", json={"pwm": 33})
    for _ in range(20):
        chunk = next(chunks).decode()
        if chunk.startswith("event: state") and '"pwm": 33' in chunk:
            break
    else:
        raise AssertionError("brak zdarzenia state z pwm=33")
    r.close()


def test_offline_returns_503(offline_client):
    assert offline_client.get("/on").status_code == 503
    assert offline_client.post("/api/state", json={"on": True}).status_code == 503
    r = offline_client.get("/api/state")
    assert r.status_code == 200 and r.json["connected"] is False
    assert offline_client.get("/api/health").json["connected"] is False
    assert offline_client.get("/api/sim").status_code == 404
