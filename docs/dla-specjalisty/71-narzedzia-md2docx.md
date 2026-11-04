# Narzędzia: `md2docx.ps1`

[← Poprzedni](70-narzedzia-github-tymczasowo.md) · [Spis treści](README.md) · [Następny →](72-narzedzia-ntc-table.md)

**Po co ten plik:** sprawozdania piszemy w Markdownie, a oddać trzeba plik Word na szkolnym szablonie. Na komputerze szkolnym nie ma Worda ani Pythona, więc ten skrypt sam buduje plik `.docx`.

**Co warto wiedzieć o pliku `.docx`:** to archiwum ZIP z plikami XML. Treść dokumentu jest w `word/document.xml`. **XML** to tekstowy format ze znacznikami, podobny do HTML. Skrypt wykonuje trzy kroki:

1. czyta plik Markdown linia po linii,
2. buduje z niego tekst XML,
3. wstawia ten tekst do kopii szablonu.

**Najważniejsze znaczniki Worda:**

| Znacznik | Znaczenie |
|---|---|
| `<w:p>` | akapit |
| `<w:pPr>` | ustawienia akapitu |
| `<w:r>` | **run**, czyli fragment tekstu o jednolitym formatowaniu |
| `<w:rPr>` | ustawienia fragmentu |
| `<w:t>` | sam tekst |
| `<w:b/>`, `<w:i/>` | pogrubienie, kursywa |
| `<w:sz>` | rozmiar czcionki, w połówkach punktu |
| `<w:tbl>`, `<w:tr>`, `<w:tc>` | tabela, wiersz, komórka |

Składnia PowerShella jest w [podstawach narzędzi](04-podstawy-narzedzi.md).

## `tools/md2docx.ps1`

[Otwórz ten plik w repozytorium](../../tools/md2docx.ps1)

### Opis

````powershell
  1  # Konwersja sprawozdania Markdown -> DOCX na bazie szkolnego szablonu
  2  # (naglowek, stopka i style z docs\szablony\SZABLON_SPRAWOZDANIA.docx).
  3  #
  4  # Obslugiwany podzbior Markdown:
  5  #   front matter (zespol, etap, nazwa, data, opcjonalnie tytul), "## " naglowki,
  6  #   "### " podnaglowki, akapity, listy "- " / "  - " (takze "- [ ]"), tabele "| a | b |",
  7  #   bloki ``` ```, cytaty "> ", **pogrubienie**, `kod`, <br> w tabelach, linki (zostaje tekst).
  8  #   "# " (tytul pliku .md), "---" i jednoliniowe komentarze <!-- --> sa pomijane.
  9  #   Obrazow nie obsluguje - zdjecia/schematy wstawia sie w Wordzie po konwersji.
 10  #
 11  # Uzycie (z katalogu glownego repozytorium):
 12  #   powershell -NoProfile -ExecutionPolicy Bypass -File tools\md2docx.ps1 docs\sprawozdania\etap1.md
 13  #   -> docs\sprawozdania\etap1.docx
````

- **Linia 1:** Komentarz: konwersja sprawozdania z Markdowna na DOCX na bazie szkolnego szablonu...
- **Linia 2:** ...z którego pochodzą nagłówek, stopka i style.
- **Linia 3:** Pusty komentarz, odstęp.
- **Linia 4:** Komentarz: obsługiwana jest tylko część składni Markdowna.
- **Linia 5:** Front matter, czyli dane na początku pliku, oraz nagłówki `##`.
- **Linia 6:** Podnagłówki `###`, akapity, listy i tabele.
- **Linia 7:** Bloki kodu, cytaty, pogrubienie, kod w tekście, `<br>` w tabelach i linki (zostaje sam tekst).
- **Linia 8:** Tytuł pliku, linie `---` i komentarze są pomijane.
- **Linia 9:** Obrazów skrypt nie obsługuje. Wstawia się je w Wordzie po konwersji.
- **Linia 10:** Pusty komentarz, odstęp.
- **Linia 11:** Sposób użycia, z głównego katalogu repozytorium.
- **Linia 12:** Przykład polecenia.
- **Linia 13:** Wynikiem jest plik `.docx` obok pliku wejściowego.

### Parametry i ścieżki

```powershell
 15  param(
 16      [Parameter(Mandatory = $true)][string]$Wejscie,
 17      [string]$Wyjscie,
 18      [string]$Szablon
 19  )
```

- **Linia 15:** Początek listy parametrów.
- **Linia 16:** Plik wejściowy, wymagany.
- **Linia 17:** Plik wyjściowy, opcjonalny.
- **Linia 18:** Szablon, opcjonalny.
- **Linia 19:** Koniec listy.

