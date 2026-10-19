import { api, type Series } from "../api";
import { ctx, guard, onConnectionChange, toast, type Page } from "../app";
import { Chart, type Pt } from "../chart";
import { clear, h } from "../dom";
import { chartDose, fmtDose, fmtDoseTotal, fmtNumber, fmtTime, t } from "../i18n";
import { live } from "../live-store";
import { summarize, thin } from "../stats";

const WINDOW_S = 900;

export function dashboardPage(go: (id: string) => void): Page {
  const el = h("section");
  let big: HTMLElement | null = null;
  let chip: HTMLElement | null = null;
  let chart: Chart | null = null;
  let tiles: Record<string, HTMLElement> = {};
  let today: Series | null = null;
  let todayAt = 0;

  async function loadToday(): Promise<void> {
    const id = ctx.connected?.device.id;
    if (!id) return;
    const midnight = new Date();
    midnight.setHours(0, 0, 0, 0);
    const from = Math.floor(midnight.getTime() / 1000);
    today = (await guard(api.series(id, from, from + 86400, 600))) ?? null;
    todayAt = Date.now();
    paintToday();
  }

  function paintToday(): void {
    if (!tiles.avg) return;
    const k = chartDose().k * (100000 / Math.max(1, ctx.connected?.device.sens_x100 || 18000));
    const pts: Pt[] = (today?.points ?? []).map((p) => ({ t: p.t, avg: p.avg * k, min: p.min * k, max: p.max * k }));
    const s = summarize(pts);
    const unit = chartDose().unit;
    const f = (v: number) => `${fmtNumber(v, v >= 100 ? 0 : v >= 10 ? 1 : 2)} ${unit}`;
    tiles.avg.textContent = s ? f(s.avg) : "—";
    tiles.min.textContent = s ? f(s.min) : "—";
    tiles.max.textContent = s ? f(s.max) : "—";
    const bucket = today?.bucket ?? 0;
    const dose = pts.reduce((a, p) => a + (p.avg / chartDose().k * bucket) / 3600 / 1000, 0);
    tiles.dose.textContent = s ? fmtDoseTotal(dose) : "—";
  }

  function tile(key: string, label: string): HTMLElement {
    const v = h("div", { class: "v" }, "—");
    tiles[key] = v;
    return h("div", { class: "tile" }, v, h("div", { class: "l" }, label));
  }

  function update(): void {
    const last = live.last;
    if (big) {
      big.textContent = last ? fmtDose(last.nsv_h) : live.running ? t("live.waiting") : "—";
      big.className = "big" + (live.level === 2 ? " error" : live.level === 1 ? " warn" : "");
    }
    if (chip) {
      const lv = live.level;
      chip.textContent = !last ? "" : t(lv === 2 ? "alarm.danger" : lv === 1 ? "alarm.caution" : "dash.normal");
      chip.className = "chip" + (lv === 2 ? " error" : lv === 1 ? " warn" : " ok");
    }
    if (last && tiles.cpm) {
      tiles.cpm.textContent = fmtNumber(last.cpm, 0);
      tiles.bat.textContent = last.battery === null ? "—" : `${last.battery} %`;
      tiles.session.textContent = fmtDoseTotal(live.doseUsv);
    }
    if (chart && last) {
      const w = live.buf.filter((r) => r.time >= last.time - WINDOW_S);
      const k = chartDose().k;
      chart.opt.unit = chartDose().unit;
      chart.setDomain(last.time - WINDOW_S, last.time);
      chart.setData(thin(w.map((r): Pt => ({ t: r.time, avg: r.nsv_h * k })), 600));
    }
    if (Date.now() - todayAt > 60_000) void loadToday();
  }

  function render(): void {
    clear(el);
    tiles = {};
    el.append(h("h1", null, t("dash.title")));
    const c = ctx.connected;
    if (!c) {
      el.append(h("div", { class: "card" }, h("h2", null, t("dash.none_title")), h("p", null, t("dash.none_text")),
        h("div", { class: "row" }, h("button", { class: "btn primary", onclick: () => go("device") }, t("dash.connect")))));
      return;
    }
    big = h("div", { class: "big" }, "—");
    chip = h("span", { class: "chip" });
    const canvas = h("canvas", { class: "chart small" });
    el.append(
      h("div", { class: "card hero" },
        h("div", { class: "row" }, h("div", { class: "muted" }, `${c.device.name} · ${c.info.model} · ${c.info.firmware}`), chip),
        big, canvas,
        h("div", { class: "row" },
          h("button", { class: "btn", onclick: () => void (live.running ? live.stop() : live.start()) }, live.running ? t("live.stop") : t("live.start")),
          h("button", { class: "btn", onclick: () => go("live") }, t("dash.more")),
          h("button", { class: "btn", onclick: () => go("stats") }, t("nav.stats")))),
      h("div", { class: "tiles" }, tile("cpm", t("live.cpm")), tile("bat", t("live.battery")), tile("session", t("live.session_dose"))),
      h("h2", null, t("dash.today")),
      h("div", { class: "tiles" }, tile("avg", t("live.avg")), tile("min", t("live.min")), tile("max", t("live.max")), tile("dose", t("dash.today_dose"))),
      h("p", { class: "hint" }, t("dash.record_hint", { time: fmtTime(Math.floor(Date.now() / 1000)) })),
    );
    chart = new Chart(canvas, { interactive: false, unit: chartDose().unit });
    todayAt = 0;
    update();
    if (!ctx.settings?.auto_live && !live.running) toast(t("dash.start_hint"));
  }

  live.subscribe(() => { if (el.isConnected && el.style.display !== "none") update(); });
  onConnectionChange(() => { if (el.isConnected && el.style.display !== "none") render(); });
  return { el, show: render };
}
