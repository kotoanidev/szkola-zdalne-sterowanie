# Firmware: pliki nagłówkowe (`firmware/include/`)

[← Poprzedni](10-firmware-makefile.md) · [Spis treści](README.md) · [Następny →](12-firmware-main.md)

**Po co są te pliki:** nagłówek `.h` to spis tego, co dany moduł udostępnia innym. Sama treść funkcji jest w odpowiadającym pliku `.c`. Wszystkie pojęcia języka C są w [podstawach C](01-podstawy-jezyka-c.md).

Dwa nagłówki, `config.h` i `pinout.h`, są wyjątkowe, bo zawierają wyłącznie ustawienia.

## `firmware/include/config.h`

[Otwórz ten plik w repozytorium](../../firmware/include/config.h)

Wszystkie „pokrętła” firmware zebrane w jednym miejscu. Zmieniasz wartość tutaj, a program po skompilowaniu zachowuje się inaczej.

```c
  1  // Ustawienia firmware. F_CPU definiuje tylko Makefile.
  2  #ifndef CONFIG_H
  3  #define CONFIG_H
  4
  5  #define FW_VERSION          "0.10.0"
  6
  7  // Domyślny próg alarmu temperatury [°C] i histereza wyłączenia [0,1 °C]
  8  #define TEMP_LIMIT_DEFAULT  50
  9  #define TEMP_LIMIT_MAX      125
 10  #define TEMP_HYST_TENTHS    20
 11
```

- **Linia 1:** Komentarz: to ustawienia firmware. Stałą `F_CPU` definiuje tylko `Makefile`, by nie było dwóch sprzecznych wartości.
- **Linia 2:** Strażnik nagłówka: dalszy tekst będzie wzięty pod uwagę tylko wtedy, gdy nazwa `CONFIG_H` jeszcze nie istnieje.
- **Linia 3:** Tworzy nazwę `CONFIG_H`, więc przy kolejnym dołączeniu plik zostanie pominięty.
- **Linia 5:** Numer wersji programu, napis `"0.10.0"`. Układ podaje go na komendę `VER` i po starcie. Wersja 0.10 to firmware dla ATmega32A na płytce ATB z czujnikiem DS18B20.
- **Linia 7:** Komentarz do trzech kolejnych stałych, dotyczących alarmu temperatury.
- **Linia 8:** Domyślny próg alarmu: 50 °C.
- **Linia 9:** Największy dozwolony próg: 125 °C (tyle mierzy DS18B20).
- **Linia 10:** Histereza alarmu w dziesiątych częściach stopnia: 20 oznacza 2,0 °C. Alarm włączony przy 50 °C zgaśnie dopiero poniżej 48 °C.

```c
 12  // 0 – potencjometr niepodłączony (ADC0 ignorowany)
 13  #define POT_ENABLE          1
 14  // Minimalna zmiana położenia potencjometru [%], która zmienia jasność
 15  #define POT_HYST            2
```

- **Linia 12:** Komentarz: wartość 0 oznacza, że potencjometr nie jest podłączony i wejście ADC0 ma być ignorowane.
- **Linia 13:** Wartość 1: potencjometr jest używany.
- **Linia 14:** Komentarz: najmniejsza zmiana położenia potencjometru w procentach, która zmienia jasność.
- **Linia 15:** 2 %. Dzięki temu drobne wahania pomiaru nie zmieniają ciągle jasności.

```c
 17  // Tryb AUTO: odczyt ADC1 (więcej światła = wyższe napięcie) przy ciemności i przy pełnym świetle
 18  #define LDR_DARK            200
 19  #define LDR_BRIGHT          800
 20  // 1 – fotorezystor podłączony odwrotnie (do GND)
 21  #define LDR_INVERT          0
 22  #define AUTO_HYST           3
```

- **Linia 17:** Komentarz: w trybie AUTO jasność zależy od odczytu z fotorezystora na ADC1. Więcej światła to wyższe napięcie.
- **Linia 18:** Odczyt 200 lub mniej uznajemy za ciemność.
- **Linia 19:** Odczyt 800 lub więcej uznajemy za pełne światło.
- **Linia 20:** Komentarz: wartość 1 oznacza fotorezystor podłączony odwrotnie, do masy.
- **Linia 21:** Wartość 0: połączenie standardowe.
- **Linia 22:** W trybie AUTO jasność zmieni się dopiero przy różnicy co najmniej 3 %.

