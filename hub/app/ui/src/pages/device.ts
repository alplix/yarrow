import { confirm } from "@tauri-apps/plugin-dialog";
import { api, type Calibration, type PortEntry } from "../api";
import { ctx, guard, onConnectionChange, setConnected, toast, type Page } from "../app";
import { clear, h } from "../dom";
import { fmtTime, t } from "../i18n";

export function devicePage(): Page {
  const el = h("section");
  let ports: PortEntry[] = [];
  let selected = "";
  let busy = false;
  let calDraft: Calibration | null = null;

  async function refreshPorts(): Promise<void> {
    ports = (await guard(api.ports())) ?? [];
    if (!ports.some((p) => p.name === selected)) {
      selected = (ports.find((p) => p.is_yarrow) ?? ports.find((p) => p.name === ctx.settings?.last_port) ?? ports[0])?.name ?? "";
    }
    render();
  }

  async function connect(): Promise<void> {
    busy = true;
    render();
    const c = await guard(api.connect(selected));
    busy = false;
    calDraft = c?.cal ?? null;
    setConnected(c ?? null);
  }

  function portLabel(p: PortEntry): string {
    if (p.name === "demo") return t("device.demo");
    if (p.name.startsWith("rah:")) return p.product ?? "Radioactive@Home counter";
    return p.is_yarrow ? t("device.yarrow_port", { name: p.name }) : p.name + (p.product ? ` — ${p.product}` : "");
  }

  function connectCard(): HTMLElement {
    const sel = h(
      "select",
      { onchange: (e: Event) => (selected = (e.target as HTMLSelectElement).value) },
      ports.map((p) => h("option", { value: p.name, selected: p.name === selected }, portLabel(p))),
    );
    const c = ctx.connected;
    return h(
      "div",
      { class: "card" },
      h("h2", null, t("device.port")),
      ports.length <= 1 ? h("p", { class: "hint" }, t("device.no_ports")) : null,
      h(
        "div",
        { class: "row" },
        sel,
        h("button", { class: "btn", onclick: refreshPorts }, t("common.refresh")),
        c
          ? h("button", { class: "btn", onclick: async () => { await guard(api.disconnect()); setConnected(null); } }, t("device.disconnect"))
          : h("button", { class: "btn primary", disabled: busy || !selected, onclick: connect }, busy ? t("device.connecting") : t("device.connect")),
      ),
    );
  }

  async function infoCard(): Promise<HTMLElement | null> {
    const c = ctx.connected;
    if (!c) return null;
    const raw = c.tools ? await guard(api.deviceInfo()) : undefined;
    const rows: [string, string][] = [
      [t("device.model"), c.info.model],
      [t("device.firmware"), c.info.firmware],
      [t("device.language"), c.info.lang ?? "—"],
    ];
    if (raw) {
      rows.push(
        [t("device.stage"), `${raw.stage} / 5`],
        [t("device.verified"), String(raw.verified)],
        [t("device.profile"), String(raw.profile)],
        [t("device.log_pages"), `${raw.pages} × ${raw.pagesize} B`],
        [t("device.clock"), fmtTime(raw.time)],
      );
    }
    rows.push([t("device.uid"), c.info.unique_id ?? t("device.uid_none")]);
    const name = h("input", { type: "text", value: c.device.name, maxLength: 40 });
    const themeSel = h("select", null, Array.from({ length: 21 }, (_, i) => h("option", { value: i, selected: raw?.theme === i }, String(i))));
    return h(
      "div",
      { class: "card" },
      h("h2", null, t("device.info")),
      h("table", null, rows.map(([k, v]) => h("tr", null, h("th", null, k), h("td", null, v)))),
      h("p", { class: "hint" }, t(`device.id_source.${c.id_source}`)),
      h("div", { class: "row" }, h("label", null, t("device.rename")), name,
        h("button", { class: "btn", onclick: async () => {
          await guard(api.renameDevice(c.device.id, name.value));
          c.device.name = name.value;
          ctx.listeners.forEach((f) => f());
        } }, t("common.apply"))),
      c.tools
        ? h(
            "div",
            null,
            h("div", { class: "row" }, h("button", { class: "btn", onclick: async () => {
              const r = await guard(api.setClock());
              if (r) { toast(t("device.clock_set", { time: fmtTime(r.device) })); render(); }
            } }, t("device.set_clock"))),
            h("div", { class: "row" }, h("label", null, t("device.theme")), themeSel,
              h("button", { class: "btn", onclick: async () => {
                if ((await guard(api.setTheme(Number(themeSel.value)))) !== undefined) toast(t("device.theme_set"));
              } }, t("common.apply"))),
          )
        : h("p", { class: "muted" }, t("device.no_tools")),
    );
  }

  function calCard(): HTMLElement | null {
    const c = ctx.connected;
    if (!c?.tools) return null;
    const d = calDraft ?? c.cal ?? { sens_x100: 0, dead_us: 0, e_pct: 100, h_pct: 100 };
    const field = (key: keyof Calibration, label: string, step = 1) => {
      const i = h("input", { type: "number", value: key === "sens_x100" ? d[key] / 100 : d[key], step: key === "sens_x100" ? 0.01 : step, min: 0 });
      i.addEventListener("input", () => {
        const v = Number(i.value);
        calDraft = { ...(calDraft ?? d), [key]: key === "sens_x100" ? Math.round(v * 100) : Math.round(v) };
      });
      return h("div", { class: "row" }, h("label", null, label), i);
    };
    return h(
      "div",
      { class: "card" },
      h("h2", null, t("cal.title")),
      field("sens_x100", t("cal.sens")),
      field("dead_us", t("cal.dead")),
      field("e_pct", t("cal.e")),
      field("h_pct", t("cal.h")),
      h("div", { class: "row" },
        h("button", { class: "btn", onclick: async () => {
          const r = await guard(api.getCal());
          if (r) { calDraft = r; render(); }
        } }, t("cal.read")),
        h("button", { class: "btn primary", onclick: async () => {
          if (!calDraft || !(await confirm(t("cal.confirm")))) return;
          const r = await guard(api.setCal(calDraft));
          if (r) { calDraft = r; toast(t("cal.written")); render(); }
        } }, t("cal.write"))),
    );
  }

  async function render(): Promise<void> {
    const parts = [connectCard(), await infoCard(), calCard()];
    clear(el);
    el.append(h("h1", null, t("device.title")), h("div", { class: "cols" }, h("div", null, parts[0], parts[1]), h("div", null, parts[2])));
  }

  onConnectionChange(() => { calDraft = ctx.connected?.cal ?? null; render(); });
  return { el, show: () => void refreshPorts() };
}
