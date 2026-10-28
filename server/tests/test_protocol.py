from app.protocol import fields_to_state, parse_line


def test_parse_ok():
    msg = parse_line("OK pwm=60 mode=MANUAL\r")
    assert msg.kind == "OK"
    assert msg.fields == {"pwm": "60", "mode": "MANUAL"}
    assert not msg.is_event


def test_parse_event_with_name():
    msg = parse_line("EVT BTN on=0")
    assert msg.is_event
    assert msg.name == "BTN"
    assert msg.fields == {"on": "0"}


def test_parse_err_and_ver():
    assert parse_line("ERR ARG").name == "ARG"
    assert parse_line("VER 0.9.0").name == "0.9.0"
    assert parse_line("PONG").kind == "PONG"
    assert parse_line("   ").kind == ""


def test_state_conversion():
    msg = parse_line("STATE on=1 pwm=60 mode=AUTO pot=512 ldr=301 temp=-3.5 alarm=0 tlim=50 up=1234")
    assert fields_to_state(msg.fields) == {
        "on": True, "pwm": 60, "mode": "AUTO", "pot": 512, "ldr": 301,
        "temp": -3.5, "alarm": False, "tlim": 50, "uptime_s": 1234,
    }


def test_state_conversion_skips_garbage():
    assert fields_to_state({"pwm": "x", "mode": "FOO", "temp": "2.5"}) == {"temp": 2.5}
