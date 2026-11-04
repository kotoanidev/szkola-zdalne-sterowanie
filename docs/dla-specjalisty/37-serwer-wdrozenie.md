# Serwer: pliki uruchomieniowe i zależności

[← Poprzedni](36-serwer-avrsim.md) · [Spis treści](README.md) · [Następny →](40-www-index-html.md)

**Po co te pliki:** nie zawierają logiki programu. Mówią systemowi Raspberry Pi, jak uruchamiać serwer przy starcie, narzędziu `pytest`, gdzie szukać testów, a instalatorowi `pip`, jakich bibliotek potrzeba.

## `server/deploy/zdalne-sterowanie@.service`

[Otwórz ten plik w repozytorium](../../server/deploy/zdalne-sterowanie@.service)

Opis usługi dla **systemd**, czyli części Linuksa, która uruchamia programy działające w tle. Dzięki temu serwer sam startuje po włączeniu Raspberry Pi. Znak `@` w nazwie oznacza usługę z parametrem: przy starcie podaje się nazwę użytkownika, a w pliku dostępna jest ona jako `%i`.

```ini
  1  # Autostart serwera na Raspberry Pi (R10). %i = użytkownik, w którego katalogu domowym
  2  # jest repozytorium. Instalacja (README, „Serwer na Raspberry Pi”):
  3  #   sudo cp deploy/zdalne-sterowanie@.service /etc/systemd/system/
  4  #   sudo systemctl daemon-reload
  5  #   sudo systemctl enable --now zdalne-sterowanie@$USER
  6  # Zmienne (SERIAL_PORT, PORT, POLL_INTERVAL) można ustawić w /etc/default/zdalne-sterowanie.
```

- **Linia 1:** Komentarz: autostart serwera na Raspberry Pi (rozszerzenie R10 z planu projektu). `%i` to użytkownik, w którego katalogu domowym jest repozytorium.
- **Linia 2:** Komentarz: dalej opis instalacji, także w README.
- **Linia 3:** Komentarz, krok 1: skopiuj ten plik do katalogu usług systemowych. `sudo` oznacza „jako administrator”.
- **Linia 4:** Komentarz, krok 2: każ systemowi ponownie wczytać listę usług.
- **Linia 5:** Komentarz, krok 3: włącz autostart i od razu uruchom usługę. `$USER` to nazwa bieżącego użytkownika.
- **Linia 6:** Komentarz: dodatkowe zmienne można ustawić w pliku `/etc/default/zdalne-sterowanie`.

```ini
  8  [Unit]
  9  Description=Zdalne sterowanie LED - serwer Flask
 10  After=network-online.target
 11  Wants=network-online.target
```

- **Linia 8:** Początek sekcji `[Unit]`, czyli ogólnego opisu usługi.
- **Linia 9:** Czytelna nazwa usługi.
- **Linia 10:** Uruchom usługę dopiero po starcie sieci.
- **Linia 11:** Poproś system o uruchomienie mechanizmu czekania na sieć.

```ini
 13  [Service]
 14  Type=simple
 15  User=%i
 16  SupplementaryGroups=dialout
 17  WorkingDirectory=/home/%i/szkola-zdalne-sterowanie/server
 18  Environment=PYTHONUNBUFFERED=1
 19  EnvironmentFile=-/etc/default/zdalne-sterowanie
 20  ExecStart=/home/%i/szkola-zdalne-sterowanie/server/.venv/bin/python -m app
 21  Restart=on-failure
 22  RestartSec=3
```

- **Linia 13:** Początek sekcji `[Service]`, która mówi, jak uruchomić program.
- **Linia 14:** Prosty typ: uruchomiony program to już działająca usługa.
- **Linia 15:** Uruchom jako użytkownik podany w nazwie usługi, a nie jako administrator.
- **Linia 16:** Dodaj go do grupy `dialout`, która w Linuksie ma dostęp do portów szeregowych.
- **Linia 17:** Katalog roboczy programu.
- **Linia 18:** Ustaw zmienną środowiskową: Python ma wypisywać komunikaty od razu, bez buforowania.
- **Linia 19:** Wczytaj dodatkowe zmienne z pliku. Minus oznacza, że brak pliku nie jest błędem.
- **Linia 20:** Polecenie uruchomienia: Python ze środowiska `.venv` uruchamia moduł `app`.
- **Linia 21:** Uruchom ponownie, jeśli program zakończy się błędem.
- **Linia 22:** Odczekaj 3 s przed ponownym uruchomieniem.

```ini
 24  [Install]
 25  WantedBy=multi-user.target
```

- **Linia 24:** Początek sekcji `[Install]`, która mówi, kiedy usługa ma startować.
- **Linia 25:** Startuj wraz z normalnym, wielodostępnym trybem pracy systemu.

## `server/pytest.ini`

[Otwórz ten plik w repozytorium](../../server/pytest.ini)

Ustawienia narzędzia do testów.

```ini
  1  [pytest]
  2  testpaths = tests
  3  pythonpath = .
```

- **Linia 1:** Początek sekcji ustawień pytest.
- **Linia 2:** Testów szukaj w katalogu `tests`.
- **Linia 3:** Dodaj bieżący katalog do ścieżki importów, by testy mogły napisać `from app import ...`.

## `server/requirements.txt`

[Otwórz ten plik w repozytorium](../../server/requirements.txt)

Lista bibliotek potrzebnych do uruchomienia serwera. Instaluje się je poleceniem `pip install -r requirements.txt`.

```text
  1  Flask>=3.0
  2  pyserial>=3.5
```

- **Linia 1:** Flask w wersji 3.0 lub nowszej. To biblioteka serwera WWW.
- **Linia 2:** pyserial w wersji 3.5 lub nowszej. To biblioteka obsługi portu szeregowego.

## `server/requirements-dev.txt`

[Otwórz ten plik w repozytorium](../../server/requirements-dev.txt)

Lista dodatkowych bibliotek dla osób rozwijających program.

```text
  1  -r requirements.txt
  2  pytest>=8
```

- **Linia 1:** Zainstaluj też wszystko z `requirements.txt`.
- **Linia 2:** pytest w wersji 8 lub nowszej, czyli narzędzie do testów.

---

[← Poprzedni](36-serwer-avrsim.md) · [Spis treści](README.md) · [Następny →](40-www-index-html.md)
