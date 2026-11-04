# Firmware: `sensors.c`, przeliczanie pomiarów

[← Poprzedni](17-firmware-adc.md) · [Spis treści](README.md) · [Następny →](19-firmware-button.md)

**Po co ten plik:** przetwornik podaje surowe liczby. Ten moduł zamienia je na wielkości, które coś znaczą: temperaturę w stopniach, położenie potencjometru w procentach i jasność dla trybu AUTO. Wszystko liczymy na liczbach całkowitych, bo ułamki są dla tego układu bardzo kosztowne.

## `firmware/src/sensors.c`

[Otwórz ten plik w repozytorium](../../firmware/src/sensors.c)

```c
  1  // Przeliczanie pomiarów ADC: temperatura, potencjometr, jasność w trybie AUTO
  2  #include <avr/pgmspace.h>
  3  #include "config.h"
  4  #include "pinout.h"
  5  #include "adc.h"
  6  #include "sensors.h"
```

- **Linia 1:** Komentarz: przeliczanie pomiarów na temperaturę, potencjometr i jasność w trybie AUTO.
- **Linia 2:** Biblioteka dostępu do pamięci flash, tu przechowujemy tabelę NTC.
- **Linia 3:** Nasze ustawienia.
- **Linia 4:** Nasze przypisanie pinów, m.in. numery kanałów.
- **Linia 5:** Nasz moduł pomiarów ADC.
- **Linia 6:** Nasz nagłówek.

### Tabela dla czujnika NTC

Termistor NTC ma charakterystykę nieliniową, nie da się jej opisać prostym mnożeniem. Dlatego z wyprzedzeniem wyliczamy tabelę: co 5 °C, jaka powinna być suma pomiarów. Ta część jest kompilowana tylko wtedy, gdy wybrano ten czujnik.

```c
  8  #if TEMP_SENSOR == TEMP_SENSOR_NTC
  9  #define NTC_T_MIN  -20
 10  #define NTC_T_STEP 5
 11
 12  // Wygenerowane: python tools/ntc_table.py (B = 3950, R25 = 10 kΩ, Rs = 10 kΩ)
 13  static const uint16_t ntc_table[] PROGMEM = {
 14      1420, 1864, 2401, 3033, 3756, 4561, 5430, 6341,
 15      7270, 8192, 9083, 9926, 10707, 11419, 12058, 12624,
 16      13122, 13556, 13932, 14257, 14537, 14778, 14985, 15163,
 17      15316, 15447, 15561, 15658, 15743, 15816,
 18  };
 19  #define NTC_COUNT ((uint8_t)(sizeof(ntc_table) / sizeof(ntc_table[0])))
 20  #endif
```

- **Linia 8:** Poniższy fragment zostanie użyty tylko, gdy w `config.h` wybrano czujnik NTC.
- **Linia 9:** Pierwsza pozycja tabeli odpowiada -20 °C.
- **Linia 10:** Kolejne pozycje są co 5 °C.
- **Linia 12:** Komentarz: tabelę wygenerował skrypt `tools/ntc_table.py` dla parametrów termistora podanych w nawiasie.
- **Linia 13:** Początek tabeli 16-bitowych liczb. `const`: stała. `PROGMEM`: trzymana we flash, bo RAM jest bardzo mały. `[]`: rozmiar wynika z liczby elementów.
- **Linia 14:** Sumy ADC dla -20, -15, -10, -5, 0, 5, 10 i 15 °C.
- **Linia 15:** Sumy ADC dla 20, 25, 30, 35, 40, 45, 50 i 55 °C.
- **Linia 16:** Sumy ADC dla 60, 65, 70, 75, 80, 85, 90 i 95 °C.
- **Linia 17:** Sumy ADC dla 100, 105, 110, 115, 120 i 125 °C.
- **Linia 18:** Koniec tabeli.
- **Linia 19:** Liczba elementów tabeli: rozmiar całości podzielony przez rozmiar jednego elementu.
- **Linia 20:** Koniec warunku z linii 8.

### Temperatura

```c
 22  int16_t sensors_temp_tenths(void)
 23  {
 24      uint16_t sum = adc_sum(ADC_CH_TEMP);
 25
 26  #if TEMP_SENSOR == TEMP_SENSOR_LM35
 27      // 10 mV/°C, czyli 1 mV = 0,1 °C; suma 16 próbek = 2^14 działek na VREF
 28      return (int16_t)(((uint32_t)sum * ADC_VREF_MV + 8192) >> 14);
```

- **Linia 22:** Funkcja zwraca temperaturę w dziesiątych częściach stopnia.
- **Linia 23:** Początek funkcji.
- **Linia 24:** Pobierz sumę 16 próbek kanału temperatury.
- **Linia 26:** Poniższy kod dotyczy tylko czujnika LM35.
- **Linia 27:** Komentarz: czujnik daje 10 mV na stopień, więc 1 mV to 0,1 °C. Suma 16 próbek to 16 × 1024 = 16 384 = 2^14 działek na całe napięcie odniesienia.
- **Linia 28:** Przelicza sumę na miliwolty, co od razu jest liczbą dziesiątych stopnia. Mnoży przez 5000 mV i dzieli przez 2^14, przesuwając o 14 bitów. Dodanie 8192 (połowy dzielnika) daje zaokrąglenie. Mnożenie wymaga 32 bitów.