```powershell
 21  $ErrorActionPreference = 'Stop'
 22  Add-Type -AssemblyName System.IO.Compression
 23  Add-Type -AssemblyName System.IO.Compression.FileSystem
 24
 25  $repo = Split-Path -Parent $PSScriptRoot
 26  if (-not $Szablon) { $Szablon = Join-Path $repo 'docs\szablony\SZABLON_SPRAWOZDANIA.docx' }
 27  $Wejscie = (Resolve-Path $Wejscie).Path
 28  if (-not $Wyjscie) { $Wyjscie = [IO.Path]::ChangeExtension($Wejscie, '.docx') }
 29  if (-not [IO.Path]::IsPathRooted($Wyjscie)) { $Wyjscie = Join-Path (Get-Location) $Wyjscie }
```

- **Linia 21:** Każdy błąd przerywa skrypt.
- **Linia 22:** Załaduj bibliotekę .NET do obsługi ZIP.
- **Linia 23:** Załaduj jej część do obsługi plików.
- **Linia 25:** Główny katalog repozytorium: katalog nadrzędny wobec katalogu skryptu.
- **Linia 26:** Jeśli nie podano szablonu, użyj domyślnego.
- **Linia 27:** Zamień ścieżkę wejściową na pełną.
- **Linia 28:** Jeśli nie podano wyjścia, użyj tej samej nazwy z rozszerzeniem `.docx`.
- **Linia 29:** Jeśli ścieżka wyjścia nie jest pełna, doklej bieżący katalog.

### Stałe

```powershell
 31  $SzerokoscTekstu = 10772
 32  $BoxPusty = [string][char]0x2610
 33  $BoxZaznaczony = [string][char]0x2612
```

- **Linia 31:** Szerokość tekstu na stronie, w dwudziestych częściach punktu, czyli jednostce używanej przez Worda.
- **Linia 32:** Znak pustego pola wyboru.
- **Linia 33:** Znak zaznaczonego pola wyboru.

### Budowanie fragmentów XML

```powershell
 35  function Esc([string]$s) { return [Security.SecurityElement]::Escape($s) }
 36
 37  # Kolejnosc elementow w w:rPr wymagana przez schemat OOXML
 38  function RPr([bool]$b, [bool]$i, [bool]$kod, [int]$sz) {
 39      $r = ''
 40      if ($kod) { $r += '<w:rFonts w:ascii="Consolas" w:hAnsi="Consolas" w:cs="Consolas"/>' }
 41      if ($b) { $r += '<w:b/><w:bCs/>' }
 42      if ($i) { $r += '<w:i/><w:iCs/>' }
 43      if ($sz -gt 0) { $r += '<w:sz w:val="' + $sz + '"/><w:szCs w:val="' + $sz + '"/>' }
 44      return $r
 45  }
```

- **Linia 35:** Funkcja zamieniająca znaki specjalne XML, np. `<` na `&lt;`, by nie psuły pliku.
- **Linia 37:** Komentarz: kolejność elementów w `w:rPr` wymusza standard OOXML, czyli format plików Worda.
- **Linia 38:** Funkcja budująca ustawienia fragmentu: pogrubienie, kursywa, kod i rozmiar.
- **Linia 39:** Pusty tekst na początek.
- **Linia 40:** Dla kodu dopisz czcionkę Consolas.
- **Linia 41:** Dla pogrubienia dopisz odpowiedni znacznik.
- **Linia 42:** Dla kursywy dopisz odpowiedni znacznik.
- **Linia 43:** Jeśli podano rozmiar, dopisz go.
- **Linia 44:** Zwróć wynik.
- **Linia 45:** Koniec funkcji.

```powershell
 47  function Run([string]$tekst, [string]$rPr) {
 48      $t = Esc $tekst
 49      if ($rPr) { return '<w:r><w:rPr>' + $rPr + '</w:rPr><w:t xml:space="preserve">' + $t + '</w:t></w:r>' }
 50      return '<w:r><w:t xml:space="preserve">' + $t + '</w:t></w:r>'
 51  }
```

- **Linia 47:** Funkcja budująca fragment tekstu z ustawieniami.
- **Linia 48:** Zabezpiecz znaki specjalne.
- **Linia 49:** Jeśli są ustawienia, zwróć fragment z nimi. `xml:space="preserve"` zachowuje spacje na brzegach.
- **Linia 50:** W przeciwnym razie zwróć fragment bez ustawień.
- **Linia 51:** Koniec funkcji.

### Formatowanie w tekście