```c
 24  // Płynne przejście: przyrost jasności (skala 0–1023) na 1 ms, pełna zmiana ok. 0,5 s
 25  #define PWM_FADE_STEP       2
 26
 27  #define BTN_DEBOUNCE_MS     20
 28  #define BTN_LONG_MS         1000
```

- **Linia 24:** Komentarz: płynne przejście. Jasność w skali 0–1023 zmienia się o stałą wartość co 1 ms, więc pełna zmiana trwa około 0,5 s.
- **Linia 25:** Krok: 2 na milisekundę. 1023 / 2 to około 512 ms.
- **Linia 27:** Przycisk musi być stabilny przez 20 ms, zanim uznamy jego stan (ochrona przed drganiami styków).
- **Linia 28:** Przytrzymanie przez 1000 ms, czyli 1 s, liczy się jako długie naciśnięcie.

```c
 30  // Opóźnienie zapisu stanu w EEPROM po ostatniej zmianie [ms]
 31  #define SAVE_DELAY_MS       2000
 32
 33  // Odkomentuj, jeśli w terminalu są „krzaki” (korekta oscylatora RC bez zmiany fusebitów)
 34  // #define OSCCAL_VALUE     0xA8
 35
 36  #endif
```

- **Linia 30:** Komentarz: stan jest zapisywany w pamięci EEPROM z opóźnieniem po ostatniej zmianie.
- **Linia 31:** Opóźnienie: 2000 ms. Ruch suwaka powoduje wiele zmian, a pamięć EEPROM zużywa się przy zapisie, więc czekamy, aż zmiany ustaną.
- **Linia 33:** Komentarz: jeśli w terminalu widać „krzaki”, można ręcznie skorygować oscylator układu, nie zmieniając fusebitów.
- **Linia 34:** Wyłączone (zakomentowane) ustawienie. Usunięcie `//` włączyłoby korektę wartością `0xA8`.
- **Linia 36:** Koniec strażnika nagłówka z linii 2.

## `firmware/include/pinout.h`

[Otwórz ten plik w repozytorium](../../firmware/include/pinout.h)

Mapa połączeń: mówi programowi, do której nóżki układu podłączono który element płytki ATB. Musi być zgodna z `docs/sprzet.md`. Dzięki nazwom w reszcie kodu piszemy `LED_PORT`, a nie `PORTD`. Program etapu 1 (ATmega8) ma własną, starą mapę w `firmware/demo/pinout.h`.

```c
  1  // Przypisanie pinów ATmega32A na płytce ATB 1.05 – musi być zgodne z docs/sprzet.md (zmiana = oba pliki).
  2  #ifndef PINOUT_H
  3  #define PINOUT_H
  4
  5  #include <avr/io.h>
  6
  7  // Diody LED na ATB świecą stanem niskim (anoda do VCC) – do potwierdzenia testem z docs/sprzet.md
  8  #define LED_ACTIVE_LOW 1
```

- **Linia 1:** Komentarz: przypisanie pinów ATmega32A na płytce ATB 1.05. Zmiana wymaga zmiany także w `docs/sprzet.md`.
- **Linia 2:** Strażnik nagłówka: wklej treść tylko raz.
- **Linia 3:** Tworzy nazwę strażnika.
- **Linia 5:** Dołącza plik biblioteki z nazwami rejestrów i bitów układu, np. `DDRD`, `PD5`.
- **Linia 7:** Komentarz: diody na płytce ATB świecą przy stanie niskim (anoda do +5 V). Trzeba to jeszcze potwierdzić prostym testem opisanym w `docs/sprzet.md`.
- **Linia 8:** `LED_ACTIVE_LOW` równe 1 mówi programowi, że diodę zapala 0 V. Gdyby test wykazał inaczej, wystarczy wpisać 0.

```c
 10  // Dioda główna: PD5 / OC1A, PWM z Timer1 (przewód do LED8, zworka J25 PC7 zdjęta)
 11  #define LED_DDR      DDRD
 12  #define LED_PORT     PORTD
 13  #define LED_BIT      PD5
 14
 15  // Dioda statusu: PC0 -> LED1 (zworka J25)
 16  #define STATUS_DDR   DDRC
 17  #define STATUS_PORT  PORTC
 18  #define STATUS_BIT   PC0
```

