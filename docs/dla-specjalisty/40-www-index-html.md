# Strona WWW: `index.html`, zawartość strony

[← Poprzedni](37-serwer-wdrozenie.md) · [Spis treści](README.md) · [Następny →](41-www-app-css.md)

**Po co ten plik:** opisuje, co jest na stronie: nagłówek, przyciski, suwaki, pomiary i dziennik. Nie zawiera logiki. Wartości takie jak `–` są tylko początkowe i zastąpi je kod z `app.js`. Znaczniki są wytłumaczone w [podstawach WWW](03-podstawy-www.md).

## `server/app/static/index.html`

[Otwórz ten plik w repozytorium](../../server/app/static/index.html)

### Część niewidoczna

```html
  1  <!doctype html>
  2  <html lang="pl">
  3  <head>
  4    <meta charset="utf-8">
  5    <meta name="viewport" content="width=device-width, initial-scale=1">
  6    <title>Oświetlenie LED – sterowanie</title>
  7    <link rel="stylesheet" href="/static/app.css">
  8    <link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3E%3Ccircle cx='8' cy='8' r='6' fill='%23f5a524'/%3E%3C/svg%3E">
  9  </head>
```

- **Linia 1:** Informacja dla przeglądarki: to nowoczesny dokument HTML.
- **Linia 2:** Początek strony. `lang="pl"` mówi, że jest po polsku, co pomaga np. czytnikom ekranu.
- **Linia 3:** Początek części z informacjami o stronie.
- **Linia 4:** Kodowanie znaków UTF-8, dzięki któremu działają polskie litery.
- **Linia 5:** Ustawienie dla telefonów: szerokość strony równa szerokości ekranu, bez powiększenia.
- **Linia 6:** Tytuł widoczny na karcie przeglądarki.
- **Linia 7:** Dołącz plik z wyglądem, `app.css`.
- **Linia 8:** Ikona karty przeglądarki: pomarańczowe kółko, zapisane bezpośrednio w adresie jako obrazek SVG. Dzięki temu nie wymaga osobnego pliku.
- **Linia 9:** Koniec części z informacjami.

### Nagłówek

```html
 10  <body>
 11    <header class="topbar">
 12      <div class="brand">
 13        <span class="brand-dot" aria-hidden="true"></span>
 14        <div>
 15          <h1>Oświetlenie LED</h1>
 16          <p class="subtitle">ATmega32A (ATB 1.05) · Raspberry Pi · sieć LAN</p>
 17        </div>
 18      </div>
 19      <div class="status-group">
 20        <span id="sim-badge" class="badge badge-sim" hidden>Symulacja</span>
 21        <span id="conn" class="badge badge-off" role="status">Łączenie…</span>
 22      </div>
 23    </header>
```

- **Linia 10:** Początek widocznej treści strony.
- **Linia 11:** Początek paska u góry strony. `class` wskazuje CSS, jak go wyświetlić.
- **Linia 12:** Pudełko na logo i nazwę.
- **Linia 13:** Pomarańczowa kropka. `aria-hidden="true"` mówi czytnikom ekranu, żeby ją pominęły, bo to tylko ozdoba.
- **Linia 14:** Pudełko na teksty.
- **Linia 15:** Główny nagłówek strony.
- **Linia 16:** Podtytuł: elementy projektu (mikrokontroler na płytce ATB, Raspberry Pi, sieć).
- **Linia 17:** Koniec pudełka na teksty.
- **Linia 18:** Koniec pudełka na logo.
- **Linia 19:** Pudełko na znaczki stanu.
- **Linia 20:** Znaczek „Symulacja”. `hidden`: na początku ukryty, a pokaże go JavaScript, gdy działa symulacja.
- **Linia 21:** Znaczek stanu połączenia. `role="status"` sprawia, że czytnik ekranu ogłasza zmiany.
- **Linia 22:** Koniec pudełka na znaczki.
- **Linia 23:** Koniec paska.

### Karta „Sterowanie”