```powershell
 53  function Inline([string]$tekst, [bool]$b = $false, [bool]$i = $false, [int]$sz = 0) {
 54      $tekst = [regex]::Replace($tekst, '\[([^\]]+)\]\(([^)]+)\)', '$1')
 55      $sb = New-Object System.Text.StringBuilder
 56      foreach ($c in [regex]::Split($tekst, '(\*\*[^*]+\*\*|`[^`]+`|<br>)')) {
 57          if ($c -eq '') { continue }
 58          if ($c -eq '<br>') { [void]$sb.Append('<w:r><w:br/></w:r>'); continue }
 59          if ($c.Length -gt 4 -and $c.StartsWith('**') -and $c.EndsWith('**')) {
 60              [void]$sb.Append((Run $c.Substring(2, $c.Length - 4) (RPr $true $i $false $sz)))
 61          } elseif ($c.Length -gt 2 -and $c.StartsWith('`') -and $c.EndsWith('`')) {
 62              [void]$sb.Append((Run $c.Substring(1, $c.Length - 2) (RPr $b $i $true $sz)))
 63          } else {
 64              [void]$sb.Append((Run $c (RPr $b $i $false $sz)))
 65          }
 66      }
 67      return $sb.ToString()
 68  }
```

- **Linia 53:** Funkcja zamieniająca tekst z oznaczeniami Markdowna na XML.
- **Linia 54:** Zamień linki `[tekst](adres)` na sam tekst. `$1` to pierwsza grupa z wzorca.
- **Linia 55:** Utwórz obiekt do wydajnego sklejania tekstu.
- **Linia 56:** Podziel tekst na kawałki: `**pogrubione**`, `` `kod` ``, `<br>` i zwykły tekst. Dzięki nawiasom wzorca te oznaczenia zostają w wyniku.
- **Linia 57:** Pomiń puste kawałki.
- **Linia 58:** `<br>` zamień na znacznik łamania wiersza. `[void]` odrzuca wynik, a `continue` przechodzi do następnego kawałka.
- **Linia 59:** Jeśli kawałek to pogrubienie...
- **Linia 60:** ...dopisz go bez gwiazdek, z ustawieniem pogrubienia.
- **Linia 61:** Jeśli kawałek to kod...
- **Linia 62:** ...dopisz go bez odwrotnych apostrofów, z czcionką kodu.
- **Linia 63:** W przeciwnym razie:
- **Linia 64:** Dopisz zwykły tekst.
- **Linia 65:** Koniec warunku.
- **Linia 66:** Koniec pętli.
- **Linia 67:** Zwróć sklejony wynik.
- **Linia 68:** Koniec funkcji.

```powershell
 70  function Akapit([string]$tekst, [string]$pPr = '', [bool]$b = $false, [bool]$i = $false) {
 71      $p = '<w:p>'
 72      if ($pPr) { $p += '<w:pPr>' + $pPr + '</w:pPr>' }
 73      return $p + (Inline $tekst $b $i) + '</w:p>'
 74  }
```

- **Linia 70:** Funkcja budująca akapit.
- **Linia 71:** Otwórz akapit.
- **Linia 72:** Jeśli są ustawienia akapitu, dopisz je.
- **Linia 73:** Dopisz tekst, zamknij akapit i zwróć wynik.
- **Linia 74:** Koniec funkcji.

### Tabele

```powershell
 76  function Wiersz([string]$linia) {
 77      $l = $linia.Trim()
 78      if ($l.StartsWith('|')) { $l = $l.Substring(1) }
 79      if ($l.EndsWith('|')) { $l = $l.Substring(0, $l.Length - 1) }
 80      return @($l.Split('|') | ForEach-Object { $_.Trim() })
 81  }
