import { save } from "@tauri-apps/plugin-dialog";
import { api, on, type AlarmEvent, type Device, type DeviceData } from "../api";
import { ctx, guard, onConnectionChange, saveSettings, toast, type Page } from "../app";
import { Chart } from "../chart";
import { clear, h } from "../dom";
import { fmtNumber, fmtTime, t } from "../i18n";
import { live } from "../live-store";

export function logPage(): Page {
  const el = h("section");
  let devices: Device[] = [];
  let current = "";
  let data: DeviceData | null = null;
  let chart: Chart | null = null;
  let mode: "cpm" | "dose" = "cpm";
  let events: AlarmEvent[] | null = null;
  let progress: HTMLElement | null = null;
  let reading = false;

  const dev = () => devices.find((d) => d.id === current);

  async function load(): Promise<void> {
    devices = (await guard(api.devices())) ?? [];
    if (!devices.some((d) => d.id === current)) {
      current = ctx.connected?.device.id ?? ctx.settings?.selected_device ?? devices[0]?.id ?? "";
      if (!devices.some((d) => d.id === current)) current = devices[0]?.id ?? "";
    }
    data = current ? ((await guard(api.deviceData(current))) ?? null) : null;
    render();
    if (data?.range) await fetchView(data.range[0], data.range[1], true);
  }

  async function fetchView(from: number, to: number, reset = false): Promise<void> {
    if (!chart || !current) return;
    const s = await guard(api.series(current, Math.floor(from), Math.ceil(to), Math.max(100, Math.round(chart.canvas.clientWidth / 2))));
    if (!s) return;
    const k = mode === "dose" ? 100000 / Math.max(1, dev()?.sens_x100 ?? 18000) : 1;
    if (reset) chart.setDomain(from, to);
    chart.setData(s.points.map((p) => ({ t: p.t, avg: p.avg * k, min: p.min * k, max: p.max * k })));
  }

  async function readLog(): Promise<void> {
    reading = true;
    render();
    const r = await guard(api.downloadLog());
    reading = false;
    void live.resume();
    if (r) {
      toast(t("log.read_done", { decoded: r.decoded, new: r.new }));
      current = ctx.connected?.device.id ?? current;
    }
    await load();
  }

  function render(): void {
    clear(el);
    el.append(h("h1", null, t("log.title")));
    const sel = h(
      "select",
      { onchange: async (e: Event) => { current = (e.target as HTMLSelectElement).value; await saveSettings({ selected_device: current }); await load(); } },
      devices.map((d) => h("option", { value: d.id, selected: d.id === current }, `${d.name} · ${d.id.slice(0, 8)}`)),
    );
    progress = h("div", { class: "progress" }, h("div"));
    el.append(
      h("div", { class: "row" },
        devices.length ? [h("label", null, t("log.device")), sel] : null,
        h("button", { class: "btn primary", disabled: !ctx.connected?.tools || reading, onclick: readLog }, t("log.read")),
        h("button", { class: "btn", disabled: !current || !data?.count, onclick: exportCsv }, t("log.export"))),
      ...(reading ? [progress, h("p", { class: "progress-label hint" }, t("log.reading", { done: 0, total: "…" }))] : []),
    );
    if (!ctx.connected) el.append(h("p", { class: "hint" }, t("common.no_device")));
    if (!data?.range) {
      el.append(h("p", { class: "muted" }, t("log.empty")));
      return;
    }
    el.append(h("p", { class: "muted" }, t("log.range", { count: fmtNumber(data.count), from: fmtTime(data.range[0]), to: fmtTime(data.range[1]) })));
    const canvas = h("canvas", { class: "chart" });
    const modeSel = h(
      "select",
      { onchange: (e: Event) => { mode = (e.target as HTMLSelectElement).value as "cpm" | "dose"; render(); void load(); } },
      h("option", { value: "cpm", selected: mode === "cpm" }, t("log.unit.cpm")),
      h("option", { value: "dose", selected: mode === "dose" }, t("log.unit.dose")),
    );
    el.append(h("div", { class: "row" }, modeSel), canvas, h("p", { class: "hint" }, t("log.zoom_hint")));
    chart = new Chart(canvas, {
      interactive: true,
      unit: mode === "dose" ? "nSv/h" : "cpm",
      onView: (a, b) => void fetchView(a, b),
    });
    el.append(eventsCard());
  }

  function eventsCard(): HTMLElement {
    return h("div", { class: "card", style: "margin-top:14px" },
      h("h2", null, t("log.events")),
      h("div", { class: "row" },
        h("button", { class: "btn", disabled: !ctx.connected?.tools, onclick: async () => {
          events = (await guard(api.events())) ?? null;
          const keep = el.querySelector("div.card:last-child");
          keep?.replaceWith(eventsCard());
        } }, t("log.events_load"))),
      events === null ? null : events.length === 0 ? h("p", { class: "muted" }, t("log.events_none"))
        : h("table", null,
            h("tr", null, h("th", null, t("log.event.start")), h("th", { class: "num" }, t("log.event.duration")), h("th", { class: "num" }, t("log.event.peak")), h("th", { class: "num" }, t("log.event.level"))),
            events.map((e) => h("tr", null, h("td", null, fmtTime(e.start)), h("td", { class: "num" }, `${e.duration_s} s`), h("td", { class: "num" }, fmtNumber(e.peak_nsv_h)), h("td", { class: "num" }, String(e.level))))),
    );
  }

  async function exportCsv(): Promise<void> {
    const path = await save({ defaultPath: `yarrow-${dev()?.name ?? "log"}.csv`, filters: [{ name: "CSV", extensions: ["csv"] }] });
    if (!path) return;
    const rows = await guard(api.exportCsv(current, path));
    if (rows !== undefined) toast(t("log.exported", { rows: fmtNumber(rows) }));
  }

  void on<[number, number]>("log-progress", ([done, total]) => {
    const bar = progress?.firstElementChild as HTMLElement | null;
    if (bar) bar.style.width = `${(done / total) * 100}%`;
    const label = el.querySelector(".progress-label");
    if (label) label.textContent = t("log.reading", { done, total });
  });

  onConnectionChange(() => { if (el.isConnected && el.style.display !== "none") void load(); });
  return { el, show: () => void load() };
}
