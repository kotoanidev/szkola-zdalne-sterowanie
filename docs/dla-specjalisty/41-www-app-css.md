# Strona WWW: `app.css`, wygląd strony

[← Poprzedni](40-www-index-html.md) · [Spis treści](README.md) · [Następny →](42-www-app-js.md)

**Po co ten plik:** mówi przeglądarce, jak mają wyglądać elementy z `index.html`: kolory, odstępy, układ. Podstawy CSS znajdziesz w [podstawach WWW](03-podstawy-www.md).

**Skróty, które często się tu pojawiają:**

- `display: flex` układa dzieci w rzędzie,
- `gap` to odstęp między dziećmi,
- `border-radius` zaokrągla rogi, a `50%` tworzy koło,
- `var(--nazwa)` wstawia wartość zmiennej.

## `server/app/static/app.css`

[Otwórz ten plik w repozytorium](../../server/app/static/app.css)

### Zmienne: paleta kolorów

```css
  1  :root {
  2    --bg: #f4f5f7;
  3    --card: #ffffff;
  4    --text: #1b1f24;
  5    --muted: #5f6b7a;
  6    --line: #e2e6ea;
  7    --accent: #f5a524;
  8    --accent-text: #3d2700;
  9    --ok: #1a7f37;
 10    --bad: #c62828;
 11    --sim: #6f42c1;
 12    --shadow: 0 1px 2px rgba(16, 24, 40, 0.06), 0 1px 3px rgba(16, 24, 40, 0.08);
 13    --radius: 14px;
 14    color-scheme: light dark;
 15  }
```

- **Linia 1:** Początek reguły dla głównego elementu strony. Zmienne określone tutaj są dostępne wszędzie.
- **Linia 2:** Kolor tła strony.
- **Linia 3:** Kolor tła kart.
- **Linia 4:** Kolor tekstu.
- **Linia 5:** Kolor tekstu przygaszonego.
- **Linia 6:** Kolor linii i ramek.
- **Linia 7:** Kolor wyróżnienia, pomarańczowy.
- **Linia 8:** Kolor tekstu na tle wyróżnienia.
- **Linia 9:** Kolor „dobrze”, zielony.
- **Linia 10:** Kolor „źle”, czerwony.
- **Linia 11:** Kolor oznaczenia symulacji, fioletowy.
- **Linia 12:** Cień kart: dwa delikatne cienie nałożone na siebie.
- **Linia 13:** Promień zaokrąglenia rogów.
- **Linia 14:** Strona obsługuje jasny i ciemny motyw, a przeglądarka dopasuje do niego elementy takie jak suwaki.
- **Linia 15:** Koniec reguły.

### Ciemny motyw

```css
 17  @media (prefers-color-scheme: dark) {
 18    :root {
 19      --bg: #0f1216;
 20      --card: #181c22;
 21      --text: #e8ebef;
 22      --muted: #97a3b2;
 23      --line: #2a3038;
 24      --ok: #3fb950;
 25      --bad: #f05252;
 26      --sim: #a78bfa;
 27      --shadow: none;
 28    }
 29  }
```

- **Linia 17:** Poniższe reguły obowiązują tylko, gdy system ma ciemny motyw.
- **Linia 18:** Nadpisujemy zmienne dla głównego elementu.
- **Linia 19:** Ciemne tło strony.
- **Linia 20:** Ciemne tło kart.
- **Linia 21:** Jasny tekst.
- **Linia 22:** Jaśniejszy tekst przygaszony.
- **Linia 23:** Ciemniejsze linie.
- **Linia 24:** Jaśniejszy zielony.
- **Linia 25:** Jaśniejszy czerwony.
- **Linia 26:** Jaśniejszy fiolet.
- **Linia 27:** Bez cienia, bo na ciemnym tle i tak go nie widać.
- **Linia 28:** Koniec reguły dla głównego elementu.
- **Linia 29:** Koniec warunku.

### Reguły ogólne

```css
 31  * { box-sizing: border-box; }
 32  [hidden] { display: none !important; }
 33
 34  body {
 35    margin: 0;
 36    background: var(--bg);
 37    color: var(--text);
 38    font: 16px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
 39  }
 40
 41  h1, h2, p { margin: 0; }
 42  h1 { font-size: 1.15rem; }
 43  h2 { font-size: 1rem; margin-bottom: 12px; }
```

