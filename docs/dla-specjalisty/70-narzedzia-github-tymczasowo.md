# Narzędzia: `github-tymczasowo.ps1`

[← Poprzedni](61-sim-avrsim-c.md) · [Spis treści](README.md) · [Następny →](71-narzedzia-md2docx.md)

**Po co ten plik:** komputer szkolny jest wspólny, więc nie można na nim zostawić zalogowanego konta. Skrypt pobiera program GitHub CLI (`gh`) do katalogu tymczasowego, pozwala się zalogować i wysłać zmiany, a na koniec wszystko po sobie usuwa. Składnia PowerShella jest w [podstawach narzędzi](04-podstawy-narzedzi.md).

Skrypt ma cztery akcje: `Zaloguj`, `Wypchnij`, `Status` i `Wyloguj`.

**Skąd brak polskich liter w pliku:** Windows PowerShell 5.1 źle odczytuje znaki spoza ASCII w plikach bez specjalnego oznaczenia, dlatego ten skrypt zawiera tylko litery bez ogonków.

## `tools/github-tymczasowo.ps1`

[Otwórz ten plik w repozytorium](../../tools/github-tymczasowo.ps1)

### Opis

```powershell
  1  # Tymczasowa sesja GitHub CLI na cudzym (szkolnym) komputerze:
  2  # bez uprawnien administratora i bez zostawiania loginow.
  3  # gh, token, konfiguracja i cache leza tylko w %TEMP%\gh-tymczasowy,
  4  # token NIE trafia do Menedzera poswiadczen Windows, przegladarka na tym
  5  # komputerze nie jest otwierana (kod logowania wpisujesz na telefonie).
  6  #
  7  # Uzycie (z katalogu glownego repozytorium):
  8  #   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Zaloguj
  9  #   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wypchnij
 10  #   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wyloguj
 11  #
 12  # Wyloguj = wylogowanie + usuniecie gh, tokenu i wpisow w globalnym .gitconfig.
```

- **Linia 1:** Komentarz: tymczasowa sesja GitHub CLI na cudzym, szkolnym komputerze...
- **Linia 2:** ...bez uprawnień administratora i bez zostawiania loginów.
- **Linia 3:** Program, token, konfiguracja i pamięć podręczna leżą tylko w katalogu tymczasowym. **Token** to rodzaj hasła dla programów.
- **Linia 4:** Token nie trafia do Menedżera poświadczeń Windows, a przeglądarka na tym...
- **Linia 5:** ...komputerze się nie otwiera, bo kod logowania wpisuje się na telefonie.
- **Linia 6:** Pusty komentarz, odstęp.
- **Linia 7:** Sposób użycia. Polecenia uruchamia się z głównego katalogu repozytorium.
- **Linia 8:** Przykład: logowanie. `-ExecutionPolicy Bypass` pozwala uruchomić skrypt mimo ograniczeń systemu.
- **Linia 9:** Przykład: wysłanie zmian.
- **Linia 10:** Przykład: wylogowanie.
- **Linia 11:** Pusty komentarz, odstęp.
- **Linia 12:** Wylogowanie oznacza także usunięcie programu, tokenu i wpisów w konfiguracji gita.

### Parametry

```powershell
 14  param(
 15      [ValidateSet('Zaloguj', 'Wypchnij', 'Status', 'Wyloguj')]
 16      [string]$Akcja = 'Status'
 17  )
```

- **Linia 14:** Początek listy parametrów skryptu.
- **Linia 15:** Dozwolone wartości kolejnego parametru. Inna wartość spowoduje błąd.
- **Linia 16:** Parametr `$Akcja`, tekstowy, domyślnie `Status`.
- **Linia 17:** Koniec listy.

### Ustawienia

```powershell
 19  $ErrorActionPreference = 'Stop'
 20  $GhWersja = '2.103.3'
 21  $Baza = Join-Path $env:TEMP 'gh-tymczasowy'
 22
 23  $env:GH_CONFIG_DIR = Join-Path $Baza 'config'
 24  $env:XDG_STATE_HOME = Join-Path $Baza 'state'
 25  $env:XDG_DATA_HOME = Join-Path $Baza 'data'
 26  $env:XDG_CACHE_HOME = Join-Path $Baza 'cache'
 27  $env:GH_NO_UPDATE_NOTIFIER = '1'
 28  $env:GH_BROWSER = 'cmd /c rem'
```