- **Linia 10:** Komentarz: dioda główna jest na pinie `PD5`, który jest też wyjściem `OC1A` sprzętowego PWM. Na płytce trzeba połączyć go przewodem z diodą LED8 i zdjąć zworkę J25 przy `PC7`.
- **Linia 11:** `LED_DDR` to inna nazwa rejestru kierunku portu D.
- **Linia 12:** `LED_PORT` to inna nazwa rejestru wyjściowego portu D.
- **Linia 13:** `LED_BIT` to numer bitu tej nóżki w porcie, czyli `PD5`.
- **Linia 15:** Komentarz: dioda statusu to LED1 połączona zworką J25 z pinem `PC0`.
- **Linia 16:** Rejestr kierunku portu C.
- **Linia 17:** Rejestr wyjściowy portu C.
- **Linia 18:** Numer bitu nóżki: `PC0`.

```c
 20  // Przycisk K1 (J26 KEY1) do GND z wewnętrznym pull-upem: PD2
 21  #define BTN_DDR      DDRD
 22  #define BTN_PORT     PORTD
 23  #define BTN_PIN      PIND
 24  #define BTN_BIT      PD2
```

- **Linia 20:** Komentarz: przycisk K1 z listwy J26 (KEY1) łączymy przewodem z pinem `PD2`. Naciśnięcie zwiera go z masą, a wbudowany rezystor pull-up trzyma stan wysoki.
- **Linia 21:** Rejestr kierunku portu D.
- **Linia 22:** Rejestr wyjściowy portu D. Dla wejścia służy do włączenia pull-upu.
- **Linia 23:** Rejestr odczytu stanu nóżek portu D. Jest potrzebny tylko dla wejścia, dlatego diody go nie mają.
- **Linia 24:** Numer bitu nóżki: `PD2`.

```c
 26  // Czujnik DS18B20 (1-Wire, pull-up na płytce): PD3
 27  #define OW_DDR       DDRD
 28  #define OW_PORT      PORTD
 29  #define OW_PIN       PIND
 30  #define OW_BIT       PD3
 31
 32  // Kanały ADC: PA0 – potencjometr P1 (POT), PA1 – fotorezystor (poza płytką); zworki J30 0–1 zdjęte
 33  #define ADC_CH_POT   0
 34  #define ADC_CH_LDR   1
 35
 36  #endif
```

- **Linia 26:** Komentarz: czujnik temperatury DS18B20 komunikuje się jednym przewodem (1-Wire) z pinem `PD3`. Rezystor podciągający linię do +5 V jest na płytce.
- **Linia 27:** Rejestr kierunku portu D. Przez zmianę kierunku pinu program ściąga linię do masy albo ją zwalnia.
- **Linia 28:** Rejestr wyjściowy portu D. Bit `PD3` zostaje w nim zawsze równy 0.
- **Linia 29:** Rejestr odczytu stanu nóżek portu D, potrzebny do odczytu bitów od czujnika.
- **Linia 30:** Numer bitu nóżki: `PD3`.
- **Linia 32:** Komentarz: kanały przetwornika ADC to nóżki `PA0` (potencjometr P1) i `PA1` (fotorezystor dokładany poza płytką). Zworki J30 łączące te piny z wyświetlaczem trzeba zdjąć.
- **Linia 33:** Kanał 0: potencjometr.
- **Linia 34:** Kanał 1: fotorezystor (LDR).
- **Linia 36:** Koniec strażnika nagłówka.

## `firmware/include/adc.h`

[Otwórz ten plik w repozytorium](../../firmware/include/adc.h)

Spis funkcji modułu pomiarów analogowych. Treść jest w [pliku `adc.c`](17-firmware-adc.md).

```c
  1  #ifndef ADC_H
  2  #define ADC_H
  3
  4  #include <stdint.h>
  5
  6  #define ADC_CHANNELS 2
  7  #define ADC_SAMPLES  16
  8
  9  void adc_init(void);
 10  void adc_poll(void);
 11  uint8_t adc_ready(void);
 12  uint16_t adc_sum(uint8_t channel);
 13  uint16_t adc_value(uint8_t channel);
 14
 15  #endif
```