```

- **Linia 76:** Funkcja dzieląca wiersz tabeli Markdown na komórki.
- **Linia 77:** Usuń spacje z brzegów.
- **Linia 78:** Jeśli na początku jest `|`, usuń go.
- **Linia 79:** Jeśli na końcu jest `|`, usuń go.
- **Linia 80:** Podziel po `|`, usuń spacje z każdej komórki i zwróć tablicę.
- **Linia 81:** Koniec funkcji.

Word potrzebuje szerokości każdej kolumny. Poniższa część wylicza je na podstawie zawartości.

```powershell
 83  function Tabela($wiersze) {
 84      # Szerokosc kolumny: minimum na najdluzsze slowo + reszta proporcjonalnie do dlugosci tekstu
 85      $n = ($wiersze | ForEach-Object { $_.Count } | Measure-Object -Maximum).Maximum
 86      $minima = @()
 87      $wagi = @()
 88      for ($k = 0; $k -lt $n; $k++) {
 89          $maxTekst = 3
 90          $maxSlowo = 3
 91          foreach ($w in $wiersze) {
 92              if ($k -lt $w.Count) {
 93                  foreach ($kawalek in ($w[$k] -split '<br>')) {
 94                      $czysty = $kawalek -replace '\*\*|`', ''
 95                      if ($czysty.Length -gt $maxTekst) { $maxTekst = $czysty.Length }
 96                      foreach ($slowo in $czysty.Split(' ')) {
 97                          if ($slowo.Length -gt $maxSlowo) { $maxSlowo = $slowo.Length }
 98                      }
 99                  }
100              }
101          }
102          $minima += [Math]::Min($maxSlowo * 105 + 220, [int]($SzerokoscTekstu / 2))
103          $wagi += [Math]::Min($maxTekst, 80)
104      }
```

- **Linia 83:** Funkcja budująca tabelę. Dostaje listę wierszy.
- **Linia 84:** Komentarz: szerokość kolumny to minimum na najdłuższe słowo plus reszta proporcjonalnie do długości tekstu.
- **Linia 85:** Liczba kolumn: największa liczba komórek w wierszu.
- **Linia 86:** Lista minimalnych szerokości.
- **Linia 87:** Lista wag.
- **Linia 88:** Dla każdej kolumny...
- **Linia 89:** Najdłuższy tekst, na początek 3 znaki.
- **Linia 90:** Najdłuższe słowo, na początek 3 znaki.
- **Linia 91:** Dla każdego wiersza...
- **Linia 92:** ...jeśli ma tę kolumnę...
- **Linia 93:** Dla każdego wiersza tekstu w komórce...
- **Linia 94:** Usuń oznaczenia formatowania.
- **Linia 95:** Jeśli tekst jest dłuższy, zapamiętaj jego długość.
- **Linia 96:** Dla każdego słowa...
- **Linia 97:** ...jeśli jest dłuższe, zapamiętaj jego długość.
- **Linia 98:** Koniec pętli po słowach.
- **Linia 99:** Koniec pętli po tekstach.
- **Linia 100:** Koniec warunku.
- **Linia 101:** Koniec pętli po wierszach.
- **Linia 102:** Oblicz minimalną szerokość, ale najwyżej połowę szerokości strony.
- **Linia 103:** Oblicz wagę, ale najwyżej 80.
- **Linia 104:** Koniec pętli po kolumnach.

```powershell
105      $sumaMin = ($minima | Measure-Object -Sum).Sum
106      $sumaWag = ($wagi | Measure-Object -Sum).Sum
107      $szer = @()
108      $reszta = $SzerokoscTekstu
109      for ($k = 0; $k -lt $n; $k++) {
110          if ($sumaMin -ge $SzerokoscTekstu) {
111              $s = [int][Math]::Floor($SzerokoscTekstu * $minima[$k] / $sumaMin)
112          } else {
113              $s = [int][Math]::Floor($minima[$k] + ($SzerokoscTekstu - $sumaMin) * $wagi[$k] / $sumaWag)
114          }
115          if ($k -eq $n - 1) { $s = $reszta }
116          $szer += $s
117          $reszta -= $s
118      }
```

- **Linia 105:** Suma minimalnych szerokości.
- **Linia 106:** Suma wag.
- **Linia 107:** Lista ostatecznych szerokości.
- **Linia 108:** Ile szerokości jeszcze zostało.
- **Linia 109:** Dla każdej kolumny...
- **Linia 110:** ...jeśli minima nie mieszczą się na stronie...
- **Linia 111:** ...podziel szerokość proporcjonalnie do minimów.
- **Linia 112:** W przeciwnym razie:
- **Linia 113:** Daj kolumnie jej minimum i część wolnego miejsca według wagi.
- **Linia 114:** Koniec warunku.
- **Linia 115:** Ostatnia kolumna dostaje całą resztę, by suma była dokładna mimo zaokrągleń.
- **Linia 116:** Dopisz szerokość do listy.
- **Linia 117:** Odejmij ją od reszty.
- **Linia 118:** Koniec pętli.

```powershell
120      $x = '<w:tbl><w:tblPr><w:tblStyle w:val="Tabela-Siatka"/><w:tblW w:w="' + $SzerokoscTekstu + '" w:type="dxa"/>'
121      $x += '<w:tblLayout w:type="fixed"/><w:tblLook w:val="04A0" w:firstRow="1" w:lastRow="0" w:firstColumn="1" w:lastColumn="0" w:noHBand="0" w:noVBand="1"/></w:tblPr><w:tblGrid>'
122      foreach ($s in $szer) { $x += '<w:gridCol w:w="' + $s + '"/>' }
123      $x += '</w:tblGrid>'
124      for ($r = 0; $r -lt $wiersze.Count; $r++) {
125          $naglowek = ($r -eq 0)
126          $x += '<w:tr>'
127          if ($naglowek) { $x += '<w:trPr><w:tblHeader/></w:trPr>' }
128          for ($k = 0; $k -lt $n; $k++) {
129              $tekst = ''
130              if ($k -lt $wiersze[$r].Count) { $tekst = $wiersze[$r][$k] }
131              $x += '<w:tc><w:tcPr><w:tcW w:w="' + $szer[$k] + '" w:type="dxa"/>'
132              if ($naglowek) { $x += '<w:shd w:val="clear" w:color="auto" w:fill="D9E2F3"/>' }
133              $x += '</w:tcPr><w:p><w:pPr><w:spacing w:before="20" w:after="20"/></w:pPr>'
134              $x += (Inline $tekst $naglowek $false 20) + '</w:p></w:tc>'
135          }
136          $x += '</w:tr>'
137      }
138      return $x + '</w:tbl><w:p><w:pPr><w:spacing w:after="0"/></w:pPr></w:p>'
139  }
```

- **Linia 120:** Rozpocznij tabelę: styl siatki i pełna szerokość.
- **Linia 121:** Stały układ kolumn, informacja o wyróżnieniu pierwszego wiersza i początek opisu kolumn.
- **Linia 122:** Dopisz szerokość każdej kolumny.
- **Linia 123:** Koniec opisu kolumn.
- **Linia 124:** Dla każdego wiersza...
- **Linia 125:** Pierwszy wiersz jest nagłówkiem.
- **Linia 126:** Otwórz wiersz.
- **Linia 127:** Nagłówek ma się powtarzać na kolejnych stronach.
- **Linia 128:** Dla każdej kolumny...
- **Linia 129:** Pusty tekst na początek.
- **Linia 130:** Jeśli wiersz ma tę kolumnę, weź jej tekst.
- **Linia 131:** Otwórz komórkę i podaj jej szerokość.
- **Linia 132:** Komórka nagłówka ma niebieskie tło.
- **Linia 133:** Zamknij ustawienia komórki i otwórz akapit z małymi odstępami.
- **Linia 134:** Dopisz tekst rozmiarem 10 pt. Nagłówek jest pogrubiony. Zamknij akapit i komórkę.
- **Linia 135:** Koniec pętli po kolumnach.
- **Linia 136:** Zamknij wiersz.
- **Linia 137:** Koniec pętli po wierszach.
- **Linia 138:** Zamknij tabelę, dopisz pusty akapit i zwróć wynik.
- **Linia 139:** Koniec funkcji.

### Bloki kodu

```powershell
141  function Kod([string]$linia) {
142      $pPr = '<w:shd w:val="clear" w:color="auto" w:fill="F2F2F2"/><w:spacing w:after="0" w:line="240" w:lineRule="auto"/>'
143      return '<w:p><w:pPr>' + $pPr + '</w:pPr>' + (Run $linia (RPr $false $false $true 18)) + '</w:p>'
144  }
```

- **Linia 141:** Funkcja budująca jedną linię kodu.
- **Linia 142:** Ustawienia akapitu: szare tło, bez odstępu, pojedyncza interlinia.
- **Linia 143:** Zwróć akapit z tekstem czcionką kodu, rozmiarem 9 pt.
- **Linia 144:** Koniec funkcji.

### Wczytanie pliku

```powershell
146  # --- Wczytanie Markdown
147  $linie = [IO.File]::ReadAllLines($Wejscie, [Text.Encoding]::UTF8)
148  $meta = @{}
149  $start = 0
150  if ($linie.Count -gt 0 -and $linie[0].Trim() -eq '---') {
151      for ($j = 1; $j -lt $linie.Count; $j++) {
152          if ($linie[$j].Trim() -eq '---') { $start = $j + 1; break }
153          if ($linie[$j] -match '^\s*([A-Za-z]+)\s*:\s*(.*)$') { $meta[$Matches[1].ToLower()] = $Matches[2].Trim().Trim('"') }
154      }
155  }
```

- **Linia 146:** Komentarz oddzielający tę część.
- **Linia 147:** Wczytaj wszystkie linie jako UTF-8.
- **Linia 148:** Pusty słownik na dane z front matter.
- **Linia 149:** Numer linii, od której zaczyna się treść.
- **Linia 150:** Jeśli pierwsza linia to `---`, to jest front matter:
- **Linia 151:** Przejdź po kolejnych liniach.
- **Linia 152:** Jeśli to zamykające `---`, zapamiętaj początek treści i przerwij pętlę.
- **Linia 153:** Jeśli linia ma postać `klucz: wartość`, zapisz ją w słowniku. Klucz zamień na małe litery, z wartości usuń cudzysłowy.
- **Linia 154:** Koniec pętli.
- **Linia 155:** Koniec warunku.

### Budowa treści

```powershell
157  # --- Budowa tresci
158  $body = New-Object System.Text.StringBuilder
159  $akapit = @()
160  $tabela = @()
161  $wKodzie = $false
162
163  function Wypisz-Akapit {
164      if ($script:akapit.Count -gt 0) {
165          [void]$script:body.Append((Akapit ($script:akapit -join ' ')))
166          $script:akapit = @()
167      }
168  }
169  function Wypisz-Tabele {
170      if ($script:tabela.Count -gt 0) {
171          [void]$script:body.Append((Tabela $script:tabela))
172          $script:tabela = @()
173      }
174  }
```

- **Linia 157:** Komentarz oddzielający tę część.
- **Linia 158:** Obiekt, w którym powstaje XML.
- **Linia 159:** Lista linii bieżącego akapitu.
- **Linia 160:** Lista wierszy bieżącej tabeli.
- **Linia 161:** Czy jesteśmy wewnątrz bloku kodu.
- **Linia 163:** Funkcja zamykająca bieżący akapit.
- **Linia 164:** Jeśli akapit ma jakieś linie. `$script:` oznacza zmienną skryptu, a nie funkcji...
- **Linia 165:** ...sklej je spacjami i dopisz akapit.
- **Linia 166:** Wyczyść listę.
- **Linia 167:** Koniec warunku.
- **Linia 168:** Koniec funkcji.
- **Linia 169:** Funkcja zamykająca bieżącą tabelę.
- **Linia 170:** Jeśli tabela ma jakieś wiersze...
- **Linia 171:** ...dopisz ją.
- **Linia 172:** Wyczyść listę.
- **Linia 173:** Koniec warunku.
- **Linia 174:** Koniec funkcji.

### Główna pętla po liniach

````powershell
176  for ($j = $start; $j -lt $linie.Count; $j++) {
177      $l = $linie[$j]
178
179      if ($l.Trim().StartsWith('```')) {
180          Wypisz-Akapit; Wypisz-Tabele
181          $wKodzie = -not $wKodzie
182          if (-not $wKodzie) { [void]$body.Append('<w:p><w:pPr><w:spacing w:after="0"/></w:pPr></w:p>') }
183          continue
184      }
185      if ($wKodzie) { [void]$body.Append((Kod $l)); continue }
186      if ($l -match '^\s*<!--.*-->\s*$') { continue }
````

