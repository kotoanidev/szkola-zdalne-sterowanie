# Strona WWW: `app.js`, zachowanie strony

[← Poprzedni](41-www-app-css.md) · [Spis treści](README.md) · [Następny →](50-testy-conftest-protocol.md)

**Po co ten plik:** ożywia stronę. Reaguje na kliknięcia i ruch suwaków, wysyła polecenia do serwera, odbiera zdarzenia na żywo i aktualizuje to, co widać. Składnia jest wytłumaczona w [podstawach WWW](03-podstawy-www.md).

**Główna idea:** jedna funkcja `render(s)` dostaje stan i rysuje według niego całą stronę. Skąd stan przyszedł (odpowiedź serwera czy zdarzenie), nie ma znaczenia.

## `server/app/static/app.js`

[Otwórz ten plik w repozytorium](../../server/app/static/app.js)

### Przygotowanie

```javascript
  1  "use strict";
  2
  3  const $ = (id) => document.getElementById(id);
  4  const els = {
  5    conn: $("conn"), simBadge: $("sim-badge"), lamp: $("lamp"),
  6    stateText: $("state-text"), modeText: $("mode-text"),
  7    btnOn: $("btn-on"), btnOff: $("btn-off"), pwm: $("pwm"), pwmOut: $("pwm-out"),
  8    temp: $("temp"), mTemp: $("m-temp"), ldr: $("ldr"), pot: $("pot"), uptime: $("uptime"), fw: $("fw"),
  9    alarmText: $("alarm-text"), tlim: $("tlim"), tlimForm: $("tlim-form"),
 10    board: $("board"), simLed: $("sim-led"), simStatus: $("sim-status"),
 11    log: $("log"), toast: $("toast"),
 12  };
 13  const modeButtons = document.querySelectorAll(".segmented button");
```

- **Linia 1:** Włącza tryb ścisły. Przeglądarka zgłosi błędy, które normalnie przemilczałaby, np. literówkę w nazwie zmiennej.
- **Linia 3:** Krótka funkcja `$`: znajduje element po jego `id`. Dzięki niej nie trzeba pisać długiej nazwy.
- **Linia 4:** Początek obiektu `els`, czyli zbioru potrzebnych elementów strony.
- **Linia 5:** Znaczek połączenia, znaczek symulacji i lampa.
- **Linia 6:** Napisy ze stanem i trybem.
- **Linia 7:** Przyciski „Włącz” i „Wyłącz”, suwak jasności i jego wartość.
- **Linia 8:** Pomiary: temperatura z jej pudełkiem, światło, potencjometr, czas pracy i wersja firmware.
- **Linia 9:** Napis o alarmie, pole progu i formularz.
- **Linia 10:** Karta wirtualnej płytki i jej diody.
- **Linia 11:** Dziennik i komunikat o błędzie.
- **Linia 12:** Koniec obiektu.
- **Linia 13:** Wszystkie przyciski przełącznika trybu.

```javascript
 15  let state = null;
 16  let simLive = false;
 17  let draggingPwm = false;
 18  let tlimEditing = false;
```

- **Linia 15:** Ostatnio otrzymany stan. Na początku `null`, czyli brak.
- **Linia 16:** Czy dane diod przychodzą na żywo z emulatora.
- **Linia 17:** Czy użytkownik właśnie przesuwa suwak jasności. Wtedy nie chcemy mu go przestawiać.
- **Linia 18:** Czy użytkownik właśnie wpisuje próg alarmu.

### Formatowanie

```javascript
 20  // --- formatowanie -----------------------------------------------------------
 21
 22  const fmtNumber = (v, digits = 1) =>
 23    v.toLocaleString("pl-PL", { minimumFractionDigits: digits, maximumFractionDigits: digits });
 24  const percent = (raw) => Math.round((raw / 1023) * 100);
 25
 26  function fmtUptime(s) {
 27    const h = Math.floor(s / 3600);
 28    const m = Math.floor((s % 3600) / 60);
 29    const sec = s % 60;
 30    const pad = (n) => String(n).padStart(2, "0");
 31    return h > 0 ? `${h}\u00a0h ${pad(m)}\u00a0min` : `${m}\u00a0min ${pad(sec)}\u00a0s`;
 32  }
 33
 34  function fmtTime(t) {
 35    return new Date(t * 1000).toLocaleTimeString("pl-PL");
 36  }
```

