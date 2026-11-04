# Firmware: `state.c`, pamięć ustawień

[← Poprzedni](19-firmware-button.md) · [Spis treści](README.md) · [Następny →](21-firmware-demo.md)

**Po co ten plik:** przechowuje stan urządzenia (włączone, jasność, tryb, próg alarmu) i zapisuje go w pamięci EEPROM, by przetrwał wyłączenie prądu.

**Dlaczego zapis jest tak ostrożny:**

- Pamięć EEPROM ma ograniczoną liczbę zapisów, więc czekamy 2 s po ostatniej zmianie i zapisujemy tylko to, co się różni.
- Zapis bajtu trwa kilka milisekund, a program nie może stać. Zapisujemy więc po jednym bajcie, gdy pamięć jest gotowa.

**Jak wykrywamy śmieci w pamięci:** nowy układ ma w EEPROM przypadkowe dane, a zanik prądu może przerwać zapis. Dlatego zapisujemy też stałą „magiczną” liczbę i sumę kontrolną. Jeśli się nie zgadzają, przyjmujemy wartości domyślne.

## `firmware/src/state.c`

[Otwórz ten plik w repozytorium](../../firmware/src/state.c)

```c
  1  // Stan urządzenia i jego zapis w EEPROM (z opóźnieniem, po jednym bajcie, bez blokowania)
  2  #include <avr/eeprom.h>
  3  #include "config.h"
  4  #include "pwm.h"
  5  #include "state.h"
```

- **Linia 1:** Komentarz: stan urządzenia i zapis w EEPROM z opóźnieniem, po jednym bajcie, bez blokowania.
- **Linia 2:** Biblioteka obsługi pamięci EEPROM.
- **Linia 3:** Nasze ustawienia.
- **Linia 4:** Nasz moduł jasności.
- **Linia 5:** Nasz nagłówek, m.in. definicja typu `state_t`.

```c
  7  #define EE_MAGIC 0x5A
  8
  9  typedef struct {
 10      uint8_t magic;
 11      uint8_t on;
 12      uint8_t pwm;
 13      uint8_t mode;
 14      uint8_t tlim;
 15      uint8_t check;
 16  } saved_t;
```

- **Linia 7:** Stała „magiczna” liczba, po której poznajemy, że w EEPROM są nasze dane.
- **Linia 9:** Początek struktury zapisywanej w EEPROM.
- **Linia 10:** Pole na magiczną liczbę.
- **Linia 11:** Pole na stan wyjścia.
- **Linia 12:** Pole na jasność.
- **Linia 13:** Pole na tryb.
- **Linia 14:** Pole na próg alarmu.
- **Linia 15:** Pole na sumę kontrolną.
- **Linia 16:** Koniec struktury, typ nazywa się `saved_t`.

```c
 18  state_t state;
 19
 20  static saved_t EEMEM ee_saved;
 21  static saved_t saved;
 22  static uint16_t save_delay;
 23  static uint8_t save_pos;
 24  static uint8_t saving;
```

- **Linia 18:** Właściwa zmienna ze stanem urządzenia. To ją zapowiada `extern` w `state.h`.
- **Linia 20:** Zmienna w pamięci EEPROM, którą wskazuje słowo `EEMEM`. Tu fizycznie leżą zapisane dane.
- **Linia 21:** Kopia zapisywanych danych w RAM.
- **Linia 22:** Ile milisekund zostało do rozpoczęcia zapisu.
- **Linia 23:** Który bajt zapisujemy jako następny.
- **Linia 24:** Znacznik: trwa zapis.

```c
 26  static uint8_t checksum(const saved_t *s)
 27  {
 28      return (uint8_t)(s->magic ^ s->on ^ s->pwm ^ s->mode ^ s->tlim ^ 0xA5);
 29  }
```

- **Linia 26:** Funkcja oblicza sumę kontrolną. Dostaje wskaźnik na strukturę, a `const` obiecuje, że jej nie zmieni.
- **Linia 27:** Początek funkcji.
- **Linia 28:** Operacja XOR łączy wszystkie pola w jeden bajt. Stała `0xA5` zapobiega temu, by same zera dawały poprawną sumę. `s->on` oznacza pole `on` struktury wskazywanej przez `s`.
- **Linia 29:** Koniec funkcji.

```c
 31  static void apply(void)
 32  {
 33      pwm_set_percent(state.on ? state.pwm : 0);
 34  }
```