- **Linia 31:** Dla wszystkich elementów: rozmiar obejmuje też ramkę i odstęp wewnętrzny, co ułatwia liczenie szerokości.
- **Linia 32:** Elementy z atrybutem `hidden` są zawsze ukryte. `!important` daje tej regule pierwszeństwo przed innymi.
- **Linia 34:** Początek reguły dla treści strony.
- **Linia 35:** Bez domyślnego marginesu wokół strony.
- **Linia 36:** Tło ze zmiennej.
- **Linia 37:** Kolor tekstu ze zmiennej.
- **Linia 38:** Czcionka: rozmiar 16 px, wysokość wiersza 1,45 i lista czcionek systemowych, pierwsza dostępna zostanie użyta.
- **Linia 39:** Koniec reguły.
- **Linia 41:** Nagłówki i akapity bez domyślnych marginesów.
- **Linia 42:** Rozmiar nagłówka `h1`.
- **Linia 43:** Rozmiar nagłówka `h2` i odstęp pod nim.

### Pasek górny

```css
 45  .topbar {
 46    display: flex;
 47    justify-content: space-between;
 48    align-items: center;
 49    gap: 12px;
 50    padding: 14px 16px;
 51    max-width: 1100px;
 52    margin: 0 auto;
 53  }
```

- **Linia 45:** Początek reguły paska.
- **Linia 46:** Układaj zawartość w rzędzie.
- **Linia 47:** Rozsuń elementy do krawędzi.
- **Linia 48:** Wyśrodkuj je w pionie.
- **Linia 49:** Odstęp między nimi.
- **Linia 50:** Odstęp wewnętrzny: 14 px góra i dół, 16 px boki.
- **Linia 51:** Pasek nie szerszy niż 1100 px.
- **Linia 52:** Wyśrodkuj pasek na stronie: górny i dolny margines 0, boczne automatyczne.
- **Linia 53:** Koniec reguły.

```css
 55  .brand { display: flex; align-items: center; gap: 10px; }
 56  .brand-dot {
 57    width: 14px; height: 14px; border-radius: 50%;
 58    background: var(--accent);
 59    box-shadow: 0 0 12px var(--accent);
 60  }
 61  .subtitle, .muted { color: var(--muted); }
 62  .small { font-size: 0.85rem; }
```

- **Linia 55:** Logo i nazwa w rzędzie, wyśrodkowane, z odstępem.
- **Linia 56:** Początek reguły kropki przy nazwie.
- **Linia 57:** Rozmiar 14 na 14 px i kształt koła.
- **Linia 58:** Kolor wyróżnienia.
- **Linia 59:** Poświata w tym samym kolorze.
- **Linia 60:** Koniec reguły.
- **Linia 61:** Podtytuł i inne elementy `.muted` mają przygaszony kolor.
- **Linia 62:** Klasa `.small` zmniejsza tekst.

### Znaczki stanu

```css
 64  .status-group { display: flex; gap: 8px; flex-wrap: wrap; justify-content: flex-end; }
 65  .badge {
 66    display: inline-flex; align-items: center; gap: 6px;
 67    padding: 4px 10px; border-radius: 999px;
 68    font-size: 0.85rem; font-weight: 600;
 69    border: 1px solid var(--line); background: var(--card);
 70  }
 71  .badge::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: currentColor; }
 72  .badge-on { color: var(--ok); }
 73  .badge-off { color: var(--bad); }
 74  .badge-sim { color: var(--sim); }
```

- **Linia 64:** Grupa znaczków: w rzędzie, z odstępem, zawijane, wyrównane do prawej.
- **Linia 65:** Początek reguły znaczka.
- **Linia 66:** Układ w tekście, wyśrodkowanie i odstęp.
- **Linia 67:** Odstęp wewnętrzny i pełne zaokrąglenie rogów.
- **Linia 68:** Mniejszy, pogrubiony tekst.
- **Linia 69:** Ramka i tło karty.
- **Linia 70:** Koniec reguły.
- **Linia 71:** Przed treścią znaczka dorysuj kropkę o średnicy 8 px w kolorze tekstu (`currentColor`).
- **Linia 72:** Znaczek „połączono” jest zielony.
- **Linia 73:** Znaczek „brak połączenia” jest czerwony.
- **Linia 74:** Znaczek symulacji jest fioletowy.