```c
 29  #elif TEMP_SENSOR == TEMP_SENSOR_NTC
 30      uint16_t lo = pgm_read_word(&ntc_table[0]);
 31      uint8_t i;
 32
 33      if (sum <= lo)
 34          return NTC_T_MIN * 10;
 35      for (i = 1; i < NTC_COUNT; i++) {
 36          uint16_t hi = pgm_read_word(&ntc_table[i]);
 37          if (sum < hi)
 38              return (int16_t)((NTC_T_MIN + NTC_T_STEP * (i - 1)) * 10
 39                               + (int16_t)((uint32_t)(sum - lo) * NTC_T_STEP * 10 / (hi - lo)));
 40          lo = hi;
 41      }
 42      return (NTC_T_MIN + NTC_T_STEP * (NTC_COUNT - 1)) * 10;
```

- **Linia 29:** W przeciwnym razie, dla czujnika NTC:
- **Linia 30:** Pobierz z flash pierwszą wartość tabeli jako dolną granicę przedziału.
- **Linia 31:** Numer pozycji w tabeli.
- **Linia 33:** Jeśli suma jest poniżej początku tabeli...
- **Linia 34:** ...zwróć najniższą temperaturę, czyli -20 °C × 10.
- **Linia 35:** Przejdź kolejno przez pozycje tabeli.
- **Linia 36:** Pobierz z flash górną granicę bieżącego przedziału.
- **Linia 37:** Jeśli suma jest poniżej tej granicy, znaleźliśmy przedział.
- **Linia 38:** Temperatura na początku tego przedziału, w dziesiątych częściach stopnia...
- **Linia 39:** ...plus część odległości wewnątrz przedziału. To tzw. **interpolacja liniowa**, czyli proporcjonalne wyliczenie wartości między dwoma punktami tabeli.
- **Linia 40:** Przedział się nie zgadza: górna granica staje się dolną granicą następnego.
- **Linia 41:** Koniec pętli.
- **Linia 42:** Suma jest powyżej końca tabeli, więc zwróć najwyższą temperaturę.

```c
 43  #else
 44  #error "Nieznany TEMP_SENSOR (config.h)"
 45  #endif
 46  }
```

- **Linia 43:** W przeciwnym razie, gdy wartość `TEMP_SENSOR` jest inna:
- **Linia 44:** Przerwij kompilację z komunikatem. Lepiej dostać błąd niż program z nieznanym czujnikiem.
- **Linia 45:** Koniec bloku warunkowego.
- **Linia 46:** Koniec funkcji.

### Potencjometr

```c
 48  uint8_t sensors_pot_percent(void)
 49  {
 50      uint16_t v = adc_value(ADC_CH_POT) >> 2;
 51
 52      return (uint8_t)((v * 100 + 127) / 255);
 53  }
```

- **Linia 48:** Funkcja zwraca położenie potencjometru w procentach.
- **Linia 49:** Początek funkcji.
- **Linia 50:** Pobierz odczyt 0–1023 i przesuń o 2 bity w prawo, czyli podziel przez 4, otrzymując 0–255. Odrzuca to najmniej stabilne bity.
- **Linia 52:** Przelicz na 0–100: pomnóż przez 100, podziel przez 255. Dodanie 127 (połowy dzielnika) daje zaokrąglenie.
- **Linia 53:** Koniec funkcji.

### Jasność w trybie AUTO

Im ciemniej, tym dioda świeci jaśniej. Zależność jest liniowa między progami `LDR_DARK` a `LDR_BRIGHT`.

```c
 55  uint8_t sensors_auto_percent(void)
 56  {
 57      uint16_t ldr = adc_value(ADC_CH_LDR);
 58
 59  #if LDR_INVERT
 60      ldr = 1023 - ldr;
 61  #endif
 62      if (ldr <= LDR_DARK)
 63          return 100;
 64      if (ldr >= LDR_BRIGHT)
 65          return 0;
 66      return (uint8_t)(100 - (uint32_t)(ldr - LDR_DARK) * 100 / (LDR_BRIGHT - LDR_DARK));
 67  }
```

- **Linia 55:** Funkcja zwraca jasność, jaką powinna mieć dioda w trybie AUTO.
- **Linia 56:** Początek funkcji.
- **Linia 57:** Pobierz odczyt fotorezystora.
- **Linia 59:** Poniższa linia jest kompilowana tylko, gdy fotorezystor jest podłączony odwrotnie.
- **Linia 60:** Odwróć wartość: 0 staje się 1023 i na odwrót.
- **Linia 61:** Koniec warunku.
- **Linia 62:** Jeśli jest ciemno, czyli odczyt nie przekracza progu ciemności...
- **Linia 63:** ...jasność wynosi 100 %.
- **Linia 64:** Jeśli jest bardzo jasno...
- **Linia 65:** ...jasność wynosi 0 %, dioda gaśnie.
- **Linia 66:** Pomiędzy: liczymy proporcjonalnie. Dla odczytu 500 przy progach 200 i 800 wychodzi 100 - 300 × 100 / 600 = 50 %.
- **Linia 67:** Koniec funkcji.

---

[← Poprzedni](17-firmware-adc.md) · [Spis treści](README.md) · [Następny →](19-firmware-button.md)