- **Linia 31:** Funkcja przekazuje stan do diody.
- **Linia 32:** Początek funkcji.
- **Linia 33:** Jeśli wyjście jest włączone, ustaw jasność ze stanu, a jeśli nie, ustaw 0.
- **Linia 34:** Koniec funkcji.

```c
 36  static void changed(void)
 37  {
 38      saved.on = state.on;
 39      // Jasność z trybu AUTO nie jest zapisywana, po restarcie liczona jest od nowa
 40      if (state.mode == MODE_MANUAL)
 41          saved.pwm = state.pwm;
 42      saved.mode = state.mode;
 43      saved.tlim = state.tlim;
 44      saved.check = checksum(&saved);
 45      save_delay = SAVE_DELAY_MS;
 46      apply();
 47  }
```

- **Linia 36:** Funkcja wywoływana po każdej zmianie stanu.
- **Linia 37:** Początek funkcji.
- **Linia 38:** Przepisz stan wyjścia do kopii do zapisu.
- **Linia 39:** Komentarz: jasności z trybu AUTO nie zapisujemy, bo po restarcie i tak zostanie wyliczona od nowa.
- **Linia 40:** Jeśli tryb jest ręczny...
- **Linia 41:** ...przepisz jasność.
- **Linia 42:** Przepisz tryb.
- **Linia 43:** Przepisz próg alarmu.
- **Linia 44:** Oblicz nową sumę kontrolną. `&saved` to adres struktury.
- **Linia 45:** Uruchom odliczanie do zapisu. Każda kolejna zmiana zaczyna je od nowa.
- **Linia 46:** Natychmiast przekaż zmianę do diody.
- **Linia 47:** Koniec funkcji.

### Odczyt po starcie

```c
 49  void state_init(void)
 50  {
 51      eeprom_read_block(&saved, &ee_saved, sizeof(saved));
 52      if (saved.magic != EE_MAGIC || saved.check != checksum(&saved) || saved.on > 1
 53          || saved.pwm > 100 || saved.mode > MODE_AUTO || saved.tlim > TEMP_LIMIT_MAX) {
 54          saved.magic = EE_MAGIC;
 55          saved.on = 0;
 56          saved.pwm = 50;
 57          saved.mode = MODE_MANUAL;
 58          saved.tlim = TEMP_LIMIT_DEFAULT;
 59          saved.check = checksum(&saved);
 60      }
 61      state.on = saved.on;
 62      state.pwm = saved.pwm;
 63      state.mode = saved.mode;
 64      state.tlim = saved.tlim;
 65      state.alarm = 0;
 66      apply();
 67  }
```

- **Linia 49:** Funkcja wywoływana raz, przy starcie.
- **Linia 50:** Początek funkcji.
- **Linia 51:** Odczytaj całą strukturę z EEPROM do kopii w RAM. Podajemy adres celu, adres źródła i rozmiar w bajtach.
- **Linia 52:** Sprawdź, czy dane są poprawne: magiczna liczba, suma kontrolna, stan wyjścia najwyżej 1...
- **Linia 53:** ...jasność najwyżej 100, znany tryb, dozwolony próg. `||` oznacza, że wystarczy jeden błąd.
- **Linia 54:** Dane są złe, więc ustaw magiczną liczbę...
- **Linia 55:** ...wyjście wyłączone...
- **Linia 56:** ...jasność 50 %...
- **Linia 57:** ...tryb ręczny...
- **Linia 58:** ...domyślny próg alarmu...
- **Linia 59:** ...i policz sumę kontrolną.
- **Linia 60:** Koniec warunku.
- **Linia 61:** Skopiuj stan wyjścia do stanu urządzenia.
- **Linia 62:** Skopiuj jasność.
- **Linia 63:** Skopiuj tryb.
- **Linia 64:** Skopiuj próg alarmu.
- **Linia 65:** Alarm zaczyna jako wyłączony. Nie jest zapisywany, bo zależy od aktualnej temperatury.
- **Linia 66:** Ustaw diodę zgodnie ze stanem.
- **Linia 67:** Koniec funkcji.

### Zapis w tle

```c
 69  void state_tick(void)
 70  {
 71      if (save_delay > 0 && --save_delay == 0) {
 72          save_pos = 0;
 73          saving = 1;
 74      }
 75      if (saving && eeprom_is_ready()) {
 76          uint8_t *ee = (uint8_t *)&ee_saved + save_pos;
 77          uint8_t value = ((const uint8_t *)&saved)[save_pos];
 78
 79          if (eeprom_read_byte(ee) != value)
 80              eeprom_write_byte(ee, value);
 81          if (++save_pos == (uint8_t)sizeof(saved))
 82              saving = 0;
 83      }
 84  }
```