### Siatka i karty

```css
 76  .grid {
 77    display: grid;
 78    grid-template-columns: repeat(auto-fit, minmax(min(100%, 420px), 1fr));
 79    gap: 16px;
 80    padding: 0 16px 24px;
 81    max-width: 1100px;
 82    margin: 0 auto;
 83  }
```

- **Linia 76:** Początek reguły głównej siatki.
- **Linia 77:** Układaj dzieci w siatce.
- **Linia 78:** Tyle kolumn, ile się zmieści, każda co najmniej 420 px, a na wąskim ekranie 100 %. Wolne miejsce jest dzielone po równo.
- **Linia 79:** Odstęp między komórkami.
- **Linia 80:** Odstęp wewnętrzny: bez góry, po bokach 16 px, na dole 24 px.
- **Linia 81:** Największa szerokość: 1100 px.
- **Linia 82:** Wyśrodkowanie na stronie.
- **Linia 83:** Koniec reguły.

```css
 85  .card {
 86    background: var(--card);
 87    border: 1px solid var(--line);
 88    border-radius: var(--radius);
 89    box-shadow: var(--shadow);
 90    padding: 18px;
 91    min-width: 0;
 92  }
 93  .card-head { display: flex; justify-content: space-between; align-items: baseline; }
 94  .col { display: flex; flex-direction: column; gap: 16px; min-width: 0; }
 95  .card-log { grid-column: 1 / -1; }
```

- **Linia 85:** Początek reguły karty.
- **Linia 86:** Tło karty.
- **Linia 87:** Ramka: 1 px, ciągła linia, kolor linii.
- **Linia 88:** Zaokrąglone rogi.
- **Linia 89:** Cień.
- **Linia 90:** Odstęp wewnętrzny.
- **Linia 91:** Karta może się zwęzić poniżej szerokości swojej treści, co zapobiega rozpychaniu strony na telefonie.
- **Linia 92:** Koniec reguły.
- **Linia 93:** Nagłówek karty: tytuł i przycisk rozsunięte do krawędzi, wyrównane do linii tekstu.
- **Linia 94:** Kolumna: dzieci jedno pod drugim, z odstępem.
- **Linia 95:** Karta dziennika zajmuje wszystkie kolumny siatki, od pierwszej do ostatniej.

### Lampa

Zmienna `--b` przyjmuje wartości od 0 do 1, czyli jasność. Ustawia ją JavaScript, a CSS sam przelicza z niej kolory.

```css
 97  .lamp-row { display: flex; align-items: center; gap: 18px; margin-bottom: 18px; }
 98  .lamp {
 99    --b: 0;
100    width: 84px; height: 84px; flex: none;
101    border-radius: 50%;
102    display: grid; place-items: center;
103    background: radial-gradient(circle, rgba(245, 165, 36, calc(0.15 + var(--b) * 0.5)), transparent 70%);
104    transition: background 0.4s;
105  }
106  .lamp-core {
107    width: 44px; height: 44px; border-radius: 50%;
108    background: color-mix(in srgb, var(--accent) calc(var(--b) * 100%), var(--line));
109    box-shadow: 0 0 calc(var(--b) * 40px) calc(var(--b) * 10px) rgba(245, 165, 36, calc(var(--b) * 0.8));
110    transition: background 0.4s, box-shadow 0.4s;
111  }
112  .big { font-size: 1.5rem; font-weight: 700; }
```