- **Linia 19:** Każdy błąd ma przerywać skrypt, a nie być pomijany.
- **Linia 20:** Wersja programu `gh` do pobrania.
- **Linia 21:** Katalog roboczy: `gh-tymczasowy` w katalogu tymczasowym systemu.
- **Linia 23:** Każ programowi `gh` trzymać konfigurację w katalogu roboczym.
- **Linia 24:** Tak samo stan programu.
- **Linia 25:** Tak samo dane.
- **Linia 26:** Tak samo pamięć podręczną.
- **Linia 27:** Wyłącz sprawdzanie aktualizacji.
- **Linia 28:** Jako przeglądarkę ustaw polecenie, które nic nie robi (`rem` to w Windows komentarz). Dzięki temu przeglądarka się nie otworzy.

### Szukanie programu

```powershell
 30  function Znajdz-Gh {
 31      if (-not (Test-Path $Baza)) { return $null }
 32      $plik = Get-ChildItem $Baza -Recurse -Filter 'gh.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
 33      if ($plik) { return $plik.FullName }
 34      return $null
 35  }
```

- **Linia 30:** Funkcja szukająca pobranego programu `gh.exe`.
- **Linia 31:** Jeśli katalog roboczy nie istnieje, zwróć brak wyniku.
- **Linia 32:** Przeszukaj katalog z podkatalogami, pomijaj błędy i weź pierwszy wynik.
- **Linia 33:** Jeśli znaleziono, zwróć pełną ścieżkę.
- **Linia 34:** W przeciwnym razie zwróć brak wyniku.
- **Linia 35:** Koniec funkcji.

### Pobieranie programu

```powershell
 37  function Pobierz-Gh {
 38      $gh = Znajdz-Gh
 39      if ($gh) { return $gh }
 40
 41      [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
 42      $url = "https://github.com/cli/cli/releases/download/v$GhWersja"
 43      $zip = "gh_${GhWersja}_windows_amd64.zip"
 44      $pobrane = Join-Path $Baza 'pobrane'
 45      New-Item -ItemType Directory -Force $pobrane | Out-Null
 46
 47      Write-Host "Pobieram $zip z github.com/cli/cli ..."
 48      Invoke-WebRequest "$url/$zip" -OutFile (Join-Path $pobrane $zip) -UseBasicParsing
 49      Invoke-WebRequest "$url/gh_${GhWersja}_checksums.txt" -OutFile (Join-Path $pobrane 'sumy.txt') -UseBasicParsing
```

- **Linia 37:** Funkcja zwracająca ścieżkę programu, w razie potrzeby pobierająca go.
- **Linia 38:** Sprawdź, czy program już jest.
- **Linia 39:** Jeśli jest, zwróć go.
- **Linia 41:** Włącz szyfrowanie TLS 1.2, którego wymaga GitHub. Starszy PowerShell domyślnie go nie włącza.
- **Linia 42:** Adres plików danej wersji.
- **Linia 43:** Nazwa pobieranego archiwum. Klamry oddzielają nazwę zmiennej od dalszego tekstu.
- **Linia 44:** Katalog na pobrane pliki.
- **Linia 45:** Utwórz go. `Out-Null` odrzuca wynik polecenia.
- **Linia 47:** Wypisz komunikat.
- **Linia 48:** Pobierz archiwum.
- **Linia 49:** Pobierz plik z sumami kontrolnymi.

### Sprawdzenie, czy plik jest prawdziwy

Suma kontrolna SHA256 to „odcisk palca” pliku. Jeśli plik pobrany różni się choćby jednym bitem, odcisk będzie inny.