- **Linia 20:** Komentarz oddzielający tę część.
- **Linia 22:** Funkcja formatująca liczbę. Parametr `digits` to liczba miejsc po przecinku, domyślnie 1.
- **Linia 23:** Zamienia liczbę na tekst w polskim zapisie, np. `23,5`.
- **Linia 24:** Funkcja przeliczająca odczyt 0–1023 na procenty, z zaokrągleniem.
- **Linia 26:** Funkcja zamieniająca sekundy na tekst, np. „1 h 05 min”.
- **Linia 27:** Liczba pełnych godzin: 3600 s to 1 h.
- **Linia 28:** Liczba pełnych minut z tego, co zostało po godzinach.
- **Linia 29:** Liczba sekund, które zostały.
- **Linia 30:** Funkcja wewnętrzna: dopisuje zero z przodu, by liczba miała dwie cyfry, np. `05`.
- **Linia 31:** Jeśli są godziny, pokaż godziny i minuty, a w przeciwnym razie minuty i sekundy. ` ` to twarda spacja.
- **Linia 32:** Koniec funkcji.
- **Linia 34:** Funkcja zamieniająca czas na tekst godziny.
- **Linia 35:** Czas przychodzi w sekundach, a `Date` chce milisekund, stąd mnożenie przez 1000. Wynik jest w polskim formacie.
- **Linia 36:** Koniec funkcji.

### Komunikacja z serwerem

```javascript
 38  // --- komunikacja ------------------------------------------------------------
 39
 40  let toastTimer = null;
 41  function showError(message) {
 42    els.toast.textContent = message;
 43    els.toast.hidden = false;
 44    clearTimeout(toastTimer);
 45    toastTimer = setTimeout(() => { els.toast.hidden = true; }, 4000);
 46  }
```

- **Linia 38:** Komentarz oddzielający tę część.
- **Linia 40:** Identyfikator licznika czasu dla komunikatu.
- **Linia 41:** Funkcja pokazująca komunikat o błędzie.
- **Linia 42:** Wstaw treść.
- **Linia 43:** Pokaż komunikat.
- **Linia 44:** Anuluj poprzednie odliczanie, jeśli trwało.
- **Linia 45:** Ukryj komunikat po 4000 ms, czyli 4 sekundach.
- **Linia 46:** Koniec funkcji.

```javascript
 48  async function api(method, url, body) {
 49    try {
 50      const r = await fetch(url, {
 51        method,
 52        headers: body ? { "Content-Type": "application/json" } : {},
 53        body: body ? JSON.stringify(body) : undefined,
 54      });
 55      const data = await r.json().catch(() => ({}));
 56      if (!r.ok) throw new Error(data.error || `HTTP ${r.status}`);
 57      return data;
 58    } catch (e) {
 59      showError(`Nie udało się: ${e.message}`);
 60      return null;
 61    }
 62  }
```

- **Linia 48:** Funkcja wysyłająca zapytanie do serwera. `async`: może czekać.
- **Linia 49:** Próbuj.
- **Linia 50:** Wyślij zapytanie pod adres i poczekaj na odpowiedź.
- **Linia 51:** Metoda: `GET` albo `POST`.
- **Linia 52:** Jeśli są dane do wysłania, zaznacz, że to JSON.
- **Linia 53:** Zamień dane na tekst JSON, jeśli są.
- **Linia 54:** Koniec opcji zapytania.
- **Linia 55:** Odczytaj JSON z odpowiedzi. Jeśli się nie uda, przyjmij pusty obiekt.
- **Linia 56:** Jeśli kod odpowiedzi oznacza błąd, zgłoś błąd z opisem od serwera albo z samym kodem.
- **Linia 57:** Zwróć dane.
- **Linia 58:** Jeśli wystąpił jakikolwiek błąd...
- **Linia 59:** ...pokaż komunikat użytkownikowi...
- **Linia 60:** ...i zwróć `null`.
- **Linia 61:** Koniec obsługi błędu.
- **Linia 62:** Koniec funkcji.

