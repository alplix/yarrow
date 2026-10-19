import { api, type Connected, type Settings } from "./api";
import { h } from "./dom";

/** State shared by the pages. */
export const ctx: { connected: Connected | null; settings: Settings | null; listeners: (() => void)[]; go?: (id: string) => void } = {
  connected: null,
  settings: null,
  listeners: [],
};

export function onConnectionChange(cb: () => void): void {
  ctx.listeners.push(cb);
}

export function setConnected(c: Connected | null): void {
  ctx.connected = c;
  ctx.listeners.forEach((cb) => cb());
}

let toastEl: HTMLElement | null = null;
let toastTimer = 0;

export function toast(msg: string, isError = false): void {
  toastEl?.remove();
  toastEl = h("div", { class: "toast" + (isError ? " error" : "") }, msg);
  document.body.append(toastEl);
  clearTimeout(toastTimer);
  toastTimer = window.setTimeout(() => toastEl?.remove(), isError ? 9000 : 4000);
}

/** Runs an action and shows a failure as a toast. */
export async function guard<T>(p: Promise<T>): Promise<T | undefined> {
  try {
    return await p;
  } catch (e) {
    toast(String(e), true);
    return undefined;
  }
}

export async function saveSettings(patch: Partial<Settings>): Promise<void> {
  ctx.settings = await api.saveSettings({ ...ctx.settings!, ...patch });
}

/** Minimum a page has to provide. `show` is called each time the page is opened; `hide` when it is left. */
export interface Page {
  el: HTMLElement;
  show(): void;
  hide?(): void;
}
