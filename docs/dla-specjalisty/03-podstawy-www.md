# Podstawy WWW: HTTP, HTML, CSS i JavaScript

[← Poprzedni](02-podstawy-pythona.md) · [Spis treści](README.md) · [Następny →](04-podstawy-narzedzi.md)

## Jak działa strona internetowa

**Przeglądarka** (klient) wysyła do **serwera** prośbę, a serwer odsyła odpowiedź. Obie strony rozmawiają według protokołu **HTTP**.

- **Adres IP** i **port** wskazują serwer i usługę, np. port 5000 na Raspberry Pi.
- **Ścieżka** wskazuje, o co prosimy, np. `/api/state`.
- **Metoda** mówi, co chcemy zrobić: `GET` oznacza „daj mi”, a `POST` „przyjmij dane”.
- **Kod odpowiedzi** to liczba opisująca wynik:

| Kod | Znaczenie |
|---|---|
| 200 | wszystko w porządku |
| 400 | błędne zapytanie, np. zły argument |
| 404 | nie ma czegoś takiego |
| 502 | usługa dalej w łańcuchu zgłosiła błąd (u nas mikrokontroler odpowiedział `ERR`) |
| 503 | usługa niedostępna (u nas brak połączenia z mikrokontrolerem) |

**API** to zestaw adresów przeznaczonych dla programów, a nie dla ludzi.

**JSON** to tekstowy format danych, który wygląda jak słownik w Pythonie: `{"on": true, "pwm": 60}`.

**SSE (Server-Sent Events)** to otwarte połączenie, przez które serwer sam wysyła zdarzenia do przeglądarki. Dzięki temu strona widzi naciśnięcie przycisku bez ciągłego pytania serwera.

Strona składa się z trzech warstw:

| Warstwa | Język | Rola | Analogia |
|---|---|---|---|
| treść | HTML | co jest na stronie | szkielet i ściany domu |
| wygląd | CSS | jak to wygląda | farba i meble |
| zachowanie | JavaScript | co się dzieje po kliknięciu | instalacja elektryczna |

## HTML

HTML składa się ze **znaczników**, które otwierają się i zamykają:

```html
<button id="btn-on" class="btn" type="button">Włącz</button>
```

- `<button>` otwiera element, `</button>` go zamyka,
- `Włącz` to treść elementu,
- `id`, `class`, `type` to **atrybuty**, czyli dodatkowe informacje,
- `id` to niepowtarzalna nazwa elementu, której używa JavaScript,
- `class` to nazwa grupy elementów, której używa CSS.

Niektóre elementy nie mają treści ani zamknięcia, np. `<input ...>` albo `<meta ...>`.

Najważniejsze znaczniki:

| Znacznik | Znaczenie |
|---|---|
| `<html>`, `<head>`, `<body>` | cała strona, informacje o stronie, widoczna treść |
| `<h1>`, `<h2>` | nagłówki |
| `<p>` | akapit |
| `<div>`, `<span>` | neutralne pudełka: blokowe (nowy wiersz) i w tekście |
| `<header>`, `<main>`, `<section>` | części strony: nagłówek, główna treść, sekcja |
| `<button>` | przycisk |
| `<input>` | pole do wprowadzania danych, np. suwak (`range`) albo liczba (`number`) |
| `<label>` | podpis pola |
| `<form>` | formularz |
| `<output>` | miejsce na wynik |
| `<dl>`, `<dt>`, `<dd>` | lista definicji: termin i jego wartość |
| `<ol>`, `<li>` | lista numerowana i jej element |
| `<script>` | dołączenie JavaScriptu |
| `<link>` | dołączenie pliku, np. CSS |

Atrybuty zaczynające się od `aria-` oraz atrybut `role` pomagają czytnikom ekranu, z których korzystają osoby niewidome. Atrybuty `data-...` to dowolne własne dane, które odczyta JavaScript. Atrybut `hidden` ukrywa element.

## CSS

CSS to lista reguł. Każda wybiera elementy i nadaje im właściwości:

```css
.btn {
  padding: 10px;
  color: red;
}
```

- `.btn` to **selektor**, tu: wszystkie elementy z klasą `btn`,
- `padding: 10px;` to **właściwość** i jej **wartość**.

Rodzaje selektorów:

| Selektor | Co wybiera |
|---|---|
| `h1` | wszystkie elementy `h1` |
| `.btn` | elementy z klasą `btn` |
| `.btn.active` | elementy z obiema klasami naraz |
| `.log time` | element `time` wewnątrz `.log` |
| `a, b` | i `a`, i `b` |
| `.btn:hover` | element, gdy wskaźnik myszy jest nad nim |
| `[hidden]` | elementy z atrybutem `hidden` |
| `*` | wszystkie elementy |
| `button + button` | przycisk bezpośrednio po innym przycisku |
| `:root` | główny element strony |