```javascript
 64  async function setState(changes) {
 65    const data = await api("POST", "/api/state", changes);
 66    if (data) render(data);
 67  }
```

- **Linia 64:** Funkcja wysyłająca zmiany stanu.
- **Linia 65:** Wyślij je metodą `POST` i poczekaj na nowy stan.
- **Linia 66:** Jeśli odpowiedź przyszła, narysuj stronę od nowa.
- **Linia 67:** Koniec funkcji.

### Rysowanie strony

```javascript
 69  // --- widok -------------------------------------------------------------------
 70
 71  function render(s) {
 72    state = s;
 73    const connected = s.connected;
 74    els.conn.textContent = connected ? "Połączono" : "Brak połączenia z MCU";
 75    els.conn.className = `badge ${connected ? "badge-on" : "badge-off"}`;
```

- **Linia 69:** Komentarz oddzielający tę część.
- **Linia 71:** Główna funkcja. Dostaje stan i aktualizuje stronę.
- **Linia 72:** Zapamiętaj stan.
- **Linia 73:** Zapamiętaj, czy jest połączenie.
- **Linia 74:** Ustaw tekst znaczka stanu połączenia.
- **Linia 75:** Ustaw kolor znaczka, czyli jego klasę: zieloną lub czerwoną.

```javascript
 77    const level = s.on ? s.pwm / 100 : 0;
 78    els.lamp.style.setProperty("--b", level);
 79    if (!simLive) els.simLed.style.setProperty("--b", level);
 80    els.stateText.textContent = s.on ? `Włączona · ${s.pwm}\u00a0%` : "Wyłączona";
 81    els.modeText.textContent = s.mode === "AUTO"
 82      ? "Tryb AUTO – jasność według oświetlenia"
 83      : "Tryb ręczny";
```

- **Linia 77:** Oblicz jasność od 0 do 1. Jeśli wyjście jest wyłączone, to 0.
- **Linia 78:** Przekaż jasność lampie jako zmienną CSS `--b`.
- **Linia 79:** Jeśli nie ma danych na żywo z emulatora, ustaw też wirtualną diodę.
- **Linia 80:** Napis stanu: np. „Włączona · 60 %” albo „Wyłączona”.
- **Linia 81:** Napis trybu. Jeśli to tryb AUTO...
- **Linia 82:** ...napis o trybie AUTO...
- **Linia 83:** ...a w przeciwnym razie „Tryb ręczny”.

```javascript
 85    els.btnOn.classList.toggle("active", s.on);
 86    els.btnOff.classList.toggle("active", !s.on);
 87    if (!draggingPwm) {
 88      els.pwm.value = s.pwm;
 89      els.pwmOut.textContent = `${s.pwm}\u00a0%`;
 90    }
 91    modeButtons.forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.mode === s.mode)));
```

- **Linia 85:** Wyróżnij przycisk „Włącz”, gdy wyjście jest włączone.
- **Linia 86:** Wyróżnij przycisk „Wyłącz”, gdy jest wyłączone.
- **Linia 87:** Jeśli użytkownik nie przesuwa suwaka...
- **Linia 88:** ...ustaw suwak na aktualną jasność...
- **Linia 89:** ...i wypisz wartość.
- **Linia 90:** Koniec warunku.
- **Linia 91:** Dla każdego przycisku trybu ustaw informację, czy to tryb aktualny.

```javascript
 93    els.temp.textContent = s.temp === null ? "–" : `${fmtNumber(s.temp)}\u00a0°C`;
 94    els.mTemp.classList.toggle("alarm", s.alarm);
 95    els.ldr.textContent = `${percent(s.ldr)}\u00a0%`;
 96    els.pot.textContent = `${percent(s.pot)}\u00a0%`;
 97    els.uptime.textContent = fmtUptime(s.uptime_s);
```

- **Linia 93:** Wypisz temperaturę, a jeśli jej nie ma, kreskę.
- **Linia 94:** Oznacz pudełko temperatury przy alarmie.
- **Linia 95:** Wypisz światło w procentach.
- **Linia 96:** Wypisz potencjometr w procentach.
- **Linia 97:** Wypisz czas pracy.

