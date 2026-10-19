import { confirm, open } from "@tauri-apps/plugin-dialog";
import { api, type Drive, type FirmwareFile, type Release } from "../api";
import { ctx, guard, saveSettings, toast, type Page } from "../app";
import { clear, h } from "../dom";
import { fmtNumber, t } from "../i18n";

export function firmwarePage(): Page {
  const el = h("section");
  let folder = "";
  let files: FirmwareFile[] = [];
  let chosen = "";
  let releases: Release[] | null = null;
  let drives: Drive[] | null = null;

  async function scan(): Promise<void> {
    files = folder ? ((await guard(api.fwScan(folder))) ?? []) : [];
    if (!files.some((f) => f.path === chosen)) chosen = files[0]?.path ?? "";
    render();
  }

  async function init(): Promise<void> {
    folder = ctx.settings?.fw_folder || (await guard(api.fwDefaultFolder())) || "";
    await scan();
  }

  function folderCard(): HTMLElement {
    return h("div", { class: "card" },
      h("h2", null, t("fw.folder")),
      h("div", { class: "row" },
        h("code", null, folder || "—"),
        h("button", { class: "btn", onclick: async () => {
          const d = await open({ directory: true, defaultPath: folder || undefined });
          if (typeof d === "string") { folder = d; await saveSettings({ fw_folder: d }); await scan(); }
        } }, t("common.choose_folder")),
        h("button", { class: "btn", onclick: scan }, t("fw.scan"))),
      files.length === 0 ? h("p", { class: "muted" }, t("fw.none"))
        : files.map((f) => h("label", { class: "file" },
            h("input", { type: "radio", name: "fw", checked: f.path === chosen, onchange: () => { chosen = f.path; render(); } }),
            `${f.name} (${fmtNumber(f.size / 1024, 1)} KB)`)),
    );
  }

  function githubCard(): HTMLElement {
    return h("div", { class: "card" },
      h("h2", null, t("fw.github")),
      h("p", { class: "hint" }, t("fw.github_hint")),
      h("div", { class: "row" }, h("button", { class: "btn", onclick: async () => { releases = (await guard(api.fwReleases())) ?? null; render(); } }, t("fw.check"))),
      releases?.map((r) => h("div", null,
        h("strong", null, r.name || r.tag), h("span", { class: "muted" }, ` ${r.tag}`),
        r.assets.length === 0 ? h("p", { class: "muted" }, t("fw.no_assets"))
          : h("div", null, r.assets.map((a) => h("div", { class: "file" },
              `${a.name} (${fmtNumber(a.size / 1024, 1)} KB)`,
              h("button", { class: "btn", onclick: async () => {
                const f = await guard(api.fwDownload(a.url, folder));
                if (f) { toast(t("fw.downloaded", { name: f.name })); await scan(); }
              } }, t("fw.download"))))))),
    );
  }

  function stepsCard(): HTMLElement {
    const file = files.find((f) => f.path === chosen);
    return h("div", { class: "card" },
      h("h2", null, t("fw.steps")),
      h("ol", { class: "steps" }, [1, 2, 3, 4, 5].map((i) => h("li", null, t(`fw.step${i}`)))),
      h("p", { class: "hint" }, t("fw.stock_hint")),
      h("h2", null, t("fw.drives")),
      h("div", { class: "row" }, h("button", { class: "btn", onclick: async () => { drives = (await guard(api.fwDrives())) ?? []; render(); } }, t("fw.find_drive"))),
      drives === null ? null : drives.length === 0 ? h("p", { class: "muted" }, t("fw.no_drive"))
        : drives.map((d) => h("div", { class: "file" },
            h("span", null, `${d.path} ${d.label}`.trim(), d.likely ? h("span", { class: "ok" }, ` — ${t("fw.likely")}`) : null),
            h("button", { class: "btn primary", disabled: !file, onclick: async () => {
              if (!file) return;
              const vars = { file: file.name, drive: d.path };
              if (!(await confirm(t("fw.copy_confirm", vars)))) return;
              if ((await guard(api.fwCopy(file.path, d.path))) !== undefined) toast(t("fw.copied"));
            } }, t("fw.copy_to", vars(file, d))))),
    );
  }

  function vars(file: FirmwareFile | undefined, d: Drive): { file: string; drive: string } {
    return { file: file?.name ?? "—", drive: d.path };
  }

  function render(): void {
    clear(el);
    el.append(h("h1", null, t("fw.title")), h("div", { class: "cols" }, h("div", null, folderCard(), githubCard()), h("div", null, stepsCard())));
  }

  return { el, show: () => void init() };
}