- **Linia 97:** Rząd z lampą i tekstem: wyśrodkowany, z odstępem i marginesem na dole.
- **Linia 98:** Początek reguły lampy, czyli zewnętrznej poświaty.
- **Linia 99:** Domyślna jasność 0.
- **Linia 100:** Rozmiar 84 na 84 px, bez zmieniania rozmiaru (`flex: none`).
- **Linia 101:** Kształt koła.
- **Linia 102:** Wyśrodkuj zawartość.
- **Linia 103:** Tło: promieniste przejście od pomarańczowego do przezroczystości. Przezroczystość pomarańczu zależy od `--b`.
- **Linia 104:** Zmiana tła trwa płynnie 0,4 s.
- **Linia 105:** Koniec reguły.
- **Linia 106:** Początek reguły rdzenia lampy.
- **Linia 107:** Rozmiar 44 na 44 px, kształt koła.
- **Linia 108:** Kolor: mieszanka koloru wyróżnienia z szarością linii. Im większe `--b`, tym więcej pomarańczu.
- **Linia 109:** Poświata, której rozmiar i siła rosną wraz z `--b`.
- **Linia 110:** Płynna zmiana tła i poświaty.
- **Linia 111:** Koniec reguły.
- **Linia 112:** Duży, pogrubiony tekst.

### Przyciski

```css
114  .buttons { display: flex; gap: 10px; margin-bottom: 18px; }
115  .buttons-wrap { flex-wrap: wrap; margin: 4px 0 0; }
116
117  .btn {
118    flex: 1;
119    min-height: 44px;
120    padding: 10px 16px;
121    border-radius: 10px;
122    border: 1px solid var(--line);
123    background: var(--bg);
124    color: var(--text);
125    font: inherit; font-weight: 600;
126    cursor: pointer;
127  }
128  .btn:hover { border-color: var(--muted); }
129  .btn:disabled { opacity: 0.5; cursor: not-allowed; }
130  .btn-primary { background: var(--accent); border-color: var(--accent); color: var(--accent-text); }
131  .btn-small { flex: none; min-height: 36px; padding: 6px 12px; font-size: 0.9rem; }
132  .btn-ghost { background: transparent; }
133  .btn.active { outline: 3px solid color-mix(in srgb, var(--accent) 45%, transparent); }
```

- **Linia 114:** Pudełko na przyciski: w rzędzie, z odstępem i marginesem na dole.
- **Linia 115:** Wariant pudełka z zawijaniem do następnego wiersza.
- **Linia 117:** Początek reguły przycisku.
- **Linia 118:** Przycisk rozciąga się na wolne miejsce.
- **Linia 119:** Co najmniej 44 px wysokości, wygodnie dla palca.
- **Linia 120:** Odstęp wewnętrzny.
- **Linia 121:** Zaokrąglone rogi.
- **Linia 122:** Ramka.
- **Linia 123:** Tło.
- **Linia 124:** Kolor tekstu.
- **Linia 125:** Czcionka taka jak strona, ale pogrubiona.
- **Linia 126:** Po najechaniu kursor zmienia się w „rączkę”.
- **Linia 127:** Koniec reguły.
- **Linia 128:** Po najechaniu ramka ciemnieje.
- **Linia 129:** Wyłączony przycisk jest półprzezroczysty, a kursor pokazuje zakaz.
- **Linia 130:** Przycisk główny: tło i ramka w kolorze wyróżnienia.
- **Linia 131:** Mały przycisk: nie rozciąga się, ma mniejszą wysokość, odstępy i czcionkę.
- **Linia 132:** Przycisk „duch”: przezroczyste tło.
- **Linia 133:** Aktywny przycisk ma obwódkę 3 px w półprzezroczystym kolorze wyróżnienia.

### Pola i suwaki

```css
135  .field { display: block; margin-bottom: 16px; }
136  .field-head { display: flex; justify-content: space-between; margin-bottom: 6px; font-weight: 600; }
137  output { font-variant-numeric: tabular-nums; color: var(--muted); }
138
139  input[type="range"] { width: 100%; accent-color: var(--accent); height: 28px; }
```

- **Linia 135:** Pole formularza: zajmuje cały wiersz, z marginesem na dole.
- **Linia 136:** Wiersz z nazwą i wartością: rozsunięte do krawędzi, pogrubione.
- **Linia 137:** Wartości: cyfry o równej szerokości, by tekst nie „skakał”, oraz kolor przygaszony.
- **Linia 139:** Suwak: pełna szerokość, kolor wyróżnienia i wysokość 28 px.

### Przełącznik trybów

```css
141  .segmented { display: flex; border: 1px solid var(--line); border-radius: 10px; overflow: hidden; }
142  .segmented button {
143    flex: 1; min-height: 44px;
144    border: 0; background: var(--bg); color: var(--text);
145    font: inherit; cursor: pointer;
146  }
147  .segmented button + button { border-left: 1px solid var(--line); }
148  .segmented button[aria-pressed="true"] { background: var(--accent); color: var(--accent-text); font-weight: 700; }
```