```javascript
 99    els.alarmText.textContent = s.alarm
100      ? `ALARM: temperatura przekroczyła ${s.tlim}\u00a0°C`
101      : `Bez alarmu (próg ${s.tlim}\u00a0°C)`;
102    els.alarmText.classList.toggle("alarm", s.alarm);
103    els.simStatus.classList.toggle("blink", s.alarm && !simLive);
104    if (!tlimEditing) els.tlim.value = s.tlim;
105
106    for (const el of [els.btnOn, els.btnOff, els.pwm, ...modeButtons]) el.disabled = !connected;
107  }
```

- **Linia 99:** Napis o alarmie. Jeśli alarm trwa...
- **Linia 100:** ...napis ostrzegawczy z progiem...
- **Linia 101:** ...a w przeciwnym razie napis o braku alarmu.
- **Linia 102:** Oznacz napis przy alarmie.
- **Linia 103:** Włącz miganie wirtualnej diody statusu przy alarmie, gdy nie ma danych na żywo.
- **Linia 104:** Jeśli użytkownik nie wpisuje progu, ustaw go w polu.
- **Linia 106:** Dla każdego elementu sterowania: wyłącz go, jeśli nie ma połączenia. `...modeButtons` dopisuje przyciski trybu do listy.
- **Linia 107:** Koniec funkcji.

### Dopisywanie do dziennika

```javascript
109  function addLog(entry) {
110    const li = document.createElement("li");
111    const time = document.createElement("time");
112    time.textContent = fmtTime(entry.t);
113    const src = document.createElement("span");
114    src.className = `src src-${entry.source}`;
115    src.textContent = { mcu: "MCU", web: "WWW", server: "serwer" }[entry.source] || entry.source;
116    const text = document.createElement("span");
117    text.className = "text";
118    text.textContent = entry.text;
119    li.append(time, src, text);
120    els.log.prepend(li);
121    while (els.log.children.length > 50) els.log.lastChild.remove();
122  }
```

- **Linia 109:** Funkcja dostaje wpis i tworzy na stronie nowy element listy.
- **Linia 110:** Utwórz element listy.
- **Linia 111:** Utwórz element czasu.
- **Linia 112:** Wstaw czas.
- **Linia 113:** Utwórz element źródła.
- **Linia 114:** Nadaj mu klasę zależną od źródła, co określa kolor.
- **Linia 115:** Zamień nazwę techniczną na czytelną. Jeśli nie ma jej na liście, zostaw oryginalną.
- **Linia 116:** Utwórz element treści.
- **Linia 117:** Nadaj mu klasę.
- **Linia 118:** Wstaw treść. `textContent` traktuje ją jako zwykły tekst, co jest bezpieczne.
- **Linia 119:** Włóż trzy elementy do elementu listy.
- **Linia 120:** Włóż wpis na początek dziennika, najnowszy będzie na górze.
- **Linia 121:** Dopóki wpisów jest więcej niż 50, usuwaj najstarszy, czyli ostatni.
- **Linia 122:** Koniec funkcji.

### Zdarzenia na żywo

```javascript
124  // --- zdarzenia na żywo (SSE) ----------------------------------------------------
125
126  function connectEvents() {
127    const es = new EventSource("/api/events");
128    es.addEventListener("init", (e) => {
129      const d = JSON.parse(e.data);
130      els.log.replaceChildren();
131      d.log.forEach(addLog);
132      els.simBadge.hidden = !d.simulated;
133      els.simBadge.textContent = d.simulator === "avr" ? "Symulacja: simavr" : "Symulacja";
134      els.board.hidden = !d.simulated;
135      els.fw.textContent = `Firmware: ${d.firmware || "–"}`;
136      if (d.sim) initSim(d.sim);
137      if (d.simulator === "avr" && d.sim) renderSim(d.sim);
138      render(d.state);
139    });
```

