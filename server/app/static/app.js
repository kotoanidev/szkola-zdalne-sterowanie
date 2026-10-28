"use strict";

const $ = (id) => document.getElementById(id);
const els = {
  conn: $("conn"), simBadge: $("sim-badge"), lamp: $("lamp"),
  stateText: $("state-text"), modeText: $("mode-text"),
  btnOn: $("btn-on"), btnOff: $("btn-off"), pwm: $("pwm"), pwmOut: $("pwm-out"),
  temp: $("temp"), mTemp: $("m-temp"), ldr: $("ldr"), pot: $("pot"), uptime: $("uptime"), fw: $("fw"),
  alarmText: $("alarm-text"), tlim: $("tlim"), tlimForm: $("tlim-form"),
  board: $("board"), simLed: $("sim-led"), simStatus: $("sim-status"),
  log: $("log"), toast: $("toast"),
};
const modeButtons = document.querySelectorAll(".segmented button");

let state = null;
let simLive = false;
let draggingPwm = false;
let tlimEditing = false;

// --- formatowanie -----------------------------------------------------------

const fmtNumber = (v, digits = 1) =>
  v.toLocaleString("pl-PL", { minimumFractionDigits: digits, maximumFractionDigits: digits });
const percent = (raw) => Math.round((raw / 1023) * 100);

function fmtUptime(s) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  const pad = (n) => String(n).padStart(2, "0");
  return h > 0 ? `${h}\u00a0h ${pad(m)}\u00a0min` : `${m}\u00a0min ${pad(sec)}\u00a0s`;
}

function fmtTime(t) {
  return new Date(t * 1000).toLocaleTimeString("pl-PL");
}

// --- komunikacja ------------------------------------------------------------

let toastTimer = null;
function showError(message) {
  els.toast.textContent = message;
  els.toast.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { els.toast.hidden = true; }, 4000);
}

async function api(method, url, body) {
  try {
    const r = await fetch(url, {
      method,
      headers: body ? { "Content-Type": "application/json" } : {},
      body: body ? JSON.stringify(body) : undefined,
    });
    const data = await r.json().catch(() => ({}));
    if (!r.ok) throw new Error(data.error || `HTTP ${r.status}`);
    return data;
  } catch (e) {
    showError(`Nie udało się: ${e.message}`);
    return null;
  }
}

async function setState(changes) {
  const data = await api("POST", "/api/state", changes);
  if (data) render(data);
}

// --- widok -------------------------------------------------------------------

function render(s) {
  state = s;
  const connected = s.connected;
  els.conn.textContent = connected ? "Połączono" : "Brak połączenia z MCU";
  els.conn.className = `badge ${connected ? "badge-on" : "badge-off"}`;

  const level = s.on ? s.pwm / 100 : 0;
  els.lamp.style.setProperty("--b", level);
  if (!simLive) els.simLed.style.setProperty("--b", level);
  els.stateText.textContent = s.on ? `Włączona · ${s.pwm}\u00a0%` : "Wyłączona";
  els.modeText.textContent = s.mode === "AUTO"
    ? "Tryb AUTO – jasność według oświetlenia"
    : "Tryb ręczny";

  els.btnOn.classList.toggle("active", s.on);
  els.btnOff.classList.toggle("active", !s.on);
  if (!draggingPwm) {
    els.pwm.value = s.pwm;
    els.pwmOut.textContent = `${s.pwm}\u00a0%`;
  }
  modeButtons.forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.mode === s.mode)));

  els.temp.textContent = s.temp === null ? "–" : `${fmtNumber(s.temp)}\u00a0°C`;
  els.mTemp.classList.toggle("alarm", s.alarm);
  els.ldr.textContent = `${percent(s.ldr)}\u00a0%`;
  els.pot.textContent = `${percent(s.pot)}\u00a0%`;
  els.uptime.textContent = fmtUptime(s.uptime_s);

  els.alarmText.textContent = s.alarm
    ? `ALARM: temperatura przekroczyła ${s.tlim}\u00a0°C`
    : `Bez alarmu (próg ${s.tlim}\u00a0°C)`;
  els.alarmText.classList.toggle("alarm", s.alarm);
  els.simStatus.classList.toggle("blink", s.alarm && !simLive);
  if (!tlimEditing) els.tlim.value = s.tlim;

  for (const el of [els.btnOn, els.btnOff, els.pwm, ...modeButtons]) el.disabled = !connected;
}

function addLog(entry) {
  const li = document.createElement("li");
  const time = document.createElement("time");
  time.textContent = fmtTime(entry.t);
  const src = document.createElement("span");
  src.className = `src src-${entry.source}`;
  src.textContent = { mcu: "MCU", web: "WWW", server: "serwer" }[entry.source] || entry.source;
  const text = document.createElement("span");
  text.className = "text";
  text.textContent = entry.text;
  li.append(time, src, text);
  els.log.prepend(li);
  while (els.log.children.length > 50) els.log.lastChild.remove();
}