- **Linia 176:** Przejdź po liniach, zaczynając od treści.
- **Linia 177:** Weź bieżącą linię.
- **Linia 179:** Jeśli linia zaczyna się od potrójnego odwrotnego apostrofu, to początek lub koniec bloku kodu.
- **Linia 180:** Zamknij otwarty akapit i tabelę.
- **Linia 181:** Odwróć znacznik „w kodzie”.
- **Linia 182:** Jeśli blok właśnie się skończył, dopisz pusty akapit.
- **Linia 183:** Przejdź do następnej linii.
- **Linia 184:** Koniec warunku.
- **Linia 185:** Wewnątrz bloku kodu: dopisz linię jako kod i przejdź dalej.
- **Linia 186:** Pomiń jednoliniowe komentarze HTML.

```powershell
188      if ($l.Trim().StartsWith('|')) {
189          Wypisz-Akapit
190          if ($l -notmatch '^\s*\|?\s*:?-{3,}') { $tabela += , (Wiersz $l) }
191          continue
192      }
193      Wypisz-Tabele
```

- **Linia 188:** Jeśli linia zaczyna się od `|`, to wiersz tabeli:
- **Linia 189:** Zamknij otwarty akapit.
- **Linia 190:** Jeśli to nie jest linia z kreskami oddzielająca nagłówek, dopisz wiersz. Przecinek sprawia, że wiersz trafia na listę jako jeden element.
- **Linia 191:** Przejdź do następnej linii.
- **Linia 192:** Koniec warunku.
- **Linia 193:** Każda inna linia kończy tabelę.

