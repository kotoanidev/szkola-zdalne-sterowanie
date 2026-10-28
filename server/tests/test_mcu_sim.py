"""Symulator musi zachowywać się jak firmware (docs/protokol.md)."""

from app.mcu_sim import McuSim, auto_percent, pot_percent


def send(sim, text):
    sim.feed(text)
    lines = []
    while (line := sim.read_line()) is not None:
        lines.append(line)
    return lines


def test_boot_event(clock):
    sim = McuSim(clock=clock)
    assert sim.read_line().startswith("EVT BOOT reset=POR ver=")


def test_compat_single_chars(sim):
    assert send(sim, "1") == ["OK on=1"]
    assert send(sim, "0\r\n") == ["OK on=0"]
    assert send(sim, "10") == ["OK on=1", "OK on=0"]


def test_basic_commands(sim):
    assert send(sim, "ping\n") == ["PONG"]
    assert send(sim, "VER\n")[0].startswith("VER ")
    assert send(sim, "on\n") == ["OK on=1"]
    assert send(sim, "TOGGLE\n") == ["OK on=0"]
    assert send(sim, "  pwm 60  \n") == ["OK pwm=60"]
    assert send(sim, "MODE AUTO\n") == ["OK mode=AUTO"]
    assert send(sim, "TLIM 30\n") == ["OK tlim=30"]


def test_errors(sim):
    assert send(sim, "FOO\n") == ["ERR UNKNOWN"]
    assert send(sim, "PWM\n") == ["ERR ARG"]
    assert send(sim, "PWM 101\n") == ["ERR ARG"]
    assert send(sim, "PWM -1\n") == ["ERR ARG"]
    assert send(sim, "MODE X\n") == ["ERR ARG"]
    assert send(sim, "TLIM 126\n") == ["ERR ARG"]
    assert send(sim, "PING 1\n") == ["ERR ARG"]
    assert send(sim, "A" * 32 + "\n") == ["ERR UNKNOWN"]
    assert send(sim, "A" * 33 + "\n") == ["ERR TOOLONG"]
    assert send(sim, "\n\n") == []


def test_pwm_in_auto_switches_to_manual(sim):
    send(sim, "MODE AUTO\n")
    assert send(sim, "PWM 40\n") == ["OK pwm=40 mode=MANUAL"]


def test_get_state_line(sim, clock):
    send(sim, "ON\nPWM 60\n")
    clock.advance(12.4)
    line = send(sim, "GET\n")[0]
    assert line == "STATE on=1 pwm=60 mode=MANUAL pot=512 ldr=500 temp=22.0 alarm=0 tlim=50 up=12"


def test_pot_event_with_hysteresis(sim, clock):
    sim.tick()  # odczyt bazowy
    sim.set_inputs(pot=pot_raw(51))
    clock.advance(0.05)
    sim.tick()
    assert send(sim, "") == []
    sim.set_inputs(pot=pot_raw(60))
    clock.advance(0.05)
    sim.tick()
    assert send(sim, "") == ["EVT POT pwm=60"]
    assert sim.pwm == 60


def test_pot_switches_auto_to_manual(sim, clock):
    send(sim, "MODE AUTO\n")
    sim.tick()
    sim.set_inputs(pot=0)
    clock.advance(0.05)
    sim.tick()
    assert send(sim, "") == ["EVT POT pwm=0 mode=MANUAL"]


def test_auto_mode_follows_light(sim, clock):
    send(sim, "ON\nMODE AUTO\n")
    sim.set_inputs(ldr=100)
    clock.advance(0.2)
    sim.tick()
    assert sim.pwm == 100
    sim.set_inputs(ldr=900)
    clock.advance(0.2)
    sim.tick()
    assert sim.pwm == 0
    assert auto_percent(500) == 50


def test_temperature_alarm_hysteresis(sim, clock):
    send(sim, "TLIM 30\n")
    sim.set_inputs(temp_c=31)
    clock.advance(0.5)
    sim.tick()
    assert send(sim, "") == ["EVT TEMP alarm=1 temp=31.0"]
    sim.set_inputs(temp_c=29)
    clock.advance(0.5)
    sim.tick()
    assert send(sim, "") == []
    sim.set_inputs(temp_c=27.5)
    clock.advance(0.5)
    sim.tick()
    assert send(sim, "") == ["EVT TEMP alarm=0 temp=27.5"]


def test_button(sim):
    sim.press_button()
    assert send(sim, "") == ["EVT BTN on=1"]
    sim.press_button(long=True)
    assert send(sim, "") == ["EVT BTN mode=AUTO"]


def test_eeprom_restore_after_reset(sim):
    send(sim, "ON\nPWM 70\nTLIM 40\n")
    sim.reset("WDT")
    assert send(sim, "") == ["EVT BOOT reset=WDT ver=0.9.0-sim"]
    assert (sim.on, sim.pwm, sim.tlim, sim.mode) == (True, 70, 40, "MANUAL")


def test_auto_brightness_not_saved(sim, clock):
    send(sim, "PWM 30\nMODE AUTO\n")
    sim.set_inputs(ldr=100)
    clock.advance(0.2)
    sim.tick()
    assert sim.pwm == 100
    sim.reset()
    assert (sim.mode, sim.pwm) == ("AUTO", 30)


def pot_raw(percent):
    return next(raw for raw in range(1024) if pot_percent(raw) == percent)
