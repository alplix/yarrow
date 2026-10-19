import { confirm } from "@tauri-apps/plugin-dialog";
import { api } from "../api";
import { ctx, guard, onConnectionChange, toast, type Page } from "../app";
import { clear, h } from "../dom";
import { fmtNumber, t } from "../i18n";
import { live } from "../live-store";
import { formatDuration } from "../stats";

/** Calibration wizard: the device is held next to a source (or left in a place) whose dose rate another instrument knows, the Hub
 *  averages what the device counts for a while, and the new sensitivity is the mean counts per minute over that dose rate. */
export function calibratePage(): Page {
  const el = h("section");
  let step: 1 | 2 | 3 = 1;
  let reference = 0.12;                 // µSv/h
  let from = 0;                         // index in live.buf where measuring began
  let startedAt = 0;
  let result: { sens: number; uncertainty: number; counts: number; minutes: number } | null = null;
  let timer = 0;

  const labelSteps = () =>
    h("div", { class: "steps" }, [1, 2, 3].map((n) => h("span", { class: n === step ? "on" : "" }, `${n}. ${t(`wiz.step${n}`)}`)));

  function measure(): { mean: number; minutes: number; counts: number } {
    const w = live.buf.slice(from);
    if (!w.length) return { mean: 0, minutes: 0, counts: 0 };
    const mean = w.reduce((a, r) => a + r.cpm, 0) / w.length;
    const minutes = (w[w.length - 1].time - w[0].time + 1) / 60;
    return { mean, minutes, counts: mean * minutes };
  }

  function render(): void {
    clear(el);
    window.clearInterval(timer);
    el.append(h("h1", null, t("wiz.title")), h("p", { class: "muted" }, t("wiz.intro")));
    if (!ctx.connected) {
      el.append(h("p", { class: "muted" }, t("common.no_device")));
      return;
    }
    if (!ctx.connected.tools) {
      el.append(h("p", { class: "muted" }, t("device.no_tools")));
      return;
    }
    el.append(labelSteps());
    const cal = ctx.connected.cal;
    if (step === 1) {
      const input = h("input", { type: "number", step: "0.01", min: "0.01", value: String(reference), style: "width:110px" }) as HTMLInputElement;
      el.append(h("div", { class: "card" },
        h("h2", null, t("wiz.ref_title")), h("p", null, t("wiz.ref_text")),
        h("div", { class: "row" }, h("label", null, t("wiz.reference")), input, h("span", { class: "muted" }, "µSv/h")),
        cal ? h("p", { class: "hint" }, t("wiz.current", { sens: fmtNumber(cal.sens_x100 / 100, 1) })) : null,
        h("div", { class: "row" }, h("button", { class: "btn primary", onclick: async () => {
          const v = Number(input.value);
          if (!(v > 0)) { toast(t("wiz.bad_ref"), true); return; }
          reference = v;
          if (!(await live.start())) return;
          from = live.buf.length;
          startedAt = Date.now();
          step = 2;
          render();
        } }, t("wiz.start")))));
    } else if (step === 2) {
      const status = h("div", { class: "tiles" });
      const info = h("p", { class: "hint" }, t("wiz.measure_hint"));
      const finish = h("button", { class: "btn primary", disabled: true, onclick: () => {
        const m = measure();
        result = { sens: (m.mean / reference) * 100, uncertainty: m.counts > 0 ? 1 / Math.sqrt(m.counts) : 1, counts: m.counts, minutes: m.minutes };
        step = 3;
        render();
      } }, t("wiz.finish"));
      const paint = () => {
        const m = measure();
        const u = m.counts > 0 ? 100 / Math.sqrt(m.counts) : 100;
        status.replaceChildren(
          h("div", { class: "tile" }, h("div", { class: "v" }, fmtNumber(m.mean, 1)), h("div", { class: "l" }, t("wiz.mean_cpm"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, formatDuration((Date.now() - startedAt) / 1000)), h("div", { class: "l" }, t("wiz.elapsed"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, fmtNumber(m.counts, 0)), h("div", { class: "l" }, t("wiz.counts"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, `± ${fmtNumber(u, 1)} %`), h("div", { class: "l" }, t("wiz.uncertainty"))),
        );
        finish.disabled = m.counts < 100;
      };
      paint();
      timer = window.setInterval(() => { if (el.isConnected) paint(); }, 1000);
      el.append(h("div", { class: "card" }, h("h2", null, t("wiz.measuring")), status, info,
        h("div", { class: "row" }, finish, h("button", { class: "btn", onclick: () => { step = 1; render(); } }, t("common.cancel")))));
    } else if (result) {
      const r = result;
      const cur = cal?.sens_x100 ?? 0;
      const change = cur ? ((r.sens - cur) / cur) * 100 : 0;
      el.append(h("div", { class: "card" }, h("h2", null, t("wiz.result")),
        h("div", { class: "tiles" },
          h("div", { class: "tile" }, h("div", { class: "v" }, fmtNumber(cur / 100, 1)), h("div", { class: "l" }, t("wiz.old_sens"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, fmtNumber(r.sens / 100, 1)), h("div", { class: "l" }, t("wiz.new_sens"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, `${change >= 0 ? "+" : ""}${fmtNumber(change, 1)} %`), h("div", { class: "l" }, t("wiz.change"))),
          h("div", { class: "tile" }, h("div", { class: "v" }, `± ${fmtNumber(r.uncertainty * 100, 1)} %`), h("div", { class: "l" }, t("wiz.uncertainty")))),
        h("p", { class: "hint" }, t("wiz.result_hint", { counts: fmtNumber(r.counts, 0), minutes: fmtNumber(r.minutes, 1) })),
        h("div", { class: "row" },
          h("button", { class: "btn primary", disabled: !cal, onclick: async () => {
            if (!cal) return;
            if (!(await confirm(t("cal.confirm")))) return;
            const next = { ...cal, sens_x100: Math.round(r.sens) };
            const back = await guard(api.setCal(next));
            if (back && ctx.connected) {
              ctx.connected.cal = back;
              toast(t("cal.written"));
              step = 1;
              render();
            }
          } }, t("wiz.apply")),
          h("button", { class: "btn", onclick: () => { step = 1; render(); } }, t("wiz.again")))));
    }
  }

  onConnectionChange(() => { if (el.isConnected && el.style.display !== "none") { step = 1; render(); } });
  return { el, show: render, hide: () => window.clearInterval(timer) };
}