```powershell
195      if ($l.Trim() -eq '' -or $l.Trim() -eq '---') { Wypisz-Akapit; continue }
196      if ($l -match '^#\s') { Wypisz-Akapit; continue }
197      if ($l -match '^##\s+(.*)$') {
198          Wypisz-Akapit
199          [void]$body.Append((Akapit $Matches[1] '<w:pStyle w:val="Nagwek2"/>'))
200          continue
201      }
202      if ($l -match '^###\s+(.*)$') {
203          Wypisz-Akapit
204          [void]$body.Append((Akapit $Matches[1] '<w:keepNext/><w:spacing w:before="160" w:after="80"/>' $true))
205          continue
206      }
```

- **Linia 195:** Pusta linia lub `---` kończy akapit.
- **Linia 196:** Tytuł pliku `# ` jest pomijany.
- **Linia 197:** Nagłówek `## `:
- **Linia 198:** Zamknij akapit.
- **Linia 199:** Dopisz akapit ze stylem nagłówka z szablonu. `$Matches[1]` to tekst nagłówka.
- **Linia 200:** Przejdź do następnej linii.
- **Linia 201:** Koniec warunku.
- **Linia 202:** Podnagłówek `### `:
- **Linia 203:** Zamknij akapit.
- **Linia 204:** Dopisz pogrubiony akapit, który nie oddzieli się od następnego na końcu strony.
- **Linia 205:** Przejdź do następnej linii.
- **Linia 206:** Koniec warunku.