**Model pudełkowy:** każdy element to prostokąt. `margin` to odstęp na zewnątrz, `border` to ramka, `padding` to odstęp wewnątrz, a w środku jest treść.

**Jednostki:** `px` to piksele, `rem` to wielokrotność rozmiaru tekstu strony (zwykle 16 px), `%` to procent rodzica, a `1fr` to jedna część wolnego miejsca.

**Kolory:** `#f4f5f7` oznacza kolejno czerwony, zielony i niebieski w zapisie szesnastkowym. `rgba(16, 24, 40, 0.06)` to te same trzy składowe, a na końcu przezroczystość od 0 do 1.

**Zmienne CSS:** `--bg: #f4f5f7;` tworzy zmienną, a `var(--bg)` jej używa. Dzięki temu zmiana motywu to zmiana kilku wartości.

**Rozmieszczanie:**

- `display: flex` układa dzieci w rzędzie lub kolumnie,
- `display: grid` układa dzieci w siatce,
- `gap` to odstęp między dziećmi,
- `justify-content` rozkłada je wzdłuż głównego kierunku,
- `align-items` ustawia je w poprzek tego kierunku.

**Warunki:** `@media (...) { ... }` włącza reguły tylko w określonych warunkach, np. na wąskim ekranie albo gdy system ma ciemny motyw.

## JavaScript

JavaScript to język działający w przeglądarce. Składnia przypomina C.

```javascript
const a = 5;              // stała: nie można przypisać nowej wartości
let b = 1;                // zmienna: można zmieniać
b = 2;

function dodaj(x, y) {    // funkcja
  return x + y;
}
const razy2 = (x) => x * 2;   // funkcja strzałkowa, krótki zapis
```

| Zapis | Znaczenie |
|---|---|
| `===` | czy dokładnie równe (ta sama wartość i ten sam typ) |
| `null`, `undefined` | „brak wartości” |
| `` `Wynik: ${x}` `` | **szablon**, wstawia wartość `x` do tekstu (uwaga na odwrotne apostrofy) |
| `{ a: 1, b: 2 }` | **obiekt**, czyli pary nazwa–wartość, jak słownik |
| `[1, 2, 3]` | **tablica** |
| `[...lista, 4]` | rozkłada elementy listy do nowej tablicy |
| `warunek ? a : b` | jeśli warunek jest prawdziwy, `a`, w przeciwnym razie `b` |
| `a \|\| b` | `a`, chyba że jest „puste”, wtedy `b` |
| `for (const x of lista)` | po kolei każdy element |
| `lista.forEach(f)` | uruchom funkcję `f` dla każdego elementu |
| ` ` | **twarda spacja**: nie pozwala przenieść `60 %` do dwóch wierszy |

**DOM** to strona widziana przez JavaScript: drzewo obiektów odpowiadających elementom HTML. Można je czytać i zmieniać, a przeglądarka od razu pokazuje skutek.

| Zapis | Znaczenie |
|---|---|
| `document.getElementById("x")` | znajdź element o `id="x"` |
| `document.querySelectorAll(".btn")` | znajdź wszystkie pasujące elementy |
| `el.textContent = "..."` | ustaw tekst elementu |
| `el.hidden = true` | ukryj element |
| `el.disabled = true` | wyłącz przycisk lub pole |
| `el.value` | wartość pola lub suwaka |
| `el.classList.toggle("a", warunek)` | dodaj klasę, gdy warunek jest prawdziwy, a w przeciwnym razie ją usuń |
| `el.style.setProperty("--b", 1)` | ustaw zmienną CSS na tym elemencie |
| `el.addEventListener("click", f)` | gdy nastąpi kliknięcie, uruchom funkcję `f` |
| `document.createElement("li")` | utwórz nowy element |

**Zdarzenie** to coś, co się wydarzyło: kliknięcie, ruch suwaka, wiadomość z serwera. Kod reaguje na zdarzenia, zamiast działać od góry do dołu i się kończyć.

**Działania asynchroniczne.** Rozmowa z serwerem trwa, a strona nie może w tym czasie zamarznąć. Słowo `async` oznacza funkcję, która może czekać, a `await` to samo czekanie: „wróć tu, gdy będzie wynik, a na razie rób co innego”.

```javascript
async function pobierz() {
  const r = await fetch("/api/state");   // wyślij zapytanie i poczekaj
  const data = await r.json();           // odczytaj JSON i poczekaj
  return data;
}
```

---

[← Poprzedni](02-podstawy-pythona.md) · [Spis treści](README.md) · [Następny →](04-podstawy-narzedzi.md)