- **Linia 141:** Pudełko przełącznika: w rzędzie, z ramką i zaokrągleniem. `overflow: hidden` przycina rogi przycisków w środku.
- **Linia 142:** Początek reguły przycisku w przełączniku.
- **Linia 143:** Rozciąga się na równą część, co najmniej 44 px wysokości.
- **Linia 144:** Bez ramki, tło i kolor tekstu ze zmiennych.
- **Linia 145:** Czcionka jak na stronie, kursor „rączka”.
- **Linia 146:** Koniec reguły.
- **Linia 147:** Każdy przycisk po innym przycisku ma linię z lewej, która je rozdziela.
- **Linia 148:** Aktywny przycisk ma kolor wyróżnienia i pogrubiony tekst.

### Pomiary

```css
150  .metrics { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin: 4px 0 12px; }
151  .metric { background: var(--bg); border-radius: 10px; padding: 10px 12px; }
152  .metric dt { font-size: 0.8rem; color: var(--muted); }
153  .metric dd { margin: 2px 0 0; font-size: 1.3rem; font-weight: 700; font-variant-numeric: tabular-nums; }
154  .metric.alarm { background: color-mix(in srgb, var(--bad) 15%, var(--card)); color: var(--bad); }
```

- **Linia 150:** Lista pomiarów: siatka z dwiema równymi kolumnami i odstępami.
- **Linia 151:** Pudełko pomiaru: tło, zaokrąglenie i odstęp wewnętrzny.
- **Linia 152:** Nazwa pomiaru: mała, przygaszona.
- **Linia 153:** Wartość pomiaru: duża, pogrubiona, cyfry o równej szerokości.
- **Linia 154:** Pudełko z alarmem: tło z domieszką czerwieni i czerwony tekst.

### Alarm

```css
156  .alarm-text { margin-bottom: 12px; font-weight: 600; }
157  .alarm-text.alarm { color: var(--bad); }
158  .inline-form { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
159  .inline-form input {
160    width: 90px; min-height: 36px; padding: 6px 10px;
161    border-radius: 8px; border: 1px solid var(--line);
162    background: var(--bg); color: var(--text); font: inherit;
163  }
```

- **Linia 156:** Napis o alarmie: pogrubiony, z marginesem na dole.
- **Linia 157:** Gdy trwa alarm, napis jest czerwony.
- **Linia 158:** Formularz progu: w rzędzie, wyśrodkowany, z zawijaniem.
- **Linia 159:** Początek reguły pola w formularzu.
- **Linia 160:** Szerokość 90 px, co najmniej 36 px wysokości, odstęp wewnętrzny.
- **Linia 161:** Zaokrąglenie i ramka.
- **Linia 162:** Tło, kolor tekstu i czcionka.
- **Linia 163:** Koniec reguły.

### Wirtualne diody

```css
165  .board-leds { display: flex; gap: 18px; margin: 12px 0 16px; flex-wrap: wrap; }
166  .board-led { display: inline-flex; align-items: center; gap: 8px; font-size: 0.9rem; }
167  .led {
168    --b: 0;
169    width: 18px; height: 18px; border-radius: 50%;
170    border: 1px solid var(--line);
171    transition: background 0.3s, box-shadow 0.3s;
172  }
173  .led-main {
174    background: color-mix(in srgb, #ffcf33 calc(var(--b) * 100%), #3a3320);
175    box-shadow: 0 0 calc(var(--b) * 16px) rgba(255, 207, 51, calc(var(--b)));
176  }
177  .led-status { background: #2a1d1d; }
178  .led-status.blink { animation: blink 0.25s steps(1) infinite; }
179  .led-status.lit { background: #ff4d4d; box-shadow: 0 0 10px #ff4d4d; transition: none; }
180  @keyframes blink { 50% { background: #ff4d4d; box-shadow: 0 0 10px #ff4d4d; } }
```

