import { save } from "@tauri-apps/plugin-dialog";
import { api, type Device, type Series } from "../api";
import { ctx, guard, saveSettings, toast, type Page } from "../app";
import { Chart, type Pt } from "../chart";
import { clear, h } from "../dom";
import { chartDose, fmtDoseTotal, fmtNumber, fmtTime, t } from "../i18n";
import { byDay, byHour, csvEscape, drawBars, histogram, summarize } from "../stats";

type Period = "24h" | "7d" | "30d" | "90d" | "all" | "custom";
const SPAN: Record<string, number> = { "24h": 86400, "7d": 7 * 86400, "30d": 30 * 86400, "90d": 90 * 86400 };

export function statisticsPage(): Page {
  const el = h("section");
  let devices: Device[] = [];
  let current = "";
  let period: Period = "7d";
  let customFrom = "";
  let customTo = "";
  let range: [number, number] | null = null;
  let chart: Chart | null = null;
  let series: Series | null = null;

  const dev = () => devices.find((d) => d.id === current);
  /** counts per minute -> the unit of the charts */
  const factor = () => (100000 / Math.max(1, dev()?.sens_x100 || 18000)) * chartDose().k;
  const toPts = (s: Series): Pt[] => s.points.map((p) => ({ t: p.t, avg: p.avg * factor(), min: p.min * factor(), max: p.max * factor() }));

  async function load(): Promise<void> {
    devices = (await guard(api.devices())) ?? [];
    if (!devices.some((d) => d.id === current)) {
      current = ctx.connected?.device.id ?? ctx.settings?.selected_device ?? devices[0]?.id ?? "";
      if (!devices.some((d) => d.id === current)) current = devices[0]?.id ?? "";
    }
    const data = current ? await guard(api.deviceData(current)) : undefined;
    range = data?.range ?? null;
    if (!range) { series = null; render(); return; }
    const [lo, hi] = windowOf(range);
    series = (await guard(api.series(current, lo, hi, 5000))) ?? null;
    render();
  }

  function windowOf(r: [number, number]): [number, number] {
    if (period === "all") return [r[0], r[1]];
    if (period === "custom") {
      const a = customFrom ? Math.floor(new Date(customFrom + "T00:00:00").getTime() / 1000) : r[0];
      const b = customTo ? Math.floor(new Date(customTo + "T23:59:59").getTime() / 1000) : r[1];
      return [Math.min(a, b), Math.max(a, b)];
    }
    return [Math.max(r[0], r[1] - SPAN[period]), r[1]];
  }

  async function refetch(from: number, to: number): Promise<void> {
    if (!chart || !current) return;
    const s = await guard(api.series(current, Math.floor(from), Math.ceil(to), Math.max(100, Math.round(chart.canvas.clientWidth / 2))));
    if (s) chart.setData(toPts(s));
  }

  function tile(v: string, l: string): HTMLElement {
    return h("div", { class: "tile" }, h("div", { class: "v" }, v), h("div", { class: "l" }, l));
  }

  function render(): void {
    clear(el);
    el.append(h("h1", null, t("stats.title")));
    const sel = h("select", { onchange: async (e: Event) => { current = (e.target as HTMLSelectElement).value; await saveSettings({ selected_device: current }); await load(); } },
      devices.map((d) => h("option", { value: d.id, selected: d.id === current }, `${d.name} · ${d.id.slice(0, 8)}`)));
    const seg = h("div", { class: "seg" },
      (["24h", "7d", "30d", "90d", "all", "custom"] as Period[]).map((p) =>
        h("button", { class: "btn" + (p === period ? " on" : ""), onclick: () => { period = p; void load(); } }, t(`stats.p.${p}`))));
    el.append(h("div", { class: "row" }, devices.length ? [h("label", null, t("log.device")), sel] : null, seg,
      period === "custom" ? [
        h("input", { type: "date", value: customFrom, onchange: (e: Event) => { customFrom = (e.target as HTMLInputElement).value; void load(); } }),
        "–",
        h("input", { type: "date", value: customTo, onchange: (e: Event) => { customTo = (e.target as HTMLInputElement).value; void load(); } }),
      ] : null));
    if (!series || !series.points.length || !range) {
      el.append(h("p", { class: "muted" }, t("stats.empty")));
      return;
    }
    const pts = toPts(series);
    const unit = chartDose().unit;
    const bucket = series.bucket;
    const s = summarize(pts)!;
    const hoursData = (pts.length * bucket) / 3600;
    const doseUsv = pts.reduce((a, p) => a + ((p.avg / chartDose().k) * bucket) / 3600 / 1000, 0);
    const f = (v: number) => `${fmtNumber(v, v >= 100 ? 0 : v >= 10 ? 1 : 2)} ${unit}`;
    el.append(h("div", { class: "tiles" },
      tile(f(s.avg), t("live.avg")), tile(f(s.min), t("live.min")), tile(f(s.max), t("live.max")),
      tile(fmtDoseTotal(doseUsv), t("stats.dose")), tile(`${fmtNumber(hoursData, 1)} h`, t("stats.coverage")), tile(fmtNumber(pts.length), t("stats.points"))));
    const avgNsv = s.avg / chartDose().k;
    el.append(h("p", { class: "hint" }, t(avgNsv < 50 ? "stats.below" : avgNsv <= 200 ? "stats.typical" : "stats.above"), " ", `${fmtTime(series.points[0].t)} – ${fmtTime(series.points[series.points.length - 1].t)}`));

    const canvas = h("canvas", { class: "chart" });
    el.append(h("div", { class: "row" }, h("button", { class: "btn", onclick: exportPng }, t("stats.png")), h("button", { class: "btn", onclick: exportDaily }, t("stats.csv_days"))), canvas,
      h("p", { class: "hint" }, t("log.zoom_hint")));
    const [lo, hi] = windowOf(range);
    chart = new Chart(canvas, { interactive: true, unit, onView: (a, b) => void refetch(a, b) });
    chart.setDomain(lo, hi);
    chart.setData(pts);

    const hist = histogram(pts, 24, bucket);
    const histCanvas = h("canvas", { class: "bars" });
    const hourCanvas = h("canvas", { class: "bars" });
    const hours = byHour(pts);
    el.append(h("div", { class: "cols", style: "margin-top:14px" },
      h("div", { class: "card" }, h("h2", null, t("stats.histogram")), h("p", { class: "hint" }, t("stats.histogram_hint")), histCanvas),
      h("div", { class: "card" }, h("h2", null, t("stats.by_hour")), h("p", { class: "hint" }, t("stats.by_hour_hint")), hourCanvas)));
    requestAnimationFrame(() => {
      if (hist) {
        const step = (hist.to - hist.from) / hist.hours.length;
        drawBars(histCanvas, hist.hours, hist.hours.map((_, i) => fmtNumber(hist.from + step * i, hist.to > 100 ? 0 : 1)), (v) => `${fmtNumber(v, 1)} h`, 4);
      }
      drawBars(hourCanvas, hours, hours.map((_, i) => String(i)), (v) => `${fmtNumber(v, v >= 100 ? 0 : 1)} ${unit}`, 3);
    });

    const days = byDay(pts.map((p) => ({ ...p })), bucket);
    const maxAvg = Math.max(...days.map((d) => d.avg), 1e-9);
    el.append(h("div", { class: "card" }, h("h2", null, t("stats.days")),
      h("div", { class: "scroll" }, h("table", null,
        h("tr", null, h("th", null, t("stats.day")), h("th", { class: "num" }, `${t("live.avg")} (${unit})`), h("th", { class: "num" }, t("live.min")), h("th", { class: "num" }, t("live.max")), h("th", { class: "num" }, t("stats.dose")), h("th", null, "")),
        [...days].reverse().map((d) => h("tr", null,
          h("td", null, d.day),
          h("td", { class: "num" }, fmtNumber(d.avg, d.avg >= 100 ? 0 : 1)),
          h("td", { class: "num" }, fmtNumber(d.min, d.min >= 100 ? 0 : 1)),
          h("td", { class: "num" }, fmtNumber(d.max, d.max >= 100 ? 0 : 1)),
          h("td", { class: "num" }, fmtDoseTotal(d.dose_usv)),
          h("td", null, h("span", { class: "daybar", style: `width:${Math.max(2, Math.round((d.avg / maxAvg) * 140))}px` }))))))));
  }

  async function exportPng(): Promise<void> {
    const canvas = el.querySelector("canvas.chart") as HTMLCanvasElement | null;
    if (!canvas) return;
    const path = await save({ defaultPath: "yarrow-statistics.png", filters: [{ name: "PNG", extensions: ["png"] }] });
    if (!path) return;
    if ((await guard(api.writeBase64(path, canvas.toDataURL("image/png").split(",")[1]))) !== undefined) toast(t("live.saved_png"));
  }

  async function exportDaily(): Promise<void> {
    if (!series) return;
    const path = await save({ defaultPath: "yarrow-days.csv", filters: [{ name: "CSV", extensions: ["csv"] }] });
    if (!path) return;
    const unit = chartDose().unit;
    const rows = [`day,avg_${unit},min_${unit},max_${unit},dose_usv`];
    for (const d of byDay(toPts(series), series.bucket)) rows.push([d.day, d.avg.toFixed(3), d.min.toFixed(3), d.max.toFixed(3), d.dose_usv.toFixed(5)].map(csvEscape).join(","));
    if ((await guard(api.writeText(path, rows.join("\n") + "\n"))) !== undefined) toast(t("live.saved", { rows: rows.length - 1 }));
  }

  return { el, show: () => void load() };
}
