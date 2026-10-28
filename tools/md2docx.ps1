# Konwersja sprawozdania Markdown -> DOCX na bazie szkolnego szablonu
# (naglowek, stopka i style z docs\szablony\SZABLON_SPRAWOZDANIA.docx).
#
# Obslugiwany podzbior Markdown:
#   front matter (zespol, etap, nazwa, data, opcjonalnie tytul), "## " naglowki,
#   "### " podnaglowki, akapity, listy "- " / "  - " (takze "- [ ]"), tabele "| a | b |",
#   bloki ``` ```, cytaty "> ", **pogrubienie**, `kod`, <br> w tabelach, linki (zostaje tekst).
#   "# " (tytul pliku .md), "---" i jednoliniowe komentarze <!-- --> sa pomijane.
#   Obrazow nie obsluguje - zdjecia/schematy wstawia sie w Wordzie po konwersji.
#
# Uzycie (z katalogu glownego repozytorium):
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\md2docx.ps1 docs\sprawozdania\etap1.md
#   -> docs\sprawozdania\etap1.docx

param(
    [Parameter(Mandatory = $true)][string]$Wejscie,
    [string]$Wyjscie,
    [string]$Szablon
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$repo = Split-Path -Parent $PSScriptRoot
if (-not $Szablon) { $Szablon = Join-Path $repo 'docs\szablony\SZABLON_SPRAWOZDANIA.docx' }
$Wejscie = (Resolve-Path $Wejscie).Path
if (-not $Wyjscie) { $Wyjscie = [IO.Path]::ChangeExtension($Wejscie, '.docx') }
if (-not [IO.Path]::IsPathRooted($Wyjscie)) { $Wyjscie = Join-Path (Get-Location) $Wyjscie }

$SzerokoscTekstu = 10772
$BoxPusty = [string][char]0x2610
$BoxZaznaczony = [string][char]0x2612

function Esc([string]$s) { return [Security.SecurityElement]::Escape($s) }

# Kolejnosc elementow w w:rPr wymagana przez schemat OOXML
function RPr([bool]$b, [bool]$i, [bool]$kod, [int]$sz) {
    $r = ''
    if ($kod) { $r += '<w:rFonts w:ascii="Consolas" w:hAnsi="Consolas" w:cs="Consolas"/>' }
    if ($b) { $r += '<w:b/><w:bCs/>' }
    if ($i) { $r += '<w:i/><w:iCs/>' }
    if ($sz -gt 0) { $r += '<w:sz w:val="' + $sz + '"/><w:szCs w:val="' + $sz + '"/>' }
    return $r
}

function Run([string]$tekst, [string]$rPr) {
    $t = Esc $tekst
    if ($rPr) { return '<w:r><w:rPr>' + $rPr + '</w:rPr><w:t xml:space="preserve">' + $t + '</w:t></w:r>' }
    return '<w:r><w:t xml:space="preserve">' + $t + '</w:t></w:r>'
}

function Inline([string]$tekst, [bool]$b = $false, [bool]$i = $false, [int]$sz = 0) {
    $tekst = [regex]::Replace($tekst, '\[([^\]]+)\]\(([^)]+)\)', '$1')
    $sb = New-Object System.Text.StringBuilder
    foreach ($c in [regex]::Split($tekst, '(\*\*[^*]+\*\*|`[^`]+`|<br>)')) {
        if ($c -eq '') { continue }
        if ($c -eq '<br>') { [void]$sb.Append('<w:r><w:br/></w:r>'); continue }
        if ($c.Length -gt 4 -and $c.StartsWith('**') -and $c.EndsWith('**')) {
            [void]$sb.Append((Run $c.Substring(2, $c.Length - 4) (RPr $true $i $false $sz)))
        } elseif ($c.Length -gt 2 -and $c.StartsWith('`') -and $c.EndsWith('`')) {
            [void]$sb.Append((Run $c.Substring(1, $c.Length - 2) (RPr $b $i $true $sz)))
        } else {
            [void]$sb.Append((Run $c (RPr $b $i $false $sz)))
        }
    }
    return $sb.ToString()
}

function Akapit([string]$tekst, [string]$pPr = '', [bool]$b = $false, [bool]$i = $false) {
    $p = '<w:p>'
    if ($pPr) { $p += '<w:pPr>' + $pPr + '</w:pPr>' }
    return $p + (Inline $tekst $b $i) + '</w:p>'
}

function Wiersz([string]$linia) {
    $l = $linia.Trim()
    if ($l.StartsWith('|')) { $l = $l.Substring(1) }
    if ($l.EndsWith('|')) { $l = $l.Substring(0, $l.Length - 1) }
    return @($l.Split('|') | ForEach-Object { $_.Trim() })
}

function Tabela($wiersze) {
    # Szerokosc kolumny: minimum na najdluzsze slowo + reszta proporcjonalnie do dlugosci tekstu
    $n = ($wiersze | ForEach-Object { $_.Count } | Measure-Object -Maximum).Maximum
    $minima = @()
    $wagi = @()
    for ($k = 0; $k -lt $n; $k++) {
        $maxTekst = 3
        $maxSlowo = 3
        foreach ($w in $wiersze) {
            if ($k -lt $w.Count) {
                foreach ($kawalek in ($w[$k] -split '<br>')) {
                    $czysty = $kawalek -replace '\*\*|`', ''
                    if ($czysty.Length -gt $maxTekst) { $maxTekst = $czysty.Length }
                    foreach ($slowo in $czysty.Split(' ')) {
                        if ($slowo.Length -gt $maxSlowo) { $maxSlowo = $slowo.Length }
                    }
                }
            }
        }
        $minima += [Math]::Min($maxSlowo * 105 + 220, [int]($SzerokoscTekstu / 2))
        $wagi += [Math]::Min($maxTekst, 80)
    }
    $sumaMin = ($minima | Measure-Object -Sum).Sum
    $sumaWag = ($wagi | Measure-Object -Sum).Sum
    $szer = @()
    $reszta = $SzerokoscTekstu
    for ($k = 0; $k -lt $n; $k++) {
        if ($sumaMin -ge $SzerokoscTekstu) {
            $s = [int][Math]::Floor($SzerokoscTekstu * $minima[$k] / $sumaMin)
        } else {
            $s = [int][Math]::Floor($minima[$k] + ($SzerokoscTekstu - $sumaMin) * $wagi[$k] / $sumaWag)
        }
        if ($k -eq $n - 1) { $s = $reszta }
        $szer += $s
        $reszta -= $s
    }

    $x = '<w:tbl><w:tblPr><w:tblStyle w:val="Tabela-Siatka"/><w:tblW w:w="' + $SzerokoscTekstu + '" w:type="dxa"/>'
    $x += '<w:tblLayout w:type="fixed"/><w:tblLook w:val="04A0" w:firstRow="1" w:lastRow="0" w:firstColumn="1" w:lastColumn="0" w:noHBand="0" w:noVBand="1"/></w:tblPr><w:tblGrid>'
    foreach ($s in $szer) { $x += '<w:gridCol w:w="' + $s + '"/>' }
    $x += '</w:tblGrid>'
    for ($r = 0; $r -lt $wiersze.Count; $r++) {
        $naglowek = ($r -eq 0)
        $x += '<w:tr>'
        if ($naglowek) { $x += '<w:trPr><w:tblHeader/></w:trPr>' }
        for ($k = 0; $k -lt $n; $k++) {
            $tekst = ''
            if ($k -lt $wiersze[$r].Count) { $tekst = $wiersze[$r][$k] }
            $x += '<w:tc><w:tcPr><w:tcW w:w="' + $szer[$k] + '" w:type="dxa"/>'
            if ($naglowek) { $x += '<w:shd w:val="clear" w:color="auto" w:fill="D9E2F3"/>' }
            $x += '</w:tcPr><w:p><w:pPr><w:spacing w:before="20" w:after="20"/></w:pPr>'
            $x += (Inline $tekst $naglowek $false 20) + '</w:p></w:tc>'
        }
        $x += '</w:tr>'
    }
    return $x + '</w:tbl><w:p><w:pPr><w:spacing w:after="0"/></w:pPr></w:p>'
}

function Kod([string]$linia) {
    $pPr = '<w:shd w:val="clear" w:color="auto" w:fill="F2F2F2"/><w:spacing w:after="0" w:line="240" w:lineRule="auto"/>'
    return '<w:p><w:pPr>' + $pPr + '</w:pPr>' + (Run $linia (RPr $false $false $true 18)) + '</w:p>'
}

# --- Wczytanie Markdown
$linie = [IO.File]::ReadAllLines($Wejscie, [Text.Encoding]::UTF8)
$meta = @{}
$start = 0
if ($linie.Count -gt 0 -and $linie[0].Trim() -eq '---') {
    for ($j = 1; $j -lt $linie.Count; $j++) {
        if ($linie[$j].Trim() -eq '---') { $start = $j + 1; break }
        if ($linie[$j] -match '^\s*([A-Za-z]+)\s*:\s*(.*)$') { $meta[$Matches[1].ToLower()] = $Matches[2].Trim().Trim('"') }
    }
}

# --- Budowa tresci
$body = New-Object System.Text.StringBuilder
$akapit = @()
$tabela = @()
$wKodzie = $false

function Wypisz-Akapit {
    if ($script:akapit.Count -gt 0) {
        [void]$script:body.Append((Akapit ($script:akapit -join ' ')))
        $script:akapit = @()
    }
}
function Wypisz-Tabele {
    if ($script:tabela.Count -gt 0) {
        [void]$script:body.Append((Tabela $script:tabela))
        $script:tabela = @()
    }
}

for ($j = $start; $j -lt $linie.Count; $j++) {
    $l = $linie[$j]

    if ($l.Trim().StartsWith('```')) {
        Wypisz-Akapit; Wypisz-Tabele
        $wKodzie = -not $wKodzie
        if (-not $wKodzie) { [void]$body.Append('<w:p><w:pPr><w:spacing w:after="0"/></w:pPr></w:p>') }
        continue
    }
    if ($wKodzie) { [void]$body.Append((Kod $l)); continue }
    if ($l -match '^\s*<!--.*-->\s*$') { continue }

    if ($l.Trim().StartsWith('|')) {
        Wypisz-Akapit
        if ($l -notmatch '^\s*\|?\s*:?-{3,}') { $tabela += , (Wiersz $l) }
        continue
    }
    Wypisz-Tabele

    if ($l.Trim() -eq '' -or $l.Trim() -eq '---') { Wypisz-Akapit; continue }
    if ($l -match '^#\s') { Wypisz-Akapit; continue }
    if ($l -match '^##\s+(.*)$') {
        Wypisz-Akapit
        [void]$body.Append((Akapit $Matches[1] '<w:pStyle w:val="Nagwek2"/>'))
        continue
    }
    if ($l -match '^###\s+(.*)$') {
        Wypisz-Akapit
        [void]$body.Append((Akapit $Matches[1] '<w:keepNext/><w:spacing w:before="160" w:after="80"/>' $true))
        continue
    }
    if ($l -match '^(\s*)[-*]\s+(.*)$') {
        Wypisz-Akapit
        $poziom = [Math]::Min([int][Math]::Floor($Matches[1].Length / 2), 1)
        $tekst = $Matches[2] -replace '^\[ \]\s*', ($BoxPusty + ' ') -replace '^\[[xX]\]\s*', ($BoxZaznaczony + ' ')
        $pPr = '<w:numPr><w:ilvl w:val="' + $poziom + '"/><w:numId w:val="2"/></w:numPr><w:spacing w:after="40"/>'
        [void]$body.Append((Akapit $tekst $pPr))
        continue
    }
    if ($l -match '^>\s?(.*)$') {
        Wypisz-Akapit
        [void]$body.Append((Akapit $Matches[1] '<w:ind w:left="360"/>' $false $true))
        continue
    }
    if ($l -match '^\d+\.\s') {
        Wypisz-Akapit
        [void]$body.Append((Akapit $l.Trim() '<w:spacing w:after="40"/>'))
        continue
    }
    $akapit += $l.Trim()
}
Wypisz-Akapit
Wypisz-Tabele

# --- Wstawienie do szablonu
$zip = [IO.Compression.ZipFile]::OpenRead($Szablon)
try {
    $czytnik = New-Object IO.StreamReader($zip.GetEntry('word/document.xml').Open(), [Text.Encoding]::UTF8)
    $xml = $czytnik.ReadToEnd()
    $czytnik.Close()
} finally { $zip.Dispose() }

$pola = @('zespol', 'etap', 'nazwa', 'data')
$puste = [regex]::Matches($xml, '(<w:p [^>]*>)<w:pPr><w:rPr><w:sz w:val="32"/></w:rPr></w:pPr></w:p>')
if ($puste.Count -ne 4) { throw 'Szablon ma inna strukture naglowka niz oczekiwano (4 pola).' }
for ($k = 3; $k -ge 0; $k--) {
    $m = $puste[$k]
    $wartosc = ''
    if ($meta.ContainsKey($pola[$k])) { $wartosc = $meta[$pola[$k]] }
    $nowy = $m.Groups[1].Value + '<w:pPr><w:rPr><w:sz w:val="32"/></w:rPr></w:pPr>' + (Run $wartosc (RPr $false $false $false 32)) + '</w:p>'
    $xml = $xml.Substring(0, $m.Index) + $nowy + $xml.Substring($m.Index + $m.Length)
}
if ($meta.ContainsKey('tytul')) { $xml = $xml.Replace('<w:t>SPRAWOZDANIE</w:t>', '<w:t>' + (Esc $meta['tytul']) + '</w:t>') }

$koniecTabeli = $xml.IndexOf('</w:tbl>') + '</w:tbl>'.Length
$sekcja = $xml.IndexOf('<w:sectPr')
$podpis = '<w:p/><w:p/>' + (Akapit 'data i podpis: ....................................................')
$xml = $xml.Substring(0, $koniecTabeli) + '<w:p/>' + $body.ToString() + $podpis + $xml.Substring($sekcja)

[void][xml]$xml

Copy-Item $Szablon $Wyjscie -Force
$zip = [IO.Compression.ZipFile]::Open($Wyjscie, [IO.Compression.ZipArchiveMode]::Update)
try {
    $zip.GetEntry('word/document.xml').Delete()
    $strumien = $zip.CreateEntry('word/document.xml', [IO.Compression.CompressionLevel]::Optimal).Open()
    $bajty = (New-Object Text.UTF8Encoding $false).GetBytes($xml)
    $strumien.Write($bajty, 0, $bajty.Length)
    $strumien.Close()
} finally { $zip.Dispose() }

Write-Host "Zapisano: $Wyjscie"
