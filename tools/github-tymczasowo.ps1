# Tymczasowa sesja GitHub CLI na cudzym (szkolnym) komputerze:
# bez uprawnien administratora i bez zostawiania loginow.
# gh, token, konfiguracja i cache leza tylko w %TEMP%\gh-tymczasowy,
# token NIE trafia do Menedzera poswiadczen Windows, przegladarka na tym
# komputerze nie jest otwierana (kod logowania wpisujesz na telefonie).
#
# Uzycie (z katalogu glownego repozytorium):
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Zaloguj
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wypchnij
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\github-tymczasowo.ps1 -Akcja Wyloguj
#
# Wyloguj = wylogowanie + usuniecie gh, tokenu i wpisow w globalnym .gitconfig.

param(
    [ValidateSet('Zaloguj', 'Wypchnij', 'Status', 'Wyloguj')]
    [string]$Akcja = 'Status'
)

$ErrorActionPreference = 'Stop'
$GhWersja = '2.103.3'
$Baza = Join-Path $env:TEMP 'gh-tymczasowy'

$env:GH_CONFIG_DIR = Join-Path $Baza 'config'
$env:XDG_STATE_HOME = Join-Path $Baza 'state'
$env:XDG_DATA_HOME = Join-Path $Baza 'data'
$env:XDG_CACHE_HOME = Join-Path $Baza 'cache'
$env:GH_NO_UPDATE_NOTIFIER = '1'
$env:GH_BROWSER = 'cmd /c rem'

function Znajdz-Gh {
    if (-not (Test-Path $Baza)) { return $null }
    $plik = Get-ChildItem $Baza -Recurse -Filter 'gh.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($plik) { return $plik.FullName }
    return $null
}

function Pobierz-Gh {
    $gh = Znajdz-Gh
    if ($gh) { return $gh }

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $url = "https://github.com/cli/cli/releases/download/v$GhWersja"
    $zip = "gh_${GhWersja}_windows_amd64.zip"
    $pobrane = Join-Path $Baza 'pobrane'
    New-Item -ItemType Directory -Force $pobrane | Out-Null

    Write-Host "Pobieram $zip z github.com/cli/cli ..."
    Invoke-WebRequest "$url/$zip" -OutFile (Join-Path $pobrane $zip) -UseBasicParsing
    Invoke-WebRequest "$url/gh_${GhWersja}_checksums.txt" -OutFile (Join-Path $pobrane 'sumy.txt') -UseBasicParsing

    $linia = Get-Content (Join-Path $pobrane 'sumy.txt') | Where-Object { $_ -match "\s$([regex]::Escape($zip))$" }
    $oczekiwana = ($linia -split '\s+')[0]
    $faktyczna = (Get-FileHash (Join-Path $pobrane $zip) -Algorithm SHA256).Hash
    if (-not $oczekiwana -or $faktyczna -ne $oczekiwana) {
        Remove-Item $Baza -Recurse -Force
        throw 'Suma SHA256 pobranego gh sie nie zgadza - przerwano.'
    }
    Write-Host 'Suma SHA256 zgodna.'

    Expand-Archive (Join-Path $pobrane $zip) -DestinationPath (Join-Path $Baza 'gh') -Force
    Remove-Item $pobrane -Recurse -Force
    return (Znajdz-Gh)
}

function Usun-WpisyGit {
    # 'gh auth login' moze dopisac helper do globalnego .gitconfig - usuwamy tylko nasze wpisy
    $globalny = Join-Path $env:USERPROFILE '.gitconfig'
    if (-not (Test-Path $globalny)) { return }
    $wpisy = git config --global --get-regexp '^credential\..*\.helper$'
    $klucze = @($wpisy | Where-Object { $_ -like '*gh-tymczasowy*' } | ForEach-Object { ($_ -split ' ', 2)[0] } | Sort-Object -Unique)
    foreach ($klucz in $klucze) {
        git config --global --unset-all $klucz
        Write-Host "Usunieto z .gitconfig: $klucz"
    }
    $reszta = git config --global --list
    if (-not $reszta) {
        Remove-Item $globalny -Force
        Write-Host 'Usunieto pusty .gitconfig.'
    }
}

switch ($Akcja) {
    'Zaloguj' {
        $gh = Pobierz-Gh
        # Czy wpis github.com byl w Menedzerze poswiadczen przed logowaniem (Wyloguj usuwa tylko nasz)
        $byl = [bool](cmdkey /list | Select-String -Pattern 'git:https://github.com' -SimpleMatch -Quiet)
        Set-Content (Join-Path $Baza 'gcm-przed.txt') ([int]$byl)
        Write-Host ''
        Write-Host 'Za chwile pojawi sie jednorazowy kod.'
        Write-Host 'Na TELEFONIE otworz https://github.com/login/device i wpisz ten kod.'
        Write-Host 'Tutaj nacisnij Enter (przegladarka na tym komputerze sie nie otworzy).'
        Write-Host 'Jesli gh zapyta "Authenticate Git with your GitHub credentials?" - odpowiedz: n'
        Write-Host ''
        & $gh auth login --hostname github.com --web --git-protocol https --insecure-storage --skip-ssh-key
        & $gh auth status
    }
    'Wypchnij' {
        $gh = Znajdz-Gh
        if (-not $gh) { throw 'Brak gh - najpierw -Akcja Zaloguj.' }
        $helper = "!'" + ($gh -replace '\\', '/') + "' auth git-credential"
        git -c credential.helper= -c "credential.helper=$helper" push -u origin HEAD
    }
    'Status' {
        $gh = Znajdz-Gh
        if (-not $gh) { Write-Host 'Brak tymczasowego gh - nic nie jest zalogowane.'; break }
        & $gh auth status
    }
    'Wyloguj' {
        $gh = Znajdz-Gh
        if ($gh) {
            try { & $gh auth logout --hostname github.com } catch { }
        }
        Usun-WpisyGit

        # Odpowiedz "Y" przy logowaniu zapisuje token w Menedzerze poswiadczen (Git Credential Manager)
        $znacznik = Join-Path $Baza 'gcm-przed.txt'
        if (cmdkey /list | Select-String -Pattern 'git:https://github.com' -SimpleMatch -Quiet) {
            if ((Test-Path $znacznik) -and ((Get-Content $znacznik) -eq '0')) {
                cmdkey /delete:git:https://github.com | Out-Null
                Write-Host 'Usunieto poswiadczenie github.com z Menedzera poswiadczen Windows.'
            } else {
                Write-Warning 'Wpis git:https://github.com w Menedzerze poswiadczen istnial przed logowaniem - nie ruszam.'
            }
        }

        if (Test-Path $Baza) { Remove-Item $Baza -Recurse -Force }
        Write-Host "Usunieto $Baza (gh, token, konfiguracja)."

        foreach ($sciezka in @("$env:LOCALAPPDATA\GitHub CLI", "$env:APPDATA\GitHub CLI")) {
            if (Test-Path $sciezka) { Write-Warning "Istnieje $sciezka - sprawdz recznie." }
        }
        if (cmdkey /list | Select-String -Pattern 'github' -Quiet) {
            Write-Warning 'W Menedzerze poswiadczen Windows sa wpisy github - sprawdz: cmdkey /list'
        }
        Write-Host 'Gotowe. Token mozesz tez uniewaznic: GitHub > Settings > Applications > Authorized OAuth Apps > GitHub CLI > Revoke.'
    }
}
