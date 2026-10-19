import "./style.css";
import { api } from "./api";
import { ctx, onConnectionChange, setConnected, type Page } from "./app";
import { clear, FLOWER, FLOWER_PALETTE, h, pixelArt } from "./dom";
import { pickLanguage, setDoseUnit, setLanguage, t } from "./i18n";
import { live } from "./live-store";
import { calibratePage } from "./pages/calibrate";
import { dashboardPage } from "./pages/dashboard";
import { devicePage } from "./pages/device";
import { firmwarePage } from "./pages/firmware";
import { livePage } from "./pages/live";
import { logPage } from "./pages/log";
import { statisticsPage } from "./pages/statistics";
import { settingsPage } from "./pages/settings";

function applyTheme(): void {
  const pref = ctx.settings?.theme ?? "system";
  const dark = pref === "dark" || (pref === "system" && matchMedia("(prefers-color-scheme: dark)").matches);
  document.documentElement.dataset.theme = dark ? "dark" : "light";
}

const pages: Record<string, Page> = {};
let active = "dashboard";
let shell: HTMLElement;
let nav: HTMLElement;
const main = h("main");

function applyAppearance(): void {
  setLanguage(pickLanguage(ctx.settings?.language ?? ""));
  setDoseUnit(ctx.settings?.unit ?? "auto");
  applyTheme();
  buildNav();
  pages[active]?.show();
}

function buildNav(): void {
  clear(nav);
  const c = ctx.connected;
  nav.append(
    h("div", { class: "brand" }, pixelArt(FLOWER, FLOWER_PALETTE, 2), "Yarrow Hub"),
    ...(["dashboard", "live", "stats", "log", "calibrate", "device", "firmware", "settings"] as const).map((id) =>
      h("button", { class: id === active ? "active" : "", onclick: () => go(id) }, t(`nav.${id}`)),
    ),
    h("div", { class: "status" }, h("span", { class: "dot" + (c ? " on" : "") }), c ? t("status.connected", { name: c.device.name }) : t("status.none")),
  );
}

function go(id: string): void {
  pages[active]?.hide?.();
  active = id;
  for (const [k, p] of Object.entries(pages)) p.el.style.display = k === id ? "" : "none";
  buildNav();
  pages[id].show();
}

async function start(): Promise<void> {
  ctx.settings = await api.getSettings();
  setLanguage(pickLanguage(ctx.settings.language));
  setDoseUnit(ctx.settings.unit);
  applyTheme();
  matchMedia("(prefers-color-scheme: dark)").addEventListener("change", applyTheme);
  nav = h("nav");
  shell = document.getElementById("app")!;
  shell.append(nav, main);
  ctx.go = go;
  await live.init();
  Object.assign(pages, {
    dashboard: dashboardPage(go),
    device: devicePage(),
    live: livePage(),
    stats: statisticsPage(),
    calibrate: calibratePage(),
    log: logPage(),
    firmware: firmwarePage(),
    settings: settingsPage(applyAppearance),
  });
  for (const p of Object.values(pages)) {
    p.el.style.display = "none";
    main.append(p.el);
  }
  onConnectionChange(buildNav);
  go("dashboard");
  // Reconnect to the device used last time, if it is still there.
  const last = ctx.settings.last_port;
  if (last) {
    const ports = await api.ports().catch(() => []);
    if (ports.some((p) => p.name === last)) {
      const c = await api.connect(last).catch(() => null);
      if (c) setConnected(c);
    }
  }
}

void start();