```powershell
207      if ($l -match '^(\s*)[-*]\s+(.*)$') {
208          Wypisz-Akapit
209          $poziom = [Math]::Min([int][Math]::Floor($Matches[1].Length / 2), 1)
210          $tekst = $Matches[2] -replace '^\[ \]\s*', ($BoxPusty + ' ') -replace '^\[[xX]\]\s*', ($BoxZaznaczony + ' ')
211          $pPr = '<w:numPr><w:ilvl w:val="' + $poziom + '"/><w:numId w:val="2"/></w:numPr><w:spacing w:after="40"/>'
212          [void]$body.Append((Akapit $tekst $pPr))
213          continue
214      }
```

- **Linia 207:** Element listy `- ` lub `* `:
- **Linia 208:** Zamknij akapit.
- **Linia 209:** Oblicz poziom: wcięcie podzielone przez 2, najwyżej 1.
- **Linia 210:** Zamień `[ ]` i `[x]` na znaki pól wyboru.
- **Linia 211:** Ustawienia akapitu: lista z szablonu, na obliczonym poziomie.
- **Linia 212:** Dopisz akapit.
- **Linia 213:** Przejdź do następnej linii.
- **Linia 214:** Koniec warunku.

```powershell
215      if ($l -match '^>\s?(.*)$') {
216          Wypisz-Akapit
217          [void]$body.Append((Akapit $Matches[1] '<w:ind w:left="360"/>' $false $true))
218          continue
219      }
220      if ($l -match '^\d+\.\s') {
221          Wypisz-Akapit
222          [void]$body.Append((Akapit $l.Trim() '<w:spacing w:after="40"/>'))
223          continue
224      }
```

- **Linia 215:** Cytat `> `:
- **Linia 216:** Zamknij akapit.
- **Linia 217:** Dopisz akapit z wcięciem, kursywą.
- **Linia 218:** Przejdź do następnej linii.
- **Linia 219:** Koniec warunku.
- **Linia 220:** Lista numerowana:
- **Linia 221:** Zamknij akapit.
- **Linia 222:** Dopisz linię jako akapit, razem z numerem.
- **Linia 223:** Przejdź do następnej linii.
- **Linia 224:** Koniec warunku.

```powershell
225      $akapit += $l.Trim()
226  }
227  Wypisz-Akapit
228  Wypisz-Tabele
```

- **Linia 225:** Każda inna linia to fragment akapitu.
- **Linia 226:** Koniec pętli.
- **Linia 227:** Zamknij akapit, jeśli został otwarty.
- **Linia 228:** Zamknij tabelę, jeśli została otwarta.

### Odczyt szablonu

```powershell
230  # --- Wstawienie do szablonu
231  $zip = [IO.Compression.ZipFile]::OpenRead($Szablon)
232  try {
233      $czytnik = New-Object IO.StreamReader($zip.GetEntry('word/document.xml').Open(), [Text.Encoding]::UTF8)
234      $xml = $czytnik.ReadToEnd()
235      $czytnik.Close()
236  } finally { $zip.Dispose() }
```

- **Linia 230:** Komentarz oddzielający tę część.
- **Linia 231:** Otwórz szablon jako archiwum ZIP.
- **Linia 232:** Próbuj.
- **Linia 233:** Utwórz obiekt czytający plik `word/document.xml`.
- **Linia 234:** Odczytaj cały tekst.
- **Linia 235:** Zamknij czytanie.
- **Linia 236:** Na końcu, także przy błędzie, zamknij archiwum.

### Wypełnienie nagłówka

Szablon ma w nagłówku cztery puste pola: zespół, etap, nazwa, data.