- **Linia 165:** Wiersz z diodami: w rzędzie, z odstępem, marginesami i zawijaniem.
- **Linia 166:** Dioda z podpisem: wyśrodkowana, z odstępem, mniejszy tekst.
- **Linia 167:** Początek reguły diody.
- **Linia 168:** Domyślna jasność 0.
- **Linia 169:** Rozmiar 18 na 18 px, kształt koła.
- **Linia 170:** Ramka.
- **Linia 171:** Płynna zmiana tła i poświaty.
- **Linia 172:** Koniec reguły.
- **Linia 173:** Początek reguły diody głównej.
- **Linia 174:** Kolor: mieszanka żółtego z ciemnym brązem, zależna od `--b`.
- **Linia 175:** Poświata zależna od `--b`.
- **Linia 176:** Koniec reguły.
- **Linia 177:** Dioda statusu: ciemne tło, gdy zgaszona.
- **Linia 178:** Klasa `blink` uruchamia miganie: cykl 0,25 s, skokowo, bez końca.
- **Linia 179:** Klasa `lit` oznacza świecenie: czerwone tło, poświata i brak płynnej zmiany.
- **Linia 180:** Definicja animacji: w połowie cyklu dioda ma czerwone tło i poświatę.

### Dziennik

```css
182  .log {
183    list-style: none; margin: 0; padding: 0;
184    max-height: 260px; overflow-y: auto;
185    font: 0.88rem/1.5 ui-monospace, "Cascadia Mono", Consolas, monospace;
186  }
187  .log li { display: flex; gap: 10px; padding: 4px 0; border-bottom: 1px solid var(--line); }
188  .log time { color: var(--muted); flex: none; }
189  .log .src { flex: none; width: 62px; color: var(--muted); }
190  .log .src-mcu { color: var(--accent); }
191  .log .src-web { color: var(--ok); }
192  .log .text { overflow-wrap: anywhere; }
```

- **Linia 182:** Początek reguły listy.
- **Linia 183:** Bez numerów, marginesów i odstępów.
- **Linia 184:** Największa wysokość 260 px, a nadmiar można przewijać.
- **Linia 185:** Czcionka o stałej szerokości znaków, jak w terminalu.
- **Linia 186:** Koniec reguły.
- **Linia 187:** Wpis: w rzędzie, z odstępem, z linią na dole.
- **Linia 188:** Czas: przygaszony, nie zwęża się.
- **Linia 189:** Źródło: stała szerokość 62 px, przygaszone.
- **Linia 190:** Źródło „MCU” jest pomarańczowe.
- **Linia 191:** Źródło „WWW” jest zielone.
- **Linia 192:** Treść wpisu może się zawijać w dowolnym miejscu, by długi tekst nie rozpychał strony.

### Komunikat o błędzie

```css
194  .toast {
195    position: fixed; left: 50%; bottom: 20px; transform: translateX(-50%);
196    max-width: calc(100% - 32px);
197    background: var(--bad); color: #fff;
198    padding: 10px 16px; border-radius: 10px; font-weight: 600;
199    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.25);
200  }
```

- **Linia 194:** Początek reguły wyskakującego komunikatu.
- **Linia 195:** Przyklejony do okna: 20 px od dołu, wyśrodkowany w poziomie.
- **Linia 196:** Szerokość najwyżej 100 % minus 32 px.
- **Linia 197:** Czerwone tło i biały tekst.
- **Linia 198:** Odstęp wewnętrzny, zaokrąglenie, pogrubienie.
- **Linia 199:** Cień.
- **Linia 200:** Koniec reguły.

### Bardzo wąskie ekrany

```css
202  @media (max-width: 420px) {
203    .topbar { flex-direction: column; align-items: flex-start; }
204    .status-group { justify-content: flex-start; }
205    .metrics { grid-template-columns: 1fr; }
206  }
```

- **Linia 202:** Poniższe reguły obowiązują, gdy okno ma najwyżej 420 px szerokości.
- **Linia 203:** Pasek górny układa się w kolumnie, wyrównany do lewej.
- **Linia 204:** Znaczki wyrównane do lewej.
- **Linia 205:** Pomiary w jednej kolumnie.
- **Linia 206:** Koniec warunku.

---

[← Poprzedni](40-www-index-html.md) · [Spis treści](README.md) · [Następny →](42-www-app-js.md)