```powershell
 51      $linia = Get-Content (Join-Path $pobrane 'sumy.txt') | Where-Object { $_ -match "\s$([regex]::Escape($zip))$" }
 52      $oczekiwana = ($linia -split '\s+')[0]
 53      $faktyczna = (Get-FileHash (Join-Path $pobrane $zip) -Algorithm SHA256).Hash
 54      if (-not $oczekiwana -or $faktyczna -ne $oczekiwana) {
 55          Remove-Item $Baza -Recurse -Force
 56          throw 'Suma SHA256 pobranego gh sie nie zgadza - przerwano.'
 57      }
 58      Write-Host 'Suma SHA256 zgodna.'
 59
 60      Expand-Archive (Join-Path $pobrane $zip) -DestinationPath (Join-Path $Baza 'gh') -Force
 61      Remove-Item $pobrane -Recurse -Force
 62      return (Znajdz-Gh)
 63  }
```

- **Linia 51:** W pliku z sumami znajdź linię kończącą się nazwą naszego archiwum. `Escape` sprawia, że kropki w nazwie są traktowane dosłownie.
- **Linia 52:** Podziel linię po białych znakach i weź pierwszą część, czyli oczekiwaną sumę.
- **Linia 53:** Oblicz sumę pobranego pliku.
- **Linia 54:** Jeśli oczekiwanej sumy nie ma albo sumy są różne...
- **Linia 55:** ...usuń wszystko...
- **Linia 56:** ...i przerwij skrypt z komunikatem.
- **Linia 57:** Koniec warunku.
- **Linia 58:** Wypisz informację o zgodności.
- **Linia 60:** Rozpakuj archiwum.
- **Linia 61:** Usuń pobrane pliki.
- **Linia 62:** Zwróć ścieżkę programu.
- **Linia 63:** Koniec funkcji.

### Czyszczenie konfiguracji gita

```powershell
 65  function Usun-WpisyGit {
 66      # 'gh auth login' moze dopisac helper do globalnego .gitconfig - usuwamy tylko nasze wpisy
 67      $globalny = Join-Path $env:USERPROFILE '.gitconfig'
 68      if (-not (Test-Path $globalny)) { return }
 69      $wpisy = git config --global --get-regexp '^credential\..*\.helper$'
 70      $klucze = @($wpisy | Where-Object { $_ -like '*gh-tymczasowy*' } | ForEach-Object { ($_ -split ' ', 2)[0] } | Sort-Object -Unique)
 71      foreach ($klucz in $klucze) {
 72          git config --global --unset-all $klucz
 73          Write-Host "Usunieto z .gitconfig: $klucz"
 74      }
 75      $reszta = git config --global --list
 76      if (-not $reszta) {
 77          Remove-Item $globalny -Force
 78          Write-Host 'Usunieto pusty .gitconfig.'
 79      }
 80  }
```

- **Linia 65:** Funkcja usuwająca nasze wpisy z globalnej konfiguracji gita.
- **Linia 66:** Komentarz: logowanie może dopisać wpis do pliku `.gitconfig`, dlatego usuwamy tylko nasze wpisy.
- **Linia 67:** Ścieżka do pliku `.gitconfig`.
- **Linia 68:** Jeśli pliku nie ma, zakończ.
- **Linia 69:** Odczytaj wszystkie wpisy o programach obsługujących hasła.
- **Linia 70:** Zostaw tylko te zawierające nazwę naszego katalogu, weź z nich sam klucz i usuń powtórzenia.
- **Linia 71:** Dla każdego klucza...
- **Linia 72:** ...usuń go z konfiguracji...
- **Linia 73:** ...i wypisz komunikat.
- **Linia 74:** Koniec pętli.
- **Linia 75:** Odczytaj, co zostało w konfiguracji.
- **Linia 76:** Jeśli nic...
- **Linia 77:** ...usuń plik...
- **Linia 78:** ...i wypisz komunikat.
- **Linia 79:** Koniec warunku.
- **Linia 80:** Koniec funkcji.

### Akcja `Zaloguj`

