# Podstawy narzędzi: terminal, Makefile, PowerShell i Git

[← Poprzedni](03-podstawy-www.md) · [Spis treści](README.md) · [Następny →](10-firmware-makefile.md)

## Terminal

**Terminal** to okno, w którym wydaje się polecenia tekstem zamiast klikać. Polecenie wygląda zwykle tak:

```text
program argument1 --opcja wartosc
```

- `cd firmware`: wejdź do katalogu `firmware`,
- `mkdir -p build`: utwórz katalog `build`, jeśli jeszcze go nie ma,
- `rm -rf build`: usuń katalog `build` z całą zawartością,
- `git clone adres`: pobierz repozytorium.

**Zmienna środowiskowa** to ustawienie przekazywane programowi przez system. Polecenie `SIMULATE=1 python -m app` uruchamia program z ustawioną zmienną `SIMULATE`.

**stdin**, **stdout** i **stderr** to trzy standardowe „rury” każdego programu: wejście, wyjście oraz wyjście na komunikaty o błędach. Program może czytać i pisać przez nie tak, jak przez klawiaturę i ekran. Inny program może się do nich podłączyć.

## Kompilator i wgrywanie

Budowa firmware przebiega kilkoma krokami:

```text
main.c, uart.c, ...   pliki z kodem C
      │ avr-gcc -c    kompilacja każdego pliku osobno
      ▼
main.o, uart.o, ...   pliki pośrednie (kod maszynowy, jeszcze niepołączony)
      │ avr-gcc       łączenie, czyli linkowanie
      ▼
firmware.elf          gotowy program z dodatkowymi informacjami
      │ avr-objcopy   wyciągnięcie samego programu
      ▼
firmware.hex          plik do wgrania
      │ avrdude       wysłanie przez programator USBasp
      ▼
pamięć flash układu ATmega8
```

**Linker** łączy osobne części w jeden program. **Programator USBasp** to urządzenie, które zapisuje program w układzie. **Fusebity** to specjalne ustawienia układu, np. źródło zegara. Zła wartość może go zablokować, dlatego w tym projekcie nie są zapisywane.

## Makefile

**Make** to narzędzie, które buduje program według przepisu z pliku `Makefile`. Sprawdza daty plików i robi tylko to, co się zmieniło. Reguła wygląda tak:

```makefile
cel: zależność1 zależność2
	polecenie
```

Czytamy to: „aby zrobić `cel`, najpierw przygotuj zależności, a potem uruchom polecenie”. Polecenie **musi** zaczynać się od znaku tabulacji. Uruchomienie `make cel` wykonuje daną regułę, a samo `make` wykonuje pierwszą regułę w pliku.