- **Linia 1:** Strażnik nagłówka: jeśli `ADC_H` jeszcze nie istnieje...
- **Linia 2:** ...utwórz tę nazwę i wklej dalszą treść.
- **Linia 4:** Dołącza definicje typów, takich jak `uint8_t` i `uint16_t`.
- **Linia 6:** Mierzymy 2 kanały: potencjometr i fotorezystor.
- **Linia 7:** Na każdy kanał zbieramy 16 próbek.
- **Linia 9:** Zapowiedź funkcji przygotowującej przetwornik.
- **Linia 10:** Zapowiedź funkcji, którą pętla główna wywołuje ciągle, by zebrać wyniki.
- **Linia 11:** Zapowiedź funkcji mówiącej, czy są już pierwsze komplety pomiarów.
- **Linia 12:** Zapowiedź funkcji zwracającej sumę 16 próbek danego kanału.
- **Linia 13:** Zapowiedź funkcji zwracającej średnią z tych próbek.
- **Linia 15:** Koniec strażnika.

## `firmware/include/button.h`

[Otwórz ten plik w repozytorium](../../firmware/include/button.h)

Spis funkcji modułu przycisku. Treść jest w [pliku `button.c`](19-firmware-button.md).

```c
  1  #ifndef BUTTON_H
  2  #define BUTTON_H
  3
  4  #include <stdint.h>
  5
  6  enum {
  7      BTN_NONE,
  8      BTN_SHORT,
  9      BTN_LONG
 10  };
 11
 12  void button_init(void);
 13  uint8_t button_tick(void);
 14
 15  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 6:** Początek wyliczenia, czyli nazw dla kolejnych liczb. To możliwe wyniki sprawdzenia przycisku.
- **Linia 7:** `BTN_NONE` = 0: nic się nie stało.
- **Linia 8:** `BTN_SHORT` = 1: krótkie naciśnięcie.
- **Linia 9:** `BTN_LONG` = 2: długie przytrzymanie.
- **Linia 10:** Koniec wyliczenia.
- **Linia 12:** Zapowiedź funkcji przygotowującej pin przycisku.
- **Linia 13:** Zapowiedź funkcji wywoływanej co 1 ms. Zwraca jeden z powyższych wyników.
- **Linia 15:** Koniec strażnika.

## `firmware/include/ds18b20.h`

[Otwórz ten plik w repozytorium](../../firmware/include/ds18b20.h)

Spis funkcji modułu czujnika temperatury DS18B20. Treść jest w [pliku `ds18b20.c`](22-firmware-ds18b20.md).

```c
  1  #ifndef DS18B20_H
  2  #define DS18B20_H
  3
  4  #include <stdint.h>
  5
  6  // Brak ważnego odczytu (czujnik nie odpowiada albo błąd CRC)
  7  #define TEMP_NONE INT16_MIN
  8
  9  void ds18b20_tick(void);
 10  int16_t ds18b20_tenths(void);
 11
 12  #endif
```

- **Linia 1:** Strażnik nagłówka: jeśli `DS18B20_H` jeszcze nie istnieje...
- **Linia 2:** ...utwórz tę nazwę i wklej dalszą treść.
- **Linia 4:** Dołącza definicje typów liczbowych, m.in. `int16_t` i stałą `INT16_MIN`.
- **Linia 6:** Komentarz: wartość oznaczająca brak ważnego odczytu.
- **Linia 7:** `TEMP_NONE` to najmniejsza liczba 16-bitowa ze znakiem (-32768). Czujnik nigdy nie zwróci takiej temperatury, więc może ona oznaczać „brak odczytu”.
- **Linia 9:** Zapowiedź funkcji wywoływanej co 1 ms, która krok po kroku obsługuje czujnik.
- **Linia 10:** Zapowiedź funkcji zwracającej ostatnią temperaturę w dziesiątych częściach stopnia, np. 235 to 23,5 °C, albo `TEMP_NONE`.
- **Linia 12:** Koniec strażnika.

## `firmware/include/protocol.h`

[Otwórz ten plik w repozytorium](../../firmware/include/protocol.h)

Spis funkcji protokołu tekstowego. Treść jest w [pliku `protocol.c`](15-firmware-protocol.md).

```c
  1  #ifndef PROTOCOL_H
  2  #define PROTOCOL_H
  3
  4  #include <stdint.h>
  5  #include <avr/pgmspace.h>
  6
  7  uint8_t protocol_rx(char c);
  8  void protocol_event_boot(PGM_P reason);
  9  void protocol_event_btn_on(void);
 10  void protocol_event_btn_mode(void);
 11  void protocol_event_pot(uint8_t mode_changed);
 12  void protocol_event_temp(int16_t temp);
 13
 14  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 5:** Dołącza narzędzia do pamięci flash, w tym typ `PGM_P`, czyli wskaźnik na napis we flash.