```html
 25    <main class="grid">
 26      <div class="col">
 27        <section class="card card-control" aria-labelledby="h-control">
 28          <h2 id="h-control">Sterowanie</h2>
 29          <div class="lamp-row">
 30            <div id="lamp" class="lamp" aria-hidden="true"><div class="lamp-core"></div></div>
 31            <div>
 32              <p class="big" id="state-text">–</p>
 33              <p class="muted" id="mode-text">–</p>
 34            </div>
 35          </div>
```

- **Linia 25:** Początek głównej części strony, układanej w siatkę.
- **Linia 26:** Pierwsza kolumna.
- **Linia 27:** Karta sterowania. `aria-labelledby` wskazuje nagłówek, który ją opisuje.
- **Linia 28:** Nagłówek karty.
- **Linia 29:** Rząd z obrazkiem lampy i opisem.
- **Linia 30:** Rysowana w CSS lampa, której jasność ustawia JavaScript. W środku ma jasny rdzeń.
- **Linia 31:** Pudełko na tekst.
- **Linia 32:** Duży napis ze stanem, np. „Włączona · 60 %”.
- **Linia 33:** Mniejszy napis z trybem pracy.
- **Linia 34:** Koniec pudełka na tekst.
- **Linia 35:** Koniec rzędu.

```html
 37          <div class="buttons">
 38            <button id="btn-on" class="btn btn-primary" type="button">Włącz</button>
 39            <button id="btn-off" class="btn" type="button">Wyłącz</button>
 40          </div>
```

- **Linia 37:** Pudełko na przyciski.
- **Linia 38:** Przycisk „Włącz”, wyróżniony kolorem (`btn-primary`). `type="button"` oznacza, że to zwykły przycisk.
- **Linia 39:** Przycisk „Wyłącz”.
- **Linia 40:** Koniec pudełka.

```html
 42          <label class="field" for="pwm">
 43            <span class="field-head"><span>Jasność</span><output id="pwm-out">–</output></span>
 44            <input id="pwm" type="range" min="0" max="100" step="1" value="0">
 45          </label>
```

- **Linia 42:** Podpis połączony z polem o `id="pwm"`. Kliknięcie w podpis uaktywnia suwak.
- **Linia 43:** Wiersz z nazwą „Jasność” i miejscem na wartość.
- **Linia 44:** Suwak od 0 do 100, co 1.
- **Linia 45:** Koniec podpisu.

```html
 47          <div class="field">
 48            <span class="field-head"><span>Tryb pracy</span></span>
 49            <div class="segmented" role="group" aria-label="Tryb pracy">
 50              <button type="button" data-mode="MANUAL" aria-pressed="false">Ręczny</button>
 51              <button type="button" data-mode="AUTO" aria-pressed="false">AUTO (wg światła)</button>
 52            </div>
 53          </div>
 54        </section>
```

- **Linia 47:** Pole wyboru trybu.
- **Linia 48:** Wiersz z nazwą „Tryb pracy”.
- **Linia 49:** Grupa przycisków wyglądająca jak jeden przełącznik. `role="group"` i `aria-label` opisują ją czytnikom ekranu.
- **Linia 50:** Przycisk „Ręczny”. `data-mode` to wartość odczytywana przez JavaScript, a `aria-pressed` mówi, czy jest aktywny.
- **Linia 51:** Przycisk „AUTO”.
- **Linia 52:** Koniec grupy.
- **Linia 53:** Koniec pola.
- **Linia 54:** Koniec karty.

### Karta „Alarm temperatury”

```html
 56        <section class="card" aria-labelledby="h-alarm">
 57          <h2 id="h-alarm">Alarm temperatury</h2>
 58          <p id="alarm-text" class="alarm-text">–</p>
 59          <form id="tlim-form" class="inline-form">
 60            <label for="tlim">Próg [°C]</label>
 61            <input id="tlim" type="number" min="0" max="125" step="1" inputmode="numeric" required>
 62            <button class="btn btn-small" type="submit">Ustaw</button>
 63          </form>
 64        </section>
 65      </div>
```