- **Linia 124:** Komentarz oddzielający tę część.
- **Linia 126:** Funkcja otwierająca połączenie SSE.
- **Linia 127:** Otwórz stałe połączenie. Przeglądarka sama wznowi je, gdy się zerwie.
- **Linia 128:** Gdy przyjdzie wiadomość `init`, wykonaj:
- **Linia 129:** Zamień tekst JSON na obiekt.
- **Linia 130:** Wyczyść dziennik.
- **Linia 131:** Dodaj wszystkie wpisy.
- **Linia 132:** Pokaż znaczek symulacji, jeśli działa symulacja.
- **Linia 133:** Ustaw jego tekst, zależnie od rodzaju symulacji.
- **Linia 134:** Pokaż kartę wirtualnej płytki, jeśli działa symulacja.
- **Linia 135:** Wypisz wersję firmware, a jeśli jej nie ma, kreskę.
- **Linia 136:** Jeśli są dane płytki, ustaw jej suwaki.
- **Linia 137:** Jeśli to simavr, narysuj też diody.
- **Linia 138:** Narysuj stronę.
- **Linia 139:** Koniec obsługi `init`.

```javascript
140    es.addEventListener("state", (e) => render(JSON.parse(e.data)));
141    es.addEventListener("sim", (e) => renderSim(JSON.parse(e.data)));
142    es.addEventListener("log", (e) => {
143      const entry = JSON.parse(e.data);
144      addLog(entry);
145      const ver = /ver=(\S+)/.exec(entry.text);
146      if (ver) els.fw.textContent = `Firmware: ${ver[1]}`;
147    });
148    es.onerror = () => {
149      els.conn.textContent = "Brak połączenia z serwerem";
150      els.conn.className = "badge badge-off";
151    };
152  }
```

- **Linia 140:** Gdy przyjdzie nowy stan, narysuj stronę.
- **Linia 141:** Gdy przyjdą dane płytki, narysuj diody.
- **Linia 142:** Gdy przyjdzie nowy wpis dziennika:
- **Linia 143:** Zamień tekst JSON na obiekt.
- **Linia 144:** Dodaj wpis.
- **Linia 145:** Sprawdź wyrażeniem regularnym, czy treść zawiera `ver=...`. Zdarzenie startu podaje wersję firmware.
- **Linia 146:** Jeśli zawiera, zaktualizuj wypisaną wersję.
- **Linia 147:** Koniec obsługi.
- **Linia 148:** Gdy połączenie się zerwie:
- **Linia 149:** Pokaż brak połączenia z serwerem.
- **Linia 150:** Ustaw czerwony znaczek.
- **Linia 151:** Koniec obsługi.
- **Linia 152:** Koniec funkcji.

### Reakcje na kliknięcia

```javascript
154  // --- sterowanie ------------------------------------------------------------------
155
156  els.btnOn.addEventListener("click", () => setState({ on: true }));
157  els.btnOff.addEventListener("click", () => setState({ on: false }));
158  $("btn-refresh").addEventListener("click", async () => {
159    const data = await api("GET", "/api/state?refresh=1");
160    if (data) render(data);
161  });
162
163  modeButtons.forEach((b) => b.addEventListener("click", () => setState({ mode: b.dataset.mode })));
```

- **Linia 154:** Komentarz oddzielający tę część.
- **Linia 156:** Kliknięcie „Włącz” wysyła zmianę `on: true`.
- **Linia 157:** Kliknięcie „Wyłącz” wysyła zmianę `on: false`.
- **Linia 158:** Kliknięcie „Odśwież”:
- **Linia 159:** Poproś serwer o natychmiastowy odczyt z układu.
- **Linia 160:** Jeśli jest odpowiedź, narysuj stronę.
- **Linia 161:** Koniec obsługi.
- **Linia 163:** Dla każdego przycisku trybu: po kliknięciu wyślij tryb zapisany w jego `data-mode`.

### Ograniczanie liczby zapytań

Ruch suwaka generuje dziesiątki zdarzeń na sekundę. Układ ma wolne łącze, dlatego `throttle` przepuszcza co najwyżej jedno wywołanie na `ms` milisekund, a ostatnie wywołanie zawsze zostanie wykonane.

