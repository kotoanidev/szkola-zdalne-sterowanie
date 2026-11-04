# Narzędzia: `ntc_table.py`

[← Poprzedni](71-narzedzia-md2docx.md) · [Spis treści](README.md) · [Następny →](80-repo-konfiguracja.md)

**Po co ten plik:** generuje tabelę z `firmware/src/sensors.c`, używaną dla czujnika NTC. Firmware nie umie sprawnie liczyć wykładniczego wzoru, dlatego Python wylicza wartości z wyprzedzeniem, a my wklejamy je do kodu. Składnia jest w [podstawach Pythona](02-podstawy-pythona.md).

**Układ pomiarowy:** termistor NTC i zwykły rezystor tworzą **dzielnik napięcia**. Napięcie w ich połączeniu zależy od oporu termistora, a ten od temperatury.

## `tools/ntc_table.py`

[Otwórz ten plik w repozytorium](../../tools/ntc_table.py)

```python
  1  """Tabela przeliczenia NTC dla firmware (src/sensors.c).
  2
  3  Dzielnik: NTC do VCC, rezystor R_SERIES do GND, pomiar na ADC2.
  4  Wartości tabeli to suma 16 próbek ADC (jak zwraca adc_sum()).
  5
  6      python tools/ntc_table.py [--beta 3950] [--r25 10000] [--rs 10000]
  7  """
```

- **Linia 1:** Początek docstringa: tabela przeliczenia NTC dla firmware.
- **Linia 3:** Dzielnik: NTC do zasilania, rezystor `R_SERIES` do masy, pomiar na kanale ADC2.
- **Linia 4:** Wartości tabeli to suma 16 próbek ADC, taka jak zwraca `adc_sum()`.
- **Linia 6:** Sposób użycia. Parametry w nawiasach są opcjonalne.
- **Linia 7:** Koniec docstringa.

```python
  9  import argparse
 10  import math
 11
 12  T_MIN, T_MAX, T_STEP = -20, 125, 5
 13  SAMPLES = 16
```

- **Linia 9:** Biblioteka do odczytu parametrów z wiersza poleceń.
- **Linia 10:** Biblioteka matematyczna.
- **Linia 12:** Trzy stałe naraz: tabela biegnie od -20 °C do 125 °C, co 5 °C.
- **Linia 13:** Na każdy pomiar przypada 16 próbek.

```python
 16  def adc_sum(temp_c: float, beta: float, r25: float, rs: float) -> int:
 17      t = temp_c + 273.15
 18      r_ntc = r25 * math.exp(beta * (1 / t - 1 / 298.15))
 19      return round(1024 * SAMPLES * rs / (rs + r_ntc))
```

- **Linia 16:** Funkcja oblicza sumę odczytów ADC dla danej temperatury.
- **Linia 17:** Zamień stopnie Celsjusza na kelwiny.
- **Linia 18:** Oblicz opór termistora ze wzoru na parametr B (`beta`). 298,15 K to 25 °C, a `r25` to opór w tej temperaturze.
- **Linia 19:** Oblicz napięcie dzielnika jako część zakresu i pomnóż przez 1024 działki oraz 16 próbek. Zaokrąglij wynik.

```python
 22  def main() -> None:
 23      p = argparse.ArgumentParser()
 24      p.add_argument("--beta", type=float, default=3950)
 25      p.add_argument("--r25", type=float, default=10000)
 26      p.add_argument("--rs", type=float, default=10000)
 27      a = p.parse_args()
```

- **Linia 22:** Funkcja główna.
- **Linia 23:** Utwórz odczyt parametrów.
- **Linia 24:** Parametr `--beta`, liczba, domyślnie 3950.
- **Linia 25:** Parametr `--r25`, liczba, domyślnie 10 000 Ω.
- **Linia 26:** Parametr `--rs`, liczba, domyślnie 10 000 Ω.
- **Linia 27:** Odczytaj podane parametry.

```python
 29      values = [adc_sum(t, a.beta, a.r25, a.rs) for t in range(T_MIN, T_MAX + 1, T_STEP)]
 30      print(f"// B = {a.beta:g}, R25 = {a.r25:g} Ω, Rs = {a.rs:g} Ω; od {T_MIN} °C co {T_STEP} °C")
 31      for i in range(0, len(values), 8):
 32          print("    " + ", ".join(str(v) for v in values[i:i + 8]) + ",")
```

- **Linia 29:** Oblicz wartość dla każdej temperatury i zbierz je w liście.
- **Linia 30:** Wypisz komentarz C z użytymi parametrami. `:g` skraca zapis liczby, np. `3950.0` do `3950`.
- **Linia 31:** Przechodź po liście co 8 elementów.
- **Linia 32:** Wypisz wiersz: wcięcie, 8 wartości rozdzielonych przecinkami i przecinek na końcu.

```python
 35  if __name__ == "__main__":
 36      main()
```

- **Linia 35:** Warunek prawdziwy tylko wtedy, gdy plik uruchomiono bezpośrednio.
- **Linia 36:** Wywołaj funkcję główną.

---

[← Poprzedni](71-narzedzia-md2docx.md) · [Spis treści](README.md) · [Następny →](80-repo-konfiguracja.md)