```powershell
 82  switch ($Akcja) {
 83      'Zaloguj' {
 84          $gh = Pobierz-Gh
 85          # Czy wpis github.com byl w Menedzerze poswiadczen przed logowaniem (Wyloguj usuwa tylko nasz)
 86          $byl = [bool](cmdkey /list | Select-String -Pattern 'git:https://github.com' -SimpleMatch -Quiet)
 87          Set-Content (Join-Path $Baza 'gcm-przed.txt') ([int]$byl)
 88          Write-Host ''
 89          Write-Host 'Za chwile pojawi sie jednorazowy kod.'
 90          Write-Host 'Na TELEFONIE otworz https://github.com/login/device i wpisz ten kod.'
 91          Write-Host 'Tutaj nacisnij Enter (przegladarka na tym komputerze sie nie otworzy).'
 92          Write-Host 'Jesli gh zapyta "Authenticate Git with your GitHub credentials?" - odpowiedz: n'
 93          Write-Host ''
 94          & $gh auth login --hostname github.com --web --git-protocol https --insecure-storage --skip-ssh-key
 95          & $gh auth status
 96      }
```

- **Linia 82:** Wybierz działanie zależnie od wartości `$Akcja`.
- **Linia 83:** Początek akcji `Zaloguj`.
- **Linia 84:** Pobierz program, jeśli go nie ma.
- **Linia 85:** Komentarz: sprawdzamy, czy wpis GitHuba był już w Menedżerze poświadczeń, bo przy wylogowaniu usuwamy tylko swój.
- **Linia 86:** Sprawdź, czy lista poświadczeń zawiera wpis GitHuba. Wynik zamień na prawdę lub fałsz.
- **Linia 87:** Zapisz to w pliku, jako 1 albo 0.
- **Linia 88:** Wypisz pustą linię.
- **Linia 89:** Instrukcja dla użytkownika: pojawi się kod.
- **Linia 90:** Instrukcja: otwórz stronę na telefonie i wpisz kod.
- **Linia 91:** Instrukcja: tutaj naciśnij Enter.
- **Linia 92:** Instrukcja: na pytanie o połączenie z gitem odpowiedz „n”.
- **Linia 93:** Wypisz pustą linię.
- **Linia 94:** Uruchom logowanie. `--hostname github.com` wskazuje serwis, `--web` to logowanie kodem, `--git-protocol https` to sposób łączenia, `--insecure-storage` zapisuje token w zwykłym pliku w katalogu roboczym, zamiast w Menedżerze poświadczeń, a `--skip-ssh-key` pomija pytanie o klucz SSH.
- **Linia 95:** Pokaż stan logowania.
- **Linia 96:** Koniec akcji.

### Akcja `Wypchnij`

```powershell
 97      'Wypchnij' {
 98          $gh = Znajdz-Gh
 99          if (-not $gh) { throw 'Brak gh - najpierw -Akcja Zaloguj.' }
100          $helper = "!'" + ($gh -replace '\\', '/') + "' auth git-credential"
101          git -c credential.helper= -c "credential.helper=$helper" push -u origin HEAD
102      }
```

- **Linia 97:** Początek akcji `Wypchnij`.
- **Linia 98:** Znajdź program.
- **Linia 99:** Jeśli go nie ma, przerwij z podpowiedzią.
- **Linia 100:** Zbuduj polecenie dla gita: niech o hasło pyta nasz program `gh`. Ukośniki są zamieniane, bo git woli `/`, a `!` oznacza „uruchom polecenie”.
- **Linia 101:** Wyślij zmiany. Pierwsza opcja `-c` czyści inne programy obsługi haseł, a druga ustawia nasz tylko na czas tego polecenia. Nic nie jest zapisywane na stałe.
- **Linia 102:** Koniec akcji.

### Akcja `Status`

```powershell
103      'Status' {
104          $gh = Znajdz-Gh
105          if (-not $gh) { Write-Host 'Brak tymczasowego gh - nic nie jest zalogowane.'; break }
106          & $gh auth status
107      }
```

- **Linia 103:** Początek akcji `Status`.
- **Linia 104:** Znajdź program.
- **Linia 105:** Jeśli go nie ma, wypisz komunikat i wyjdź z bloku `switch`.
- **Linia 106:** Pokaż stan logowania.
- **Linia 107:** Koniec akcji.