```javascript
165  function throttle(fn, ms) {
166    let last = 0;
167    let timer = null;
168    return (...args) => {
169      const wait = last + ms - Date.now();
170      clearTimeout(timer);
171      if (wait <= 0) {
172        last = Date.now();
173        fn(...args);
174      } else {
175        timer = setTimeout(() => { last = Date.now(); fn(...args); }, wait);
176      }
177    };
178  }
```

- **Linia 165:** Funkcja dostaje inną funkcję i odstęp. Zwraca jej „przyhamowaną” wersję.
- **Linia 166:** Kiedy ostatnio wywołano funkcję.
- **Linia 167:** Identyfikator zaplanowanego wywołania.
- **Linia 168:** Zwracana funkcja. `...args` zbiera wszystkie jej argumenty.
- **Linia 169:** Ile jeszcze trzeba poczekać.
- **Linia 170:** Anuluj poprzednie zaplanowane wywołanie.
- **Linia 171:** Jeśli nie trzeba czekać...
- **Linia 172:** ...zapamiętaj czas...
- **Linia 173:** ...i wywołaj funkcję.
- **Linia 174:** W przeciwnym razie:
- **Linia 175:** Zaplanuj wywołanie po upływie brakującego czasu.
- **Linia 176:** Koniec warunku.
- **Linia 177:** Koniec zwracanej funkcji.
- **Linia 178:** Koniec `throttle`.

### Suwak jasności

```javascript
180  const sendPwm = throttle((v) => setState({ pwm: v }), 200);
181  els.pwm.addEventListener("pointerdown", () => { draggingPwm = true; });
182  els.pwm.addEventListener("input", () => {
183    els.pwmOut.textContent = `${els.pwm.value}\u00a0%`;
184    sendPwm(Number(els.pwm.value));
185  });
186  els.pwm.addEventListener("change", () => {
187    draggingPwm = false;
188    sendPwm(Number(els.pwm.value));
189  });
```

- **Linia 180:** Funkcja wysyłająca jasność, nie częściej niż raz na 200 ms.
- **Linia 181:** Gdy użytkownik chwyci suwak, oznacz to.
- **Linia 182:** Podczas przesuwania suwaka:
- **Linia 183:** Od razu pokaż wartość.
- **Linia 184:** Wyślij ją, z ograniczeniem częstotliwości. `Number` zamienia tekst na liczbę.
- **Linia 185:** Koniec obsługi.
- **Linia 186:** Gdy użytkownik puści suwak:
- **Linia 187:** Oznacz koniec przesuwania.
- **Linia 188:** Wyślij ostateczną wartość.
- **Linia 189:** Koniec obsługi.

### Pole progu alarmu

```javascript
191  els.tlim.addEventListener("focus", () => { tlimEditing = true; });
192  els.tlim.addEventListener("blur", () => { tlimEditing = false; });
193  els.tlimForm.addEventListener("submit", (e) => {
194    e.preventDefault();
195    tlimEditing = false;
196    setState({ tlim: Number(els.tlim.value) });
197  });
```

- **Linia 191:** Gdy użytkownik wejdzie w pole, oznacz edycję.
- **Linia 192:** Gdy użytkownik wyjdzie z pola, oznacz jej koniec.
- **Linia 193:** Gdy formularz zostanie wysłany:
- **Linia 194:** Zablokuj domyślne przeładowanie strony.
- **Linia 195:** Oznacz koniec edycji.
- **Linia 196:** Wyślij nowy próg.
- **Linia 197:** Koniec obsługi.

### Wirtualna płytka

```javascript
199  // --- wirtualna płytka (tylko symulacja) --------------------------------------------
200
201  const simInputs = {
202    pot: { el: $("sim-pot"), out: $("sim-pot-out"), fmt: (v) => `${percent(v)}\u00a0%` },
203    ldr: { el: $("sim-ldr"), out: $("sim-ldr-out"), fmt: (v) => `${percent(v)}\u00a0%` },
204    temp: { el: $("sim-temp"), out: $("sim-temp-out"), fmt: (v) => `${fmtNumber(v)}\u00a0°C` },
205  };
```

- **Linia 199:** Komentarz oddzielający tę część.
- **Linia 201:** Początek obiektu z suwakami wirtualnej płytki.
- **Linia 202:** Potencjometr: suwak, miejsce na wartość i funkcja formatująca.
- **Linia 203:** Fotorezystor: to samo.
- **Linia 204:** Temperatura: to samo, ale formatowana w stopniach.
- **Linia 205:** Koniec obiektu.

