import { api } from "../api";
import { ctx, guard, saveSettings, toast, type Page } from "../app";
import { clear, h } from "../dom";
import { languages, setDoseUnit, t } from "../i18n";

export function settingsPage(applyAppearance: () => void): Page {
  const el = h("section");

  async function render(): Promise<void> {
    const s = ctx.settings!;
    const about = await guard(api.about());
    const langSel = h("select", { onchange: async (e: Event) => { await saveSettings({ language: (e.target as HTMLSelectElement).value }); applyAppearance(); } },
      h("option", { value: "", selected: s.language === "" }, t("settings.language_auto")),
      languages.map((l) => h("option", { value: l.code, selected: l.code === s.language }, l.name)));
    const themeSel = h("select", { onchange: async (e: Event) => { await saveSettings({ theme: (e.target as HTMLSelectElement).value }); applyAppearance(); } },
      ["system", "dark", "light"].map((v) => h("option", { value: v, selected: v === s.theme }, t(`settings.theme.${v}`))));
    const unitSel = h("select", { onchange: async (e: Event) => { const u = (e.target as HTMLSelectElement).value; await saveSettings({ unit: u }); setDoseUnit(u); applyAppearance(); } },
      ["auto", "nsv", "usv", "urem"].map((v) => h("option", { value: v, selected: v === s.unit }, t(`settings.unit.${v}`))));
    const caution = h("input", { type: "number", min: "1", step: "10", value: String(s.alarm_caution), style: "width:110px" }) as HTMLInputElement;
    const danger = h("input", { type: "number", min: "1", step: "100", value: String(s.alarm_danger), style: "width:110px" }) as HTMLInputElement;
    const sound = h("input", { type: "checkbox", checked: s.alarm_sound }) as HTMLInputElement;
    const autoLive = h("input", { type: "checkbox", checked: s.auto_live }) as HTMLInputElement;
    const record = h("input", { type: "checkbox", checked: s.record_live }) as HTMLInputElement;
    clear(el);
    el.append(
      h("h1", null, t("settings.title")),
      h("div", { class: "card" },
        h("div", { class: "row" }, h("label", null, t("settings.language")), langSel),
        h("div", { class: "row" }, h("label", null, t("settings.theme")), themeSel),
        h("div", { class: "row" }, h("label", null, t("settings.unit")), unitSel)),
      h("div", { class: "card" },
        h("h2", null, t("settings.alarms")),
        h("p", { class: "hint" }, t("settings.alarms_hint")),
        h("div", { class: "row" }, h("label", null, t("settings.caution")), caution, h("span", { class: "muted" }, "nSv/h")),
        h("div", { class: "row" }, h("label", null, t("settings.danger")), danger, h("span", { class: "muted" }, "nSv/h")),
        h("div", { class: "row" }, h("label", null, t("settings.sound")), sound)),
      h("div", { class: "card" },
        h("h2", null, t("settings.recording")),
        h("p", { class: "hint" }, t("settings.recording_hint")),
        h("div", { class: "row" }, h("label", null, t("settings.auto_live")), autoLive),
        h("div", { class: "row" }, h("label", null, t("settings.record")), record)),
      h("div", { class: "row" }, h("button", { class: "btn primary", onclick: async () => {
        const c = Math.max(1, Math.round(Number(caution.value) || 500));
        const d = Math.max(c + 1, Math.round(Number(danger.value) || 10000));
        await saveSettings({ alarm_caution: c, alarm_danger: d, alarm_sound: sound.checked, auto_live: autoLive.checked, record_live: record.checked });
        toast(t("settings.saved"));
        void render();
      } }, t("common.apply"))),
      h("div", { class: "card", style: "margin-top:14px" }, h("h2", null, t("settings.about")), h("p", null, t("settings.version", { version: about?.version ?? "" })),
        h("p", { class: "muted" }, about?.repo ?? ""), h("p", { class: "hint" }, t("settings.privacy"))),
    );
  }

  return { el, show: () => void render() };
}