- **Linia 69:** Funkcja wywoływana co 1 ms.
- **Linia 70:** Początek funkcji.
- **Linia 71:** Jeśli trwa odliczanie, zmniejsz je o 1. Jeśli właśnie doszło do zera...
- **Linia 72:** ...zacznij zapis od pierwszego bajtu...
- **Linia 73:** ...i oznacz, że zapis trwa.
- **Linia 74:** Koniec warunku.
- **Linia 75:** Jeśli zapis trwa, a pamięć EEPROM jest gotowa (poprzedni bajt się zapisał)...
- **Linia 76:** Oblicz adres bieżącego bajtu w EEPROM: adres struktury potraktowany jak adres bajtów plus numer bajtu.
- **Linia 77:** Odczytaj wartość bajtu z kopii w RAM. Struktura jest tu traktowana jak zwykła tablica bajtów.
- **Linia 79:** Jeśli bajt w EEPROM jest inny niż ten do zapisu...
- **Linia 80:** ...zapisz go. Niezmienione bajty pomijamy, co oszczędza pamięć.
- **Linia 81:** Przejdź do następnego bajtu. Jeśli to był ostatni...
- **Linia 82:** ...zakończ zapis.
- **Linia 83:** Koniec warunku.
- **Linia 84:** Koniec funkcji.

### Zmiany stanu

```c
 86  void state_set_on(uint8_t on)
 87  {
 88      state.on = on ? 1 : 0;
 89      changed();
 90  }
 91
 92  uint8_t state_set_pwm(uint8_t pwm)
 93  {
 94      uint8_t mode_changed = state.mode != MODE_MANUAL;
 95
 96      state.mode = MODE_MANUAL;
 97      state.pwm = pwm;
 98      changed();
 99      return mode_changed;
100  }
101
102  void state_set_mode(uint8_t mode)
103  {
104      state.mode = mode;
105      changed();
106  }
107
108  void state_set_tlim(uint8_t tlim)
109  {
110      state.tlim = tlim;
111      changed();
112  }
113
114  void state_auto_pwm(uint8_t pwm)
115  {
116      state.pwm = pwm;
117      apply();
118  }
```

- **Linia 86:** Funkcja ustawiająca włączenie lub wyłączenie.
- **Linia 87:** Początek funkcji.
- **Linia 88:** Zapisz 1, jeśli parametr jest różny od zera, a w przeciwnym razie 0.
- **Linia 89:** Zgłoś zmianę.
- **Linia 90:** Koniec funkcji.
- **Linia 92:** Funkcja ustawiająca jasność. Zwraca 1, gdy zmienił się tryb.
- **Linia 93:** Początek funkcji.
- **Linia 94:** Zapamiętaj, czy tryb był inny niż ręczny.
- **Linia 96:** Ustaw tryb ręczny, bo ręczna zmiana jasności wyłącza tryb AUTO.
- **Linia 97:** Zapisz jasność.
- **Linia 98:** Zgłoś zmianę.
- **Linia 99:** Zwróć informację o zmianie trybu.
- **Linia 100:** Koniec funkcji.
- **Linia 102:** Funkcja ustawiająca tryb.
- **Linia 103:** Początek funkcji.
- **Linia 104:** Zapisz tryb.
- **Linia 105:** Zgłoś zmianę.
- **Linia 106:** Koniec funkcji.
- **Linia 108:** Funkcja ustawiająca próg alarmu.
- **Linia 109:** Początek funkcji.
- **Linia 110:** Zapisz próg.
- **Linia 111:** Zgłoś zmianę.
- **Linia 112:** Koniec funkcji.
- **Linia 114:** Funkcja ustawiająca jasność wyliczoną w trybie AUTO.
- **Linia 115:** Początek funkcji.
- **Linia 116:** Zapisz jasność.
- **Linia 117:** Przekaż ją do diody. Celowo nie wywołujemy `changed()`, bo tej jasności nie zapisujemy w EEPROM.
- **Linia 118:** Koniec funkcji.

---

[← Poprzedni](19-firmware-button.md) · [Spis treści](README.md) · [Następny →](21-firmware-demo.md)