```javascript
207  function initSim(sim) {
208    const values = { pot: sim.pot, ldr: sim.ldr, temp: sim.temp_c };
209    for (const [key, input] of Object.entries(simInputs)) {
210      input.el.value = values[key];
211      input.out.textContent = input.fmt(Number(values[key]));
212    }
213  }
```

- **Linia 207:** Funkcja ustawiająca suwaki na wartości podane przez serwer.
- **Linia 208:** Słownik wartości z dostosowanymi nazwami.
- **Linia 209:** Przejdź po wszystkich suwakach. `Object.entries` daje pary: nazwa i obiekt.
- **Linia 210:** Ustaw suwak.
- **Linia 211:** Wypisz wartość.
- **Linia 212:** Koniec pętli.
- **Linia 213:** Koniec funkcji.

```javascript
215  for (const [key, input] of Object.entries(simInputs)) {
216    const send = throttle((v) => api("POST", "/api/sim", { [key]: v }), 150);
217    input.el.addEventListener("input", () => {
218      const v = Number(input.el.value);
219      input.out.textContent = input.fmt(v);
220      send(v);
221    });
222  }
223  document.querySelectorAll("[data-sim-button]").forEach((b) =>
224    b.addEventListener("click", () => api("POST", "/api/sim", { button: b.dataset.simButton })));
225  $("sim-reset").addEventListener("click", () => api("POST", "/api/sim", { reset: true }));
226  $("sim-power").addEventListener("click", () => api("POST", "/api/sim", { reset: "power" }));
```

- **Linia 215:** Dla każdego suwaka:
- **Linia 216:** Utwórz funkcję wysyłającą, nie częściej niż raz na 150 ms. `{ [key]: v }` tworzy obiekt z kluczem o nazwie zawartej w `key`.
- **Linia 217:** Podczas przesuwania suwaka:
- **Linia 218:** Odczytaj wartość.
- **Linia 219:** Wypisz ją.
- **Linia 220:** Wyślij ją.
- **Linia 221:** Koniec obsługi.
- **Linia 222:** Koniec pętli.
- **Linia 223:** Dla każdego przycisku z atrybutem `data-sim-button`...
- **Linia 224:** ...po kliknięciu wyślij rodzaj naciśnięcia.
- **Linia 225:** Przycisk „Reset MCU” wysyła żądanie resetu.
- **Linia 226:** Przycisk „Zanik zasilania” wysyła żądanie symulacji zaniku.

```javascript
228  // simavr mierzy wypełnienie PWM w rejestrach emulowanego układu
229  function renderSim(sim) {
230    simLive = true;
231    els.simLed.style.setProperty("--b", sim.led / 100);
232    els.simStatus.classList.remove("blink");
233    els.simStatus.classList.toggle("lit", sim.status);
234    const duty = $("sim-duty");
235    duty.hidden = false;
236    duty.textContent = `wypełnienie PWM: ${fmtNumber(sim.duty * 100)}\u00a0%`;
237  }
```

- **Linia 228:** Komentarz: simavr mierzy wypełnienie PWM w rejestrach emulowanego układu.
- **Linia 229:** Funkcja rysująca diody na podstawie danych z emulatora.
- **Linia 230:** Zaznacz, że dane są na żywo.
- **Linia 231:** Ustaw jasność wirtualnej diody.
- **Linia 232:** Wyłącz miganie sterowane przez stronę.
- **Linia 233:** Zapal lub zgaś diodę statusu.
- **Linia 234:** Znajdź element z wypełnieniem.
- **Linia 235:** Pokaż go.
- **Linia 236:** Wypisz wypełnienie w procentach.
- **Linia 237:** Koniec funkcji.

### Start

```javascript
239  connectEvents();
```

- **Linia 239:** Otwórz połączenie SSE. Od tej chwili strona dostaje dane z serwera.

---

[← Poprzedni](41-www-app-css.md) · [Spis treści](README.md) · [Następny →](50-testy-conftest-protocol.md)