- **Linia 56:** Początek karty alarmu.
- **Linia 57:** Nagłówek karty.
- **Linia 58:** Napis ze stanem alarmu.
- **Linia 59:** Formularz ustawiania progu.
- **Linia 60:** Podpis pola.
- **Linia 61:** Pole na liczbę od 0 do 125. `inputmode="numeric"` otwiera na telefonie klawiaturę numeryczną, a `required` nie pozwala wysłać pustego pola.
- **Linia 62:** Przycisk wysyłający formularz (`type="submit"`).
- **Linia 63:** Koniec formularza.
- **Linia 64:** Koniec karty.
- **Linia 65:** Koniec pierwszej kolumny.

### Karta „Stan urządzenia”

```html
 67      <div class="col">
 68        <section class="card" aria-labelledby="h-measure">
 69          <div class="card-head">
 70            <h2 id="h-measure">Stan urządzenia</h2>
 71            <button id="btn-refresh" class="btn btn-small" type="button">Odśwież</button>
 72          </div>
 73          <dl class="metrics">
 74            <div class="metric" id="m-temp">
 75              <dt>Temperatura</dt>
 76              <dd><span id="temp">–</span></dd>
 77            </div>
 78            <div class="metric">
 79              <dt>Oświetlenie otoczenia</dt>
 80              <dd><span id="ldr">–</span></dd>
 81            </div>
 82            <div class="metric">
 83              <dt>Potencjometr</dt>
 84              <dd><span id="pot">–</span></dd>
 85            </div>
 86            <div class="metric">
 87              <dt>Czas pracy MCU</dt>
 88              <dd><span id="uptime">–</span></dd>
 89            </div>
 90          </dl>
 91          <p class="muted small" id="fw">Firmware: –</p>
 92        </section>
```

- **Linia 67:** Druga kolumna.
- **Linia 68:** Początek karty pomiarów.
- **Linia 69:** Nagłówek karty, z tytułem i przyciskiem w jednym wierszu.
- **Linia 70:** Tytuł karty.
- **Linia 71:** Przycisk „Odśwież”.
- **Linia 72:** Koniec nagłówka karty.
- **Linia 73:** Początek listy pomiarów. `dl` to lista definicji: nazwa i wartość.
- **Linia 74:** Pudełko pomiaru temperatury. Ma własne `id`, bo przy alarmie zmieni kolor.
- **Linia 75:** Nazwa pomiaru.
- **Linia 76:** Wartość. Do elementu o `id="temp"` JavaScript wpisze wynik.
- **Linia 77:** Koniec pudełka.
- **Linia 78:** Pudełko pomiaru światła.
- **Linia 79:** Nazwa pomiaru.
- **Linia 80:** Wartość.
- **Linia 81:** Koniec pudełka.
- **Linia 82:** Pudełko potencjometru.
- **Linia 83:** Nazwa pomiaru.
- **Linia 84:** Wartość.
- **Linia 85:** Koniec pudełka.
- **Linia 86:** Pudełko czasu pracy układu.
- **Linia 87:** Nazwa pomiaru.
- **Linia 88:** Wartość.
- **Linia 89:** Koniec pudełka.
- **Linia 90:** Koniec listy pomiarów.
- **Linia 91:** Napis z wersją firmware, małym i przygaszonym tekstem.
- **Linia 92:** Koniec karty.

### Karta „Wirtualna płytka”

Widoczna tylko w trybie symulacji. Zastępuje prawdziwe elementy na płytce.

```html
 94        <section class="card card-board" id="board" aria-labelledby="h-board" hidden>
 95          <h2 id="h-board">Wirtualna płytka</h2>
 96          <p class="muted small">Tryb symulacji: tu zmieniasz to, co na prawdziwej płytce robią przycisk i czujniki.</p>
 97          <div class="board-leds">
 98            <span class="board-led"><span id="sim-led" class="led led-main"></span>PD5 – dioda LED8</span>
 99            <span class="board-led"><span id="sim-status" class="led led-status"></span>PC0 – status LED1</span>
100            <span class="board-led muted" id="sim-duty" hidden></span>
101          </div>
```

- **Linia 94:** Początek karty. `hidden`: na początku ukryta.
- **Linia 95:** Nagłówek karty.
- **Linia 96:** Wyjaśnienie, do czego służy karta.
- **Linia 97:** Wiersz z rysunkami diod.
- **Linia 98:** Dioda główna z podpisem. JavaScript ustawi jej jasność.
- **Linia 99:** Dioda statusu z podpisem.
- **Linia 100:** Miejsce na wypełnienie PWM. Na początku ukryte, działa tylko w simavr.
- **Linia 101:** Koniec wiersza.