### Akcja `Wyloguj`

```powershell
108      'Wyloguj' {
109          $gh = Znajdz-Gh
110          if ($gh) {
111              try { & $gh auth logout --hostname github.com } catch { }
112          }
113          Usun-WpisyGit
114
```

- **Linia 108:** Początek akcji `Wyloguj`.
- **Linia 109:** Znajdź program.
- **Linia 110:** Jeśli jest...
- **Linia 111:** ...wyloguj się. Ewentualny błąd jest ignorowany, bo sprzątanie ma trwać dalej.
- **Linia 112:** Koniec warunku.
- **Linia 113:** Usuń wpisy z konfiguracji gita.

```powershell
115          # Odpowiedz "Y" przy logowaniu zapisuje token w Menedzerze poswiadczen (Git Credential Manager)
116          $znacznik = Join-Path $Baza 'gcm-przed.txt'
117          if (cmdkey /list | Select-String -Pattern 'git:https://github.com' -SimpleMatch -Quiet) {
118              if ((Test-Path $znacznik) -and ((Get-Content $znacznik) -eq '0')) {
119                  cmdkey /delete:git:https://github.com | Out-Null
120                  Write-Host 'Usunieto poswiadczenie github.com z Menedzera poswiadczen Windows.'
121              } else {
122                  Write-Warning 'Wpis git:https://github.com w Menedzerze poswiadczen istnial przed logowaniem - nie ruszam.'
123              }
124          }
```

- **Linia 115:** Komentarz: odpowiedź „Y” przy logowaniu zapisuje token w Menedżerze poświadczeń.
- **Linia 116:** Ścieżka pliku z informacją zapisaną przy logowaniu.
- **Linia 117:** Jeśli na liście poświadczeń jest wpis GitHuba...
- **Linia 118:** ...i wiemy, że przed logowaniem go nie było...
- **Linia 119:** ...usuń go...
- **Linia 120:** ...i wypisz komunikat.
- **Linia 121:** W przeciwnym razie:
- **Linia 122:** Wypisz ostrzeżenie, że wpis istniał wcześniej, więc go nie ruszamy.
- **Linia 123:** Koniec warunku.
- **Linia 124:** Koniec warunku.

```powershell
126          if (Test-Path $Baza) { Remove-Item $Baza -Recurse -Force }
127          Write-Host "Usunieto $Baza (gh, token, konfiguracja)."
128
129          foreach ($sciezka in @("$env:LOCALAPPDATA\GitHub CLI", "$env:APPDATA\GitHub CLI")) {
130              if (Test-Path $sciezka) { Write-Warning "Istnieje $sciezka - sprawdz recznie." }
131          }
132          if (cmdkey /list | Select-String -Pattern 'github' -Quiet) {
133              Write-Warning 'W Menedzerze poswiadczen Windows sa wpisy github - sprawdz: cmdkey /list'
134          }
135          Write-Host 'Gotowe. Token mozesz tez uniewaznic: GitHub > Settings > Applications > Authorized OAuth Apps > GitHub CLI > Revoke.'
136      }
137  }
```

- **Linia 126:** Jeśli katalog roboczy istnieje, usuń go z całą zawartością.
- **Linia 127:** Wypisz komunikat.
- **Linia 129:** Dla dwóch standardowych lokalizacji konfiguracji `gh`...
- **Linia 130:** ...jeśli istnieją, wypisz ostrzeżenie. Skrypt ich nie utworzył, więc ich nie usuwa.
- **Linia 131:** Koniec pętli.
- **Linia 132:** Jeśli na liście poświadczeń są jeszcze wpisy GitHuba...
- **Linia 133:** ...wypisz ostrzeżenie.
- **Linia 134:** Koniec warunku.
- **Linia 135:** Wypisz podsumowanie i podpowiedź, jak dodatkowo unieważnić token na stronie GitHuba.
- **Linia 136:** Koniec akcji.
- **Linia 137:** Koniec bloku `switch`.

---

[← Poprzedni](61-sim-avrsim-c.md) · [Spis treści](README.md) · [Następny →](71-narzedzia-md2docx.md)
