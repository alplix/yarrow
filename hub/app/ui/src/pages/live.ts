import { save } from "@tauri-apps/plugin-dialog";
import { api, type Reading } from "../api";
import { ctx, guard, onConnectionChange, toast, type Page } from "../app";
import { Chart, type Pt } from "../chart";
import { clear, h } from "../dom";
import { chartDose, doseScale, fmtDose, fmtDoseTotal, fmtNumber, fmtTime, t } from "../i18n";
import { live } from "../live-store";
import { csvEscape, formatDuration, summarize, thin } from "../stats";

const CHART_POINTS = 1200;

type Metric = "nsv" | "cpm" | "e_field" | "h_field";
type Win = 600 | 3600 | 0;          // seconds; 0 is the whole session

export function livePage(): Page {
  const el = h("section");
  let metric: Metric = "nsv";
  let win: Win = 600;
  let chart: Chart | null = null;
  let tiles: Record<string, HTMLElement> = {};
  let statsBox: HTMLElement | null = null;
  let alarmBox: HTMLElement | null = null;
  let toggleBtn: HTMLButtonElement | null = null;

  const local = (r: Reading, n: string) => r.local_only.find((v) => v.name === n)?.value ?? 0;
  const value = (r: Reading): number =>
    metric === "nsv" ? r.nsv_h * chartDose().k : metric === "cpm" ? r.cpm : local(r, metric);
  const unit = () => (metric === "nsv" ? chartDose().unit : ({ cpm: "cpm", e_field: "V/m", h_field: "µT" } as Record<string, string>)[metric]);

  function tile(key: string, label: string): HTMLElement {
    const v = h("div", { class: "v" }, "—");
    tiles[key] = v;
    return h("div", { class: "tile" }, v, h("div", { class: "l" }, label));
  }

  function windowed(): Reading[] {
    const buf = live.buf;
    if (!buf.length) return [];
    if (win === 0) return buf;
    const from = buf[buf.length - 1].time - win;
    let i = buf.length;
    while (i > 0 && buf[i - 1].time >= from) i--;
    return buf.slice(i);
  }

  const fmtMetric = (v: number): string =>
    metric === "nsv" ? `${fmtNumber(v, v >= 100 ? 0 : v >= 10 ? 1 : 2)} ${unit()}` : `${fmtNumber(v, metric === "cpm" ? 0 : metric === "e_field" ? 1 : 2)} ${unit()}`;

  function statsCard(w: Reading[]): HTMLElement {
    const sum = summarize(w.map((r): Pt => ({ t: r.time, avg: value(r) })));
    const box = h("div", { class: "tiles" });
    const add = (v: string, l: string) => box.append(h("div", { class: "tile" }, h("div", { class: "v" }, v), h("div", { class: "l" }, l)));
    if (sum) {
      add(fmtMetric(sum.min), t("live.min"));
      add(fmtMetric(sum.avg), t("live.avg"));
      add(fmtMetric(sum.max), t("live.max"));
    }
    add(fmtNumber(live.buf.length), t("live.samples"));
    add(formatDuration(live.last ? live.last.time - live.started : 0), t("live.elapsed"));
    add(fmtDoseTotal(live.doseUsv), t("live.session_dose"));
    return h("div", null, h("h2", null, t("live.stats")), box);
  }

  function alarmsCard(): HTMLElement {
    const rows = [...live.alarms].reverse().slice(0, 12);
    return h("div", null, h("h2", null, t("live.alarms")),
      rows.length === 0 ? h("p", { class: "muted" }, t("live.alarms_none"))
        : h("table", null,
            h("tr", null, h("th", null, t("log.event.start")), h("th", { class: "num" }, t("log.event.duration")), h("th", { class: "num" }, t("log.event.peak")), h("th", null, t("log.event.level"))),
            rows.map((a) => h("tr", null,
              h("td", null, fmtTime(a.start)),
              h("td", { class: "num" }, a.end ? formatDuration(a.end - a.start) : t("live.ongoing")),
              h("td", { class: "num" }, fmtDose(a.peak)),
              h("td", { class: a.level === 2 ? "error" : "warn" }, t(a.level === 2 ? "alarm.danger" : "alarm.caution"))))));
  }

  function update(): void {
    if (toggleBtn) toggleBtn.textContent = live.running ? t("live.stop") : t("live.start");
    const last = live.last;
    if (last && tiles.nsv) {
      tiles.nsv.textContent = fmtDose(last.nsv_h);
      tiles.nsv.className = "v" + (live.level === 2 ? " error" : live.level === 1 ? " warn" : "");
      tiles.cpm.textContent = fmtNumber(last.cpm, 0);
      tiles.e.textContent = `${fmtNumber(local(last, "e_field"), 1)} V/m`;
      tiles.h.textContent = `${fmtNumber(local(last, "h_field"), 2)} µT`;
      tiles.bat.textContent = last.battery === null ? "—" : `${last.battery} %`;
    }
    if (chart && last) {
      const w = windowed();
      chart.setDomain(win === 0 ? w[0].time : last.time - win, last.time);
      chart.setData(thin(w.map((r): Pt => ({ t: r.time, avg: value(r) })), CHART_POINTS));
      statsBox?.replaceChildren(statsCard(w));
      alarmBox?.replaceChildren(alarmsCard());
    }
  }

  function render(): void {
    clear(el);
    tiles = {};
    el.append(h("h1", null, t("live.title")));
    if (!ctx.connected) {
      el.append(h("p", { class: "muted" }, t("common.no_device")));
      return;
    }
    const canvas = h("canvas", { class: "chart" });
    const metricSel = h("select", { onchange: (e: Event) => { metric = (e.target as HTMLSelectElement).value as Metric; if (chart) chart.opt.unit = unit(); update(); } },
      (["nsv", "cpm", "e_field", "h_field"] as Metric[]).map((m) =>
        h("option", { value: m, selected: m === metric }, t({ nsv: "live.dose", cpm: "live.cpm", e_field: "live.e", h_field: "live.h" }[m]))));
    const winSel = h("select", { onchange: (e: Event) => { win = Number((e.target as HTMLSelectElement).value) as Win; update(); } },
      ([600, 3600, 0] as Win[]).map((w) => h("option", { value: String(w), selected: w === win }, t(`live.win.${w}`))));
    toggleBtn = h("button", { class: "btn primary", onclick: () => void (live.running ? live.stop() : live.start()) }, live.running ? t("live.stop") : t("live.start"));
    statsBox = h("div", { class: "card" });
    alarmBox = h("div", { class: "card" });
    el.append(
      h("div", { class: "row" }, toggleBtn,
        h("label", null, t("live.metric")), metricSel, h("label", null, t("live.window")), winSel,
        h("button", { class: "btn", disabled: !live.buf.length, onclick: saveCsv }, t("live.save")),
        h("button", { class: "btn", onclick: savePng }, t("live.save_png"))),
      h("div", { class: "tiles" }, tile("nsv", t("live.dose")), tile("cpm", t("live.cpm")), tile("e", t("live.e")), tile("h", t("live.h")), tile("bat", t("live.battery"))),
      canvas, statsBox, alarmBox,
      h("p", { class: "hint" }, t("live.local_only") + (ctx.settings?.record_live ? " " + t("live.recorded") : "")),
    );
    chart = new Chart(canvas, { interactive: false, unit: unit() });
    if (!live.buf.length && tiles.nsv) tiles.nsv.textContent = live.running ? t("live.waiting") : "—";
    update();
  }

  async function saveCsv(): Promise<void> {
    if (!live.buf.length) { toast(t("live.nothing")); return; }
    const path = await save({ defaultPath: `yarrow-live-${new Date(live.buf[0].time * 1000).toISOString().slice(0, 10)}.csv`, filters: [{ name: "CSV", extensions: ["csv"] }] });
    if (!path) return;
    const rows = ["unix,utc,nsv_h,cpm,e_field_v_m,h_field_ut,battery_pct"];
    for (const r of live.buf) rows.push([r.time, new Date(r.time * 1000).toISOString(), r.nsv_h, r.cpm, local(r, "e_field"), local(r, "h_field"), r.battery ?? ""].map(csvEscape).join(","));
    if ((await guard(api.writeText(path, rows.join("\n") + "\n"))) !== undefined) toast(t("live.saved", { rows: fmtNumber(live.buf.length) }));
  }

  async function savePng(): Promise<void> {
    const canvas = el.querySelector("canvas.chart") as HTMLCanvasElement | null;
    if (!canvas || !live.buf.length) { toast(t("live.nothing")); return; }
    const path = await save({ defaultPath: "yarrow-live.png", filters: [{ name: "PNG", extensions: ["png"] }] });
    if (!path) return;
    const data = canvas.toDataURL("image/png").split(",")[1];
    if ((await guard(api.writeBase64(path, data))) !== undefined) toast(t("live.saved_png"));
  }

  live.subscribe(() => { if (el.isConnected && el.style.display !== "none") update(); });
  onConnectionChange(() => { if (el.isConnected && el.style.display !== "none") render(); });
  void doseScale;
  return { el, show: render };
}