```html
102          <label class="field" for="sim-pot">
103            <span class="field-head"><span>Potencjometr (PA0)</span><output id="sim-pot-out">–</output></span>
104            <input id="sim-pot" type="range" min="0" max="1023" value="512">
105          </label>
106          <label class="field" for="sim-ldr">
107            <span class="field-head"><span>Światło na fotorezystorze (PA1)</span><output id="sim-ldr-out">–</output></span>
108            <input id="sim-ldr" type="range" min="0" max="1023" value="500">
109          </label>
110          <label class="field" for="sim-temp">
111            <span class="field-head"><span>Temperatura czujnika DS18B20</span><output id="sim-temp-out">–</output></span>
112            <input id="sim-temp" type="range" min="0" max="80" step="0.5" value="22">
113          </label>
```

- **Linia 102:** Podpis suwaka potencjometru.
- **Linia 103:** Nazwa i miejsce na wartość.
- **Linia 104:** Suwak od 0 do 1023, czyli w zakresie przetwornika ADC. Startowo 512.
- **Linia 105:** Koniec podpisu.
- **Linia 106:** Podpis suwaka fotorezystora.
- **Linia 107:** Nazwa i miejsce na wartość.
- **Linia 108:** Suwak od 0 do 1023. Startowo 500.
- **Linia 109:** Koniec podpisu.
- **Linia 110:** Podpis suwaka temperatury.
- **Linia 111:** Nazwa i miejsce na wartość.
- **Linia 112:** Suwak od 0 do 80 °C, co 0,5. Startowo 22.
- **Linia 113:** Koniec podpisu.

```html
114          <div class="buttons buttons-wrap">
115            <button class="btn btn-small" type="button" data-sim-button="short">Przycisk K1 (PD2)</button>
116            <button class="btn btn-small" type="button" data-sim-button="long">Przytrzymaj 1 s (tryb)</button>
117            <button class="btn btn-small btn-ghost" type="button" id="sim-reset">Reset MCU</button>
118            <button class="btn btn-small btn-ghost" type="button" id="sim-power">Zanik zasilania</button>
119          </div>
120        </section>
121      </div>
```

- **Linia 114:** Pudełko na przyciski, które mogą zawijać się do kolejnego wiersza.
- **Linia 115:** Krótkie naciśnięcie wirtualnego przycisku.
- **Linia 116:** Długie naciśnięcie.
- **Linia 117:** Reset układu.
- **Linia 118:** Symulacja zaniku zasilania.
- **Linia 119:** Koniec pudełka.
- **Linia 120:** Koniec karty.
- **Linia 121:** Koniec drugiej kolumny.

### Dziennik i dół strony

```html
123      <section class="card card-log" aria-labelledby="h-log">
124        <h2 id="h-log">Dziennik zdarzeń</h2>
125        <ol id="log" class="log" aria-live="polite"></ol>
126      </section>
127    </main>
128
129    <div id="toast" class="toast" role="alert" hidden></div>
130    <script src="/static/app.js"></script>
131  </body>
132  </html>
```

- **Linia 123:** Początek karty dziennika.
- **Linia 124:** Nagłówek karty.
- **Linia 125:** Pusta lista, do której JavaScript będzie dopisywać wpisy. `aria-live="polite"` sprawia, że czytnik ekranu ogłosi nowe wpisy, gdy skończy mówić.
- **Linia 126:** Koniec karty.
- **Linia 127:** Koniec głównej części strony.
- **Linia 129:** Wyskakujący komunikat o błędzie. `role="alert"` każe czytnikowi ekranu ogłosić go od razu. Na początku ukryty.
- **Linia 130:** Dołącz i uruchom kod strony, `app.js`. Jest na końcu, by elementy strony już istniały.
- **Linia 131:** Koniec widocznej treści.
- **Linia 132:** Koniec strony.

---

[← Poprzedni](37-serwer-wdrozenie.md) · [Spis treści](README.md) · [Następny →](41-www-app-css.md)