```powershell
238  $pola = @('zespol', 'etap', 'nazwa', 'data')
239  $puste = [regex]::Matches($xml, '(<w:p [^>]*>)<w:pPr><w:rPr><w:sz w:val="32"/></w:rPr></w:pPr></w:p>')
240  if ($puste.Count -ne 4) { throw 'Szablon ma inna strukture naglowka niz oczekiwano (4 pola).' }
241  for ($k = 3; $k -ge 0; $k--) {
242      $m = $puste[$k]
243      $wartosc = ''
244      if ($meta.ContainsKey($pola[$k])) { $wartosc = $meta[$pola[$k]] }
245      $nowy = $m.Groups[1].Value + '<w:pPr><w:rPr><w:sz w:val="32"/></w:rPr></w:pPr>' + (Run $wartosc (RPr $false $false $false 32)) + '</w:p>'
246      $xml = $xml.Substring(0, $m.Index) + $nowy + $xml.Substring($m.Index + $m.Length)
247  }
248  if ($meta.ContainsKey('tytul')) { $xml = $xml.Replace('<w:t>SPRAWOZDANIE</w:t>', '<w:t>' + (Esc $meta['tytul']) + '</w:t>') }
```

- **Linia 238:** Nazwy tych pól.
- **Linia 239:** Znajdź puste akapity o rozmiarze czcionki 16 pt.
- **Linia 240:** Jeśli nie ma ich dokładnie cztery, przerwij skrypt, bo szablon się zmienił.
- **Linia 241:** Przejdź po nich od końca. Dzięki temu wstawienie tekstu nie zmienia pozycji wcześniejszych pól.
- **Linia 242:** Weź bieżące dopasowanie.
- **Linia 243:** Pusta wartość na początek.
- **Linia 244:** Jeśli front matter zawiera to pole, weź jego wartość.
- **Linia 245:** Zbuduj akapit z wartością.
- **Linia 246:** Wstaw go do XML: tekst przed polem, nowy akapit, tekst po polu.
- **Linia 247:** Koniec pętli.
- **Linia 248:** Jeśli podano własny tytuł, podmień napis `SPRAWOZDANIE`.

### Wstawienie treści

```powershell
250  $koniecTabeli = $xml.IndexOf('</w:tbl>') + '</w:tbl>'.Length
251  $sekcja = $xml.IndexOf('<w:sectPr')
252  $podpis = '<w:p/><w:p/>' + (Akapit 'data i podpis: ....................................................')
253  $xml = $xml.Substring(0, $koniecTabeli) + '<w:p/>' + $body.ToString() + $podpis + $xml.Substring($sekcja)
254
255  [void][xml]$xml
```

- **Linia 250:** Znajdź koniec tabeli nagłówkowej.
- **Linia 251:** Znajdź ustawienia sekcji, które są na końcu dokumentu.
- **Linia 252:** Zbuduj miejsce na podpis.
- **Linia 253:** Złóż całość: początek szablonu, treść, podpis i końcówka szablonu.
- **Linia 255:** Spróbuj odczytać wynik jako XML. Jeśli jest błędny, skrypt przerwie się tutaj, a nie zapisze uszkodzonego pliku.

### Zapis pliku

```powershell
257  Copy-Item $Szablon $Wyjscie -Force
258  $zip = [IO.Compression.ZipFile]::Open($Wyjscie, [IO.Compression.ZipArchiveMode]::Update)
259  try {
260      $zip.GetEntry('word/document.xml').Delete()
261      $strumien = $zip.CreateEntry('word/document.xml', [IO.Compression.CompressionLevel]::Optimal).Open()
262      $bajty = (New-Object Text.UTF8Encoding $false).GetBytes($xml)
263      $strumien.Write($bajty, 0, $bajty.Length)
264      $strumien.Close()
265  } finally { $zip.Dispose() }
266
267  Write-Host "Zapisano: $Wyjscie"
```

- **Linia 257:** Skopiuj szablon pod nazwę wyjściową.
- **Linia 258:** Otwórz kopię jako archiwum do edycji.
- **Linia 259:** Próbuj.
- **Linia 260:** Usuń stary plik `word/document.xml` z archiwum.
- **Linia 261:** Utwórz nowy plik o tej samej nazwie i otwórz go do zapisu.
- **Linia 262:** Zamień tekst na bajty UTF-8, bez znacznika BOM na początku.
- **Linia 263:** Zapisz bajty.
- **Linia 264:** Zamknij zapis.
- **Linia 265:** Na końcu, także przy błędzie, zamknij archiwum.
- **Linia 267:** Wypisz, gdzie zapisano plik.

---

[← Poprzedni](70-narzedzia-github-tymczasowo.md) · [Spis treści](README.md) · [Następny →](72-narzedzia-ntc-table.md)
