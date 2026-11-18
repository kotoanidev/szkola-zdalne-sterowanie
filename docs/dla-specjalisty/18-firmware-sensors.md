# Firmware: `sensors.c`, przeliczanie pomiarów

[← Poprzedni](17-firmware-adc.md) · [Spis treści](README.md) · [Następny →](19-firmware-button.md)

**Po co ten plik:** przetwornik podaje surowe liczby. Ten moduł zamienia je na wielkości, które coś znaczą: położenie potencjometru w procentach i jasność dla trybu AUTO. Temperaturę podaje gotową czujnik cyfrowy DS18B20 – opis w [pliku `ds18b20.c`](22-firmware-ds18b20.md). Wszystko liczymy na liczbach całkowitych, bo ułamki są dla tego układu bardzo kosztowne.

## `firmware/src/sensors.c`

[Otwórz ten plik w repozytorium](../../firmware/src/sensors.c)

```c
  1  // Przeliczanie pomiarów ADC: potencjometr, jasność w trybie AUTO
  2  #include "config.h"
  3  #include "pinout.h"
  4  #include "adc.h"
  5  #include "sensors.h"
```

- **Linia 1:** Komentarz: przeliczanie pomiarów ADC na położenie potencjometru i jasność w trybie AUTO.
- **Linia 2:** Nasze ustawienia.
- **Linia 3:** Nasze przypisanie pinów, m.in. numery kanałów.
- **Linia 4:** Nasz moduł pomiarów ADC.
- **Linia 5:** Nasz nagłówek.

### Potencjometr

```c
  7  uint8_t sensors_pot_percent(void)
  8  {
  9      uint16_t v = adc_value(ADC_CH_POT) >> 2;
 10
 11      return (uint8_t)((v * 100 + 127) / 255);
 12  }
```

- **Linia 7:** Funkcja zwraca położenie potencjometru w procentach.
- **Linia 8:** Początek funkcji.
- **Linia 9:** Pobierz odczyt 0–1023 i przesuń o 2 bity w prawo, czyli podziel przez 4, otrzymując 0–255. Odrzuca to najmniej stabilne bity.
- **Linia 11:** Przelicz na 0–100: pomnóż przez 100, podziel przez 255. Dodanie 127 (połowy dzielnika) daje zaokrąglenie.
- **Linia 12:** Koniec funkcji.

### Jasność w trybie AUTO

Im ciemniej, tym dioda świeci jaśniej. Zależność jest liniowa między progami `LDR_DARK` a `LDR_BRIGHT`.

```c
 14  uint8_t sensors_auto_percent(void)
 15  {
 16      uint16_t ldr = adc_value(ADC_CH_LDR);
 17
 18  #if LDR_INVERT
 19      ldr = 1023 - ldr;
 20  #endif
 21      if (ldr <= LDR_DARK)
 22          return 100;
 23      if (ldr >= LDR_BRIGHT)
 24          return 0;
 25      return (uint8_t)(100 - (uint32_t)(ldr - LDR_DARK) * 100 / (LDR_BRIGHT - LDR_DARK));
 26  }
```

- **Linia 14:** Funkcja zwraca jasność, jaką powinna mieć dioda w trybie AUTO.
- **Linia 15:** Początek funkcji.
- **Linia 16:** Pobierz odczyt fotorezystora.
- **Linia 18:** Poniższa linia jest kompilowana tylko, gdy fotorezystor jest podłączony odwrotnie.
- **Linia 19:** Odwróć wartość: 0 staje się 1023 i na odwrót.
- **Linia 20:** Koniec warunku.
- **Linia 21:** Jeśli jest ciemno, czyli odczyt nie przekracza progu ciemności...
- **Linia 22:** ...jasność wynosi 100 %.
- **Linia 23:** Jeśli jest bardzo jasno...
- **Linia 24:** ...jasność wynosi 0 %, dioda gaśnie.
- **Linia 25:** Pomiędzy: liczymy proporcjonalnie. Dla odczytu 500 przy progach 200 i 800 wychodzi 100 - 300 × 100 / 600 = 50 %.
- **Linia 26:** Koniec funkcji.

---

[← Poprzedni](17-firmware-adc.md) · [Spis treści](README.md) · [Następny →](19-firmware-button.md)
