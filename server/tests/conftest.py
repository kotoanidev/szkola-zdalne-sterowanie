import pytest

from app import create_app
from app.config import Config
from app.device import Device
from app.mcu_sim import McuSim
from app.transport import SimTransport, TransportError


class FakeClock:
    def __init__(self) -> None:
        self.t = 1000.0

    def __call__(self) -> float:
        return self.t

    def advance(self, seconds: float) -> None:
        self.t += seconds


class DeadTransport:
    """Port, którego nie da się otworzyć (np. odłączony konwerter)."""

    port = None
    description = "brak"

    def open(self) -> None:
        raise TransportError("nie znaleziono konwertera USB-UART")

    def close(self) -> None:
        pass

    def write_line(self, line: str) -> None:
        raise TransportError("port zamknięty")

    def read_line(self, timeout: float):
        raise TransportError("port zamknięty")


@pytest.fixture
def clock():
    return FakeClock()


@pytest.fixture
def sim(clock):
    s = McuSim(clock=clock)
    s.read_line()  # EVT BOOT
    return s


@pytest.fixture
def device():
    sim = McuSim()
    dev = Device(SimTransport(sim), poll_interval=0.1, simulated=True)
    dev.start()
    assert dev.wait_connected(3)
    yield dev, sim
    dev.stop()


@pytest.fixture
def client(device):
    dev, sim = device
    app = create_app(Config(simulate=True), device=dev, sim=sim)
    app.testing = True
    return app.test_client()


@pytest.fixture
def offline_client():
    dev = Device(DeadTransport(), poll_interval=0.1)
    dev.start()
    app = create_app(Config(), device=dev)
    yield app.test_client()
    dev.stop()