- **Linia 7:** Zapowiedź funkcji przyjmującej jeden odebrany znak. Zwraca 1, gdy wykonano komendę.
- **Linia 8:** Zapowiedź funkcji wysyłającej zdarzenie startu. Dostaje przyczynę resetu.
- **Linia 9:** Zapowiedź funkcji wysyłającej zdarzenie „przycisk zmienił stan wyjścia”.
- **Linia 10:** Zapowiedź funkcji wysyłającej zdarzenie „przycisk zmienił tryb”.
- **Linia 11:** Zapowiedź funkcji wysyłającej zdarzenie zmiany potencjometru. Parametr mówi, czy zmienił się też tryb.
- **Linia 12:** Zapowiedź funkcji wysyłającej zdarzenie alarmu temperatury. Parametr to temperatura.
- **Linia 14:** Koniec strażnika.

## `firmware/include/pwm.h`

[Otwórz ten plik w repozytorium](../../firmware/include/pwm.h)

Spis funkcji sterowania jasnością. Treść jest w [pliku `pwm.c`](16-firmware-pwm.md).

```c
  1  #ifndef PWM_H
  2  #define PWM_H
  3
  4  #include <stdint.h>
  5
  6  void pwm_init(void);
  7  void pwm_set_percent(uint8_t percent);
  8  void pwm_tick(void);
  9
 10  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 6:** Zapowiedź funkcji przygotowującej sprzętowy PWM.
- **Linia 7:** Zapowiedź funkcji ustawiającej docelową jasność w procentach.
- **Linia 8:** Zapowiedź funkcji wywoływanej co 1 ms, która płynnie zbliża jasność do celu.
- **Linia 10:** Koniec strażnika.

## `firmware/include/sensors.h`

[Otwórz ten plik w repozytorium](../../firmware/include/sensors.h)

Spis funkcji przeliczających pomiary. Treść jest w [pliku `sensors.c`](18-firmware-sensors.md).

```c
  1  #ifndef SENSORS_H
  2  #define SENSORS_H
  3
  4  #include <stdint.h>
  5
  6  uint8_t sensors_pot_percent(void);
  7  uint8_t sensors_auto_percent(void);
  8
  9  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 6:** Zapowiedź funkcji zwracającej położenie potencjometru w procentach.
- **Linia 7:** Zapowiedź funkcji zwracającej jasność, jaką powinna mieć dioda w trybie AUTO.
- **Linia 9:** Koniec strażnika.

## `firmware/include/state.h`

[Otwórz ten plik w repozytorium](../../firmware/include/state.h)

Opis stanu urządzenia, czyli tego, co układ pamięta. Treść funkcji jest w [pliku `state.c`](20-firmware-state.md).

```c
  1  #ifndef STATE_H
  2  #define STATE_H
  3
  4  #include <stdint.h>
  5
  6  enum {
  7      MODE_MANUAL,
  8      MODE_AUTO
  9  };
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 6:** Początek wyliczenia trybów pracy.
- **Linia 7:** `MODE_MANUAL` = 0: jasność ustawia człowiek.
- **Linia 8:** `MODE_AUTO` = 1: jasność zależy od światła.
- **Linia 9:** Koniec wyliczenia.

```c
 11  typedef struct {
 12      uint8_t on;
 13      uint8_t pwm;
 14      uint8_t mode;
 15      uint8_t tlim;
 16      uint8_t alarm;
 17  } state_t;
 18
 19  extern state_t state;
