# Repozytorium: pliki konfiguracyjne

[← Poprzedni](72-narzedzia-ntc-table.md) · [Spis treści](README.md)

**Po co te pliki:** nie są częścią działającego programu. Mówią edytorom i gitowi, jak traktować pliki w repozytorium. Kropka na początku nazwy oznacza w Linuksie plik ukryty.

## `.editorconfig`

[Otwórz ten plik w repozytorium](../../.editorconfig)

Ustawienia dla edytorów tekstu, dzięki którym każdy zapisuje pliki w ten sam sposób.

```ini
  1  root = true
```

- **Linia 1:** To plik nadrzędny, więc edytor nie szuka podobnych w katalogach wyżej.

```ini
  3  [*]
  4  charset = utf-8
  5  end_of_line = lf
  6  insert_final_newline = true
  7  trim_trailing_whitespace = true
  8  indent_style = space
  9  indent_size = 4
```

- **Linia 3:** Sekcja dla wszystkich plików.
- **Linia 4:** Kodowanie znaków UTF-8.
- **Linia 5:** Koniec linii w stylu Linuksa, jeden znak LF.
- **Linia 6:** Plik ma kończyć się znakiem nowej linii.
- **Linia 7:** Usuwaj spacje na końcach linii.
- **Linia 8:** Wcięcia zapisuj spacjami.
- **Linia 9:** Jedno wcięcie to 4 spacje.

```ini
 11  [Makefile]
 12  indent_style = tab
```

- **Linia 11:** Sekcja dla plików `Makefile`.
- **Linia 12:** Wcięcia tabulatorem, bo Make tego wymaga.

```ini
 14  [*.{html,css,js,json,yml,yaml}]
 15  indent_size = 2
```

- **Linia 14:** Sekcja dla plików stron WWW i danych.
- **Linia 15:** Jedno wcięcie to 2 spacje.

```ini
 17  [*.md]
 18  trim_trailing_whitespace = false
```

- **Linia 17:** Sekcja dla plików Markdown.
- **Linia 18:** Nie usuwaj spacji na końcu linii, bo w Markdownie dwie spacje oznaczają złamanie wiersza.

```ini
 20  [*.ps1]
 21  end_of_line = crlf
```

- **Linia 20:** Sekcja dla skryptów PowerShella.
- **Linia 21:** Koniec linii w stylu Windows, dwa znaki CR i LF.

## `.gitattributes`

[Otwórz ten plik w repozytorium](../../.gitattributes)

Mówi gitowi, jak traktować poszczególne rodzaje plików.

```text
  1  # Domyslnie LF - kod serwera trafia na Raspberry Pi (Linux)
  2  * text=auto eol=lf
```

- **Linia 1:** Komentarz: domyślnie LF, bo kod serwera trafia na Raspberry Pi z Linuksem.
- **Linia 2:** Dla wszystkich plików: rozpoznaj sam, czy to tekst, i przechowuj go z końcem linii LF.

```text
  4  # Skrypty PowerShell dla Windows
  5  *.ps1 text eol=crlf
```

- **Linia 4:** Komentarz: skrypty PowerShella dla Windows.
- **Linia 5:** Pliki `.ps1` to tekst z końcem linii CRLF.

```text
  7  # Pliki binarne
  8  *.docx binary
  9  *.pdf binary
 10  *.png binary
 11  *.jpg binary
 12  *.hex binary
```

- **Linia 7:** Komentarz: pliki binarne.
- **Linia 8:** Pliki Worda są binarne, git ma ich nie zmieniać.
- **Linia 9:** Pliki PDF są binarne.
- **Linia 10:** Obrazy PNG są binarne.
- **Linia 11:** Obrazy JPG są binarne.
- **Linia 12:** Skompilowany firmware traktujemy jak plik binarny.

## `.gitignore`

[Otwórz ten plik w repozytorium](../../.gitignore)

Lista plików, które git ma pomijać: wyniki kompilacji, pliki tymczasowe, ustawienia prywatne. Gwiazdka `*` oznacza dowolny tekst, a ukośnik na końcu wskazuje katalog.