| Zapis | Znaczenie |
|---|---|
| `# tekst` | komentarz |
| `NAZWA = wartość` | zmienna; wartość jest wyliczana przy każdym użyciu |
| `NAZWA := wartość` | zmienna; wartość jest wyliczana od razu, jeden raz |
| `NAZWA ?= wartość` | ustaw tylko wtedy, gdy zmienna jeszcze nie ma wartości |
| `$(NAZWA)` | wstaw wartość zmiennej |
| `\` na końcu linii | polecenie lub wartość ciągnie się dalej w następnej linii |
| `$@` | nazwa celu |
| `$<` | pierwsza zależność |
| `$^` | wszystkie zależności |
| `%` | dowolny tekst, np. `%.o: %.c` to reguła dla wszystkich par plików |
| `cel: a \| b` | `b` musi istnieć, ale jego data nie ma znaczenia (zależność tylko na kolejność) |
| `$(MAKE)` | uruchom `make` jeszcze raz, np. w innym katalogu |
| `@polecenie` | nie wypisuj samego polecenia przed jego wykonaniem |
| `-include plik` | dołącz plik, a jeśli go nie ma, nie zgłaszaj błędu |
| `.PHONY: a b` | `a` i `b` to nazwy czynności, a nie plików |

Funkcje Make:

| Funkcja | Znaczenie |
|---|---|
| `$(wildcard wzorzec)` | lista istniejących plików pasujących do wzorca |
| `$(patsubst wzór,zamiana,lista)` | zamień każdy element listy według wzoru |
| `$(subst a,b,tekst)` | zamień wszystkie `a` na `b` |
| `$(firstword lista)`, `$(lastword lista)` | pierwszy lub ostatni element listy |
| `$(dir ścieżka)` | sam katalog ze ścieżki |
| `$(shell polecenie)` | wynik polecenia terminala |
| `$(X:.o=.d)` | wartość `X` z końcówką `.o` zamienioną na `.d` |

## PowerShell

**PowerShell** to język poleceń systemu Windows. Skrypty mają rozszerzenie `.ps1`.

| Zapis | Znaczenie |
|---|---|
| `# tekst` | komentarz |
| `$x = 5` | zmienna |
| `$env:TEMP` | zmienna środowiskowa |
| `$true`, `$false`, `$null` | prawda, fałsz, brak wartości |
| `$a -eq $b`, `-ne`, `-gt`, `-lt`, `-ge` | równe, różne, większe, mniejsze, większe lub równe |
| `-not $a` | logiczne „nie” |
| `-and`, `-or` | logiczne „i”, „lub” |
| `$a -match 'wzorzec'` | czy tekst pasuje do wyrażenia regularnego; dopasowane części trafiają do `$Matches` |
| `$a -replace 'x', 'y'` | zamień w tekście |
| `$a -split ' '` | podziel tekst |
| `$a -join ' '` | sklej listę w tekst |
| `@(1, 2)` | tablica |
| `@{}` | słownik |
| `$a += 1` | dopisz do tablicy lub dodaj do liczby |
| `[int]$x` | zamień lub potraktuj jako dany typ |
| `[Math]::Min(a, b)` | wywołaj funkcję z biblioteki .NET |
| `$obj.Metoda()` | wywołaj metodę obiektu |
| `komenda \| inna` | **potok**: wyjście pierwszego polecenia idzie na wejście drugiego |
| `$_` | bieżący element w potoku |
| `Where-Object { ... }` | zostaw elementy spełniające warunek |
| `ForEach-Object { ... }` | zrób coś z każdym elementem |
| `& $program argumenty` | uruchom program, którego ścieżka jest w zmiennej |
| `function Nazwa { ... }` | definicja funkcji |
| `try { } catch { } finally { }` | obsługa błędów |
| `throw 'tekst'` | zgłoś błąd |

Funkcje wywołuje się bez nawiasów i przecinków: `Run $tekst $rPr`. Wszystko, co funkcja wyprodukuje, staje się jej wynikiem, dlatego przy niepotrzebnych wynikach stosuje się `[void]` albo `| Out-Null`.

**Wyrażenie regularne** to wzorzec opisujący tekst. Na przykład `^\d+` to „na początku jedna lub więcej cyfr”. Podstawowe znaki:

| Znak | Znaczenie |
|---|---|
| `^`, `$` | początek i koniec tekstu |
| `\d`, `\s`, `\S` | cyfra, biały znak, niebiały znak |
| `.` | dowolny znak |
| `*`, `+`, `?` | zero lub więcej, jeden lub więcej, zero lub jeden |
| `[abc]`, `[^abc]` | jeden ze znaków, dowolny inny znak |
| `( )` | grupa, którą można potem odczytać |
| `a\|b` | `a` albo `b` |

## Git

**Git** to system zapisywania historii kodu.

- **Repozytorium** to projekt razem z całą historią.
- **Commit** to zapisany stan plików z opisem.
- **Gałąź** to osobna linia pracy, `main` to gałąź główna.
- **Push** wysyła commity na serwer (GitHub), a **pull** je pobiera.
- **Token** to tymczasowe hasło dla programów.

Plik `.gitignore` zawiera listę plików, których Git ma nie śledzić, np. wyników kompilacji i haseł.

---

[← Poprzedni](03-podstawy-www.md) · [Spis treści](README.md) · [Następny →](10-firmware-makefile.md)