```

- **Linia 11:** Początek definicji struktury, czyli zestawu powiązanych pól.
- **Linia 12:** Pole `on`: czy wyjście jest włączone (1) czy wyłączone (0).
- **Linia 13:** Pole `pwm`: jasność w procentach.
- **Linia 14:** Pole `mode`: tryb pracy.
- **Linia 15:** Pole `tlim`: próg alarmu temperatury w stopniach.
- **Linia 16:** Pole `alarm`: czy alarm jest aktywny.
- **Linia 17:** Koniec struktury. `state_t` to nazwa nowego typu.
- **Linia 19:** `extern` zapowiada, że zmienna `state` istnieje w innym pliku (`state.c`), ale wolno jej używać wszędzie, gdzie dołączono ten nagłówek.

```c
 21  void state_init(void);
 22  void state_tick(void);
 23  void state_set_on(uint8_t on);
 24  uint8_t state_set_pwm(uint8_t pwm);
 25  void state_set_mode(uint8_t mode);
 26  void state_set_tlim(uint8_t tlim);
 27  void state_auto_pwm(uint8_t pwm);
 28
 29  #endif
```

- **Linia 21:** Zapowiedź funkcji odczytującej stan z EEPROM po starcie.
- **Linia 22:** Zapowiedź funkcji wywoływanej co 1 ms, która zajmuje się zapisem.
- **Linia 23:** Zapowiedź funkcji ustawiającej włączenie lub wyłączenie.
- **Linia 24:** Zapowiedź funkcji ustawiającej jasność. Zwraca informację, czy zmienił się tryb.
- **Linia 25:** Zapowiedź funkcji ustawiającej tryb.
- **Linia 26:** Zapowiedź funkcji ustawiającej próg alarmu.
- **Linia 27:** Zapowiedź funkcji ustawiającej jasność wyliczoną w trybie AUTO.
- **Linia 29:** Koniec strażnika.

## `firmware/include/timer.h`

[Otwórz ten plik w repozytorium](../../firmware/include/timer.h)

Spis funkcji zegara systemowego. Treść jest w [pliku `timer.c`](13-firmware-timer.md).

```c
  1  #ifndef TIMER_H
  2  #define TIMER_H
  3
  4  #include <stdint.h>
  5
  6  void timer_init(void);
  7  uint32_t timer_ms(void);
  8  uint16_t timer_ms16(void);
  9
 10  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 6:** Zapowiedź funkcji uruchamiającej zegar.
- **Linia 7:** Zapowiedź funkcji zwracającej liczbę milisekund od startu jako liczbę 32-bitową.
- **Linia 8:** Zapowiedź funkcji zwracającej to samo jako liczbę 16-bitową. Jest szybsza, ale przekręca się co około 65 sekund.
- **Linia 10:** Koniec strażnika.

## `firmware/include/uart.h`

[Otwórz ten plik w repozytorium](../../firmware/include/uart.h)

Spis funkcji komunikacji szeregowej. Treść jest w [pliku `uart.c`](14-firmware-uart.md).

```c
  1  #ifndef UART_H
  2  #define UART_H
  3
  4  #include <stdint.h>
  5  #include <avr/pgmspace.h>
  6
  7  #define UART_BAUD 9600UL
  8
  9  void uart_init(void);
 10  uint8_t uart_read(char *c);
 11  void uart_putc(char c);
 12  void uart_puts_P(PGM_P s);
 13  void uart_put_uint(uint32_t value);
 14  void uart_put_tenths(int16_t value);
 15  void uart_endl(void);
 16
 17  #endif
```

- **Linia 1:** Strażnik nagłówka: początek warunku.
- **Linia 2:** Strażnik nagłówka: utworzenie nazwy.
- **Linia 4:** Dołącza definicje typów liczbowych.
- **Linia 5:** Dołącza narzędzia do pamięci flash.
- **Linia 7:** Prędkość transmisji: 9600 bitów na sekundę.
- **Linia 9:** Zapowiedź funkcji przygotowującej UART.
- **Linia 10:** Zapowiedź funkcji czytającej jeden znak. Dostaje adres, pod który ma go wpisać, i zwraca 1, jeśli znak był.
- **Linia 11:** Zapowiedź funkcji wysyłającej jeden znak.
- **Linia 12:** Zapowiedź funkcji wysyłającej napis z pamięci flash.
- **Linia 13:** Zapowiedź funkcji wysyłającej liczbę jako tekst.
- **Linia 14:** Zapowiedź funkcji wysyłającej liczbę dziesiątych części jako tekst z kropką, np. 235 jako `23.5`.
- **Linia 15:** Zapowiedź funkcji wysyłającej znak końca linii.
- **Linia 17:** Koniec strażnika.

---

[← Poprzedni](10-firmware-makefile.md) · [Spis treści](README.md) · [Następny →](12-firmware-main.md)