```text
  1  # Firmware (avr-gcc)
  2  firmware/build/
  3  *.o
  4  *.d
  5  *.elf
  6  *.hex
  7  *.eep
  8  *.lss
  9  *.map
```

- **Linia 1:** Komentarz: firmware.
- **Linia 2:** Katalog z wynikami kompilacji firmware.
- **Linia 3:** Pliki pośrednie kompilacji.
- **Linia 4:** Pliki zależności od nagłówków.
- **Linia 5:** Pliki programu z dodatkowymi informacjami.
- **Linia 6:** Pliki gotowe do wgrania.
- **Linia 7:** Dane EEPROM.
- **Linia 8:** Listingi, czyli kod źródłowy wymieszany z kodem maszynowym.
- **Linia 9:** Mapy pamięci.

```text
 11  # Symulator simavr (zależności i kompilacja tylko lokalnie)
 12  sim/.deps/
 13  sim/build/
```

- **Linia 11:** Komentarz: symulator buduje się tylko lokalnie.
- **Linia 12:** Pobrany kod simavr.
- **Linia 13:** Katalog z wynikiem kompilacji symulatora.

```text
 15  # Python
 16  __pycache__/
 17  *.py[cod]
 18  .venv/
 19  venv/
 20  .pytest_cache/
 21  .coverage
 22  htmlcov/
```

- **Linia 15:** Komentarz: Python.
- **Linia 16:** Katalogi z pamięcią podręczną Pythona.
- **Linia 17:** Pliki skompilowane Pythona. Nawias oznacza jeden ze znaków: `.pyc`, `.pyo` lub `.pyd`.
- **Linia 18:** Środowisko wirtualne, czyli lokalny katalog z bibliotekami.
- **Linia 19:** To samo, pod inną popularną nazwą.
- **Linia 20:** Pamięć podręczna pytest.
- **Linia 21:** Dane o pokryciu kodu testami.
- **Linia 22:** Raport pokrycia kodu testami.

```text
 24  # Lokalna konfiguracja i sekrety
 25  .env
 26  .env.*
 27  *.local.*
 28  .claude/settings.local.json
```

- **Linia 24:** Komentarz: lokalna konfiguracja i sekrety.
- **Linia 25:** Plik ze zmiennymi środowiskowymi, często z hasłami.
- **Linia 26:** Jego warianty.
- **Linia 27:** Pliki, które w nazwie mają `.local.`.
- **Linia 28:** Lokalne ustawienia Claude Code.

```text
 30  # Materialy prowadzacego - tylko lokalnie (prawa autorskie, nie publikujemy)
 31  docs/materialy/
```

- **Linia 30:** Komentarz: materiały prowadzącego są tylko lokalne ze względu na prawa autorskie.
- **Linia 31:** Katalog z tymi materiałami.

```text
 33  # Office / system
 34  ~$*
 35  *.tmp
 36  Thumbs.db
 37  desktop.ini
 38  .DS_Store
```

- **Linia 33:** Komentarz: pliki Office i systemowe.
- **Linia 34:** Pliki blokady tworzone przez Worda.
- **Linia 35:** Pliki tymczasowe.
- **Linia 36:** Miniatury obrazów w Windows.
- **Linia 37:** Ustawienia folderów w Windows.
- **Linia 38:** Ustawienia folderów w macOS.

```text
 40  # Edytory
 41  .vscode/*
 42  !.vscode/extensions.json
 43  !.vscode/c_cpp_properties.json
 44  !.vscode/tasks.json
 45  .idea/
```

- **Linia 40:** Komentarz: edytory.
- **Linia 41:** Pomiń zawartość katalogu VS Code...
- **Linia 42:** ...oprócz listy polecanych rozszerzeń. `!` oznacza wyjątek.
- **Linia 43:** Wyjątek: ustawienia podpowiadania kodu (IntelliSense) dla avr-gcc i ATmega32, wspólne dla obu komputerów.
- **Linia 44:** Wyjątek: zadanie budowania `Ctrl+Shift+B`, które uruchamia `make` w katalogu `firmware/`.
- **Linia 45:** Pomiń katalog edytorów JetBrains.

---

[← Poprzedni](72-narzedzia-ntc-table.md) · [Spis treści](README.md)
