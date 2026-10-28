"""Uruchomienie: python -m app (konfiguracja w zmiennych środowiskowych, patrz README)."""

import logging

from . import create_app
from .config import Config


def main() -> None:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    config = Config.from_env()
    app = create_app(config)
    mode = "symulacja" if config.simulate else (config.serial_port or "autowykrywanie portu")
    logging.getLogger("app").info("start na %s:%d (%s)", config.host, config.port, mode)
    app.run(host=config.host, port=config.port, threaded=True, use_reloader=False)


if __name__ == "__main__":
    main()
