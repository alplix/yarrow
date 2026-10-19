type Dict = Record<string, string>;

const modules = import.meta.glob("./locales/*.json", { eager: true }) as Record<string, { default: Dict }>;
const dicts: Record<string, Dict> = {};
for (const [path, m] of Object.entries(modules)) dicts[path.replace("./locales/", "").replace(".json", "")] = m.default;

/** Every locale file in `locales/` is a language; its `_name` is how it calls itself. */
export const languages = Object.entries(dicts)
  .map(([code, d]) => ({ code, name: d._name ?? code }))
  .sort((a, b) => a.name.localeCompare(b.name));

let current = "en";

export function pickLanguage(setting: string): string {
  if (setting && dicts[setting]) return setting;
  const wanted = navigator.languages?.length ? navigator.languages : [navigator.language];
  for (const l of wanted) {
    const base = l.toLowerCase().split("-")[0];
    if (dicts[base]) return base;
  }
  return "en";
}

export function setLanguage(code: string): void {
  current = dicts[code] ? code : "en";
  document.documentElement.lang = current;
}

export function lang(): string {
  return current;
}

/** Looks a text up (falls back to English, then to the key) and fills `{name}` placeholders. */
export function t(key: string, vars: Record<string, string | number> = {}): string {
  const s = dicts[current]?.[key] ?? dicts.en?.[key] ?? key;
  return s.replace(/\{(\w+)\}/g, (_, k) => String(vars[k] ?? `{${k}}`));
}

export function fmtTime(unix: number): string {
  return new Intl.DateTimeFormat(current, { dateStyle: "medium", timeStyle: "medium" }).format(new Date(unix * 1000));
}

export function fmtNumber(n: number, digits = 0): string {
  return new Intl.NumberFormat(current, { maximumFractionDigits: digits, minimumFractionDigits: digits }).format(n);
}

export type DoseUnit = "auto" | "nsv" | "usv" | "urem";
let unitPref: DoseUnit = "auto";

export function setDoseUnit(u: string): void {
  unitPref = u === "nsv" || u === "usv" || u === "urem" ? u : "auto";
}

/** The number a dose rate in nSv/h has in the unit the user chose (1 nSv/h is 0.1 µrem/h), and the unit's name. */
export function doseScale(nsv: number): { k: number; unit: string } {
  if (unitPref === "nsv") return { k: 1, unit: "nSv/h" };
  if (unitPref === "usv") return { k: 0.001, unit: "µSv/h" };
  if (unitPref === "urem") return { k: 0.1, unit: "µrem/h" };
  return nsv >= 1000 ? { k: 0.001, unit: "µSv/h" } : { k: 1, unit: "nSv/h" };
}

/** Name of the unit used for charts and tables (a fixed unit: "auto" is nSv/h there). */
export function chartDose(): { k: number; unit: string } {
  return unitPref === "usv" ? { k: 0.001, unit: "µSv/h" } : unitPref === "urem" ? { k: 0.1, unit: "µrem/h" } : { k: 1, unit: "nSv/h" };
}

/** Dose rate in nSv/h written in the unit the user chose. */
export function fmtDose(nsv: number): string {
  const { k, unit } = doseScale(nsv);
  const v = nsv * k;
  const digits = unit === "nSv/h" ? 0 : v >= 100 ? 1 : v >= 10 ? 2 : 3;
  return `${fmtNumber(v, digits)} ${unit}`;
}

/** The dose in µSv written as µSv or mSv (µrem when the user chose that). */
export function fmtDoseTotal(usv: number): string {
  if (unitPref === "urem") return `${fmtNumber(usv * 100, usv * 100 >= 100 ? 0 : 1)} µrem`;
  return usv >= 1000 ? `${fmtNumber(usv / 1000, 2)} mSv` : `${fmtNumber(usv, usv >= 100 ? 1 : 3)} µSv`;
}