// --- zdarzenia na żywo (SSE) ----------------------------------------------------

function connectEvents() {
  const es = new EventSource("/api/events");
  es.addEventListener("init", (e) => {
    const d = JSON.parse(e.data);
    els.log.replaceChildren();
    d.log.forEach(addLog);
    els.simBadge.hidden = !d.simulated;
    els.simBadge.textContent = d.simulator === "avr" ? "Symulacja: simavr" : "Symulacja";
    els.board.hidden = !d.simulated;
    els.fw.textContent = `Firmware: ${d.firmware || "–"}`;
    if (d.sim) initSim(d.sim);
    if (d.simulator === "avr" && d.sim) renderSim(d.sim);
    render(d.state);
  });
  es.addEventListener("state", (e) => render(JSON.parse(e.data)));
  es.addEventListener("sim", (e) => renderSim(JSON.parse(e.data)));
  es.addEventListener("log", (e) => {
    const entry = JSON.parse(e.data);
    addLog(entry);
    const ver = /ver=(\S+)/.exec(entry.text);
    if (ver) els.fw.textContent = `Firmware: ${ver[1]}`;
  });
  es.onerror = () => {
    els.conn.textContent = "Brak połączenia z serwerem";
    els.conn.className = "badge badge-off";
  };
}

// --- sterowanie ------------------------------------------------------------------

els.btnOn.addEventListener("click", () => setState({ on: true }));
els.btnOff.addEventListener("click", () => setState({ on: false }));
$("btn-refresh").addEventListener("click", async () => {
  const data = await api("GET", "/api/state?refresh=1");
  if (data) render(data);
});

modeButtons.forEach((b) => b.addEventListener("click", () => setState({ mode: b.dataset.mode })));

function throttle(fn, ms) {
  let last = 0;
  let timer = null;
  return (...args) => {
    const wait = last + ms - Date.now();
    clearTimeout(timer);
    if (wait <= 0) {
      last = Date.now();
      fn(...args);
    } else {
      timer = setTimeout(() => { last = Date.now(); fn(...args); }, wait);
    }
  };
}

const sendPwm = throttle((v) => setState({ pwm: v }), 200);
els.pwm.addEventListener("pointerdown", () => { draggingPwm = true; });
els.pwm.addEventListener("input", () => {
  els.pwmOut.textContent = `${els.pwm.value}\u00a0%`;
  sendPwm(Number(els.pwm.value));
});
els.pwm.addEventListener("change", () => {
  draggingPwm = false;
  sendPwm(Number(els.pwm.value));
});

els.tlim.addEventListener("focus", () => { tlimEditing = true; });
els.tlim.addEventListener("blur", () => { tlimEditing = false; });
els.tlimForm.addEventListener("submit", (e) => {
  e.preventDefault();
  tlimEditing = false;
  setState({ tlim: Number(els.tlim.value) });
});

// --- wirtualna płytka (tylko symulacja) --------------------------------------------

const simInputs = {
  pot: { el: $("sim-pot"), out: $("sim-pot-out"), fmt: (v) => `${percent(v)}\u00a0%` },
  ldr: { el: $("sim-ldr"), out: $("sim-ldr-out"), fmt: (v) => `${percent(v)}\u00a0%` },
  temp: { el: $("sim-temp"), out: $("sim-temp-out"), fmt: (v) => `${fmtNumber(v)}\u00a0°C` },
};

function initSim(sim) {
  const values = { pot: sim.pot, ldr: sim.ldr, temp: sim.temp_c };
  for (const [key, input] of Object.entries(simInputs)) {
    input.el.value = values[key];
    input.out.textContent = input.fmt(Number(values[key]));
  }
}

for (const [key, input] of Object.entries(simInputs)) {
  const send = throttle((v) => api("POST", "/api/sim", { [key]: v }), 150);
  input.el.addEventListener("input", () => {
    const v = Number(input.el.value);
    input.out.textContent = input.fmt(v);
    send(v);
  });
}
document.querySelectorAll("[data-sim-button]").forEach((b) =>
  b.addEventListener("click", () => api("POST", "/api/sim", { button: b.dataset.simButton })));
$("sim-reset").addEventListener("click", () => api("POST", "/api/sim", { reset: true }));
$("sim-power").addEventListener("click", () => api("POST", "/api/sim", { reset: "power" }));

// simavr mierzy wypełnienie PWM w rejestrach emulowanego układu
function renderSim(sim) {
  simLive = true;
  els.simLed.style.setProperty("--b", sim.led / 100);
  els.simStatus.classList.remove("blink");
  els.simStatus.classList.toggle("lit", sim.status);
  const duty = $("sim-duty");
  duty.hidden = false;
  duty.textContent = `wypełnienie PWM: ${fmtNumber(sim.duty * 100)}\u00a0%`;
}

connectEvents();
