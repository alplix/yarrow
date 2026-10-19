import type { Pt } from "./chart";

export interface Summary { min: number; avg: number; max: number; n: number }

/** Lowest, mean and highest of a series (the min/max band when the points have one, else the mean line). */
export function summarize(points: Pt[]): Summary | null {
  if (!points.length) return null;
  let min = Infinity, max = -Infinity, sum = 0;
  for (const p of points) {
    min = Math.min(min, p.min ?? p.avg);
    max = Math.max(max, p.max ?? p.avg);
    sum += p.avg;
  }
  return { min, avg: sum / points.length, max, n: points.length };
}

/** Thins a long series to about `limit` points by averaging buckets, keeping each bucket's lowest and highest value. */
export function thin(points: Pt[], limit: number): Pt[] {
  if (points.length <= limit) return points;
  const size = Math.ceil(points.length / limit);
  const out: Pt[] = [];
  for (let i = 0; i < points.length; i += size) {
    const part = points.slice(i, i + size);
    let min = Infinity, max = -Infinity, sum = 0;
    for (const p of part) {
      min = Math.min(min, p.min ?? p.avg);
      max = Math.max(max, p.max ?? p.avg);
      sum += p.avg;
    }
    out.push({ t: part[Math.floor(part.length / 2)].t, avg: sum / part.length, min, max });
  }
  return out;
}

export interface DayRow { day: string; avg: number; min: number; max: number; dose_usv: number }

/** Groups a dose-rate series (nSv/h) by calendar day of the computer; `bucket` is the seconds each point stands for. */
export function byDay(points: Pt[], bucket: number): DayRow[] {
  const days = new Map<string, { sum: number; n: number; min: number; max: number; dose: number }>();
  for (const p of points) {
    const d = new Date(p.t * 1000);
    const key = `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`;
    const e = days.get(key) ?? { sum: 0, n: 0, min: Infinity, max: -Infinity, dose: 0 };
    e.sum += p.avg;
    e.n++;
    e.min = Math.min(e.min, p.min ?? p.avg);
    e.max = Math.max(e.max, p.max ?? p.avg);
    e.dose += (p.avg * bucket) / 3600 / 1000;      // nSv/h for `bucket` seconds, in µSv
    days.set(key, e);
  }
  return [...days.entries()]
    .sort((a, b) => (a[0] < b[0] ? -1 : 1))
    .map(([day, e]) => ({ day, avg: e.sum / e.n, min: e.min, max: e.max, dose_usv: e.dose }));
}

export function csvEscape(v: string | number): string {
  const s = String(v);
  return /[",\n]/.test(s) ? `"${s.replace(/"/g, '""')}"` : s;
}

export function formatDuration(seconds: number): string {
  const s = Math.max(0, Math.round(seconds));
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), r = s % 60;
  return h ? `${h} h ${String(m).padStart(2, "0")} min` : m ? `${m} min ${String(r).padStart(2, "0")} s` : `${r} s`;
}

/** Histogram of how long the dose rate was at each level: `bins` equal-width classes between the lowest and highest value,
 *  each point weighing `bucket` seconds. */
export function histogram(points: Pt[], bins: number, bucket: number): { from: number; to: number; hours: number[] } | null {
  if (!points.length) return null;
  let lo = Infinity, hi = -Infinity;
  for (const p of points) { lo = Math.min(lo, p.avg); hi = Math.max(hi, p.avg); }
  if (hi - lo < 1e-9) hi = lo + 1;
  const hours = new Array<number>(bins).fill(0);
  for (const p of points) {
    const i = Math.min(bins - 1, Math.floor(((p.avg - lo) / (hi - lo)) * bins));
    hours[i] += bucket / 3600;
  }
  return { from: lo, to: hi, hours };
}

/** Mean dose rate for each hour of the day (local time); null where nothing was recorded. */
export function byHour(points: Pt[]): (number | null)[] {
  const sum = new Array<number>(24).fill(0), n = new Array<number>(24).fill(0);
  for (const p of points) {
    const hr = new Date(p.t * 1000).getHours();
    sum[hr] += p.avg;
    n[hr]++;
  }
  return sum.map((s, i) => (n[i] ? s / n[i] : null));
}

function cssVar(name: string): string {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

/** A simple bar chart on a canvas: one bar per value, a label under some of them, the highest value written on top. */
export function drawBars(canvas: HTMLCanvasElement, values: (number | null)[], labels: string[], format: (v: number) => string, every = 1): void {
  const dpr = window.devicePixelRatio || 1;
  const w = canvas.clientWidth, hgt = canvas.clientHeight;
  canvas.width = Math.max(1, Math.floor(w * dpr));
  canvas.height = Math.max(1, Math.floor(hgt * dpr));
  const g = canvas.getContext("2d")!;
  g.setTransform(dpr, 0, 0, dpr, 0, 0);
  g.clearRect(0, 0, w, hgt);
  const padB = 22, padT = 18, padL = 6;
  const max = Math.max(1e-9, ...values.map((v) => v ?? 0));
  const bw = (w - padL * 2) / values.length;
  g.font = "11px system-ui, sans-serif";
  g.textAlign = "center";
  values.forEach((v, i) => {
    const x = padL + i * bw;
    if (v !== null) {
      const bh = ((hgt - padB - padT) * v) / max;
      g.fillStyle = cssVar("--accent") || "#5ca860";
      g.fillRect(x + 1, hgt - padB - bh, Math.max(1, bw - 2), bh);
    }
    if (i % every === 0) {
      g.fillStyle = cssVar("--muted") || "#888";
      g.fillText(labels[i] ?? "", x + bw / 2, hgt - 6);
    }
  });
  g.fillStyle = cssVar("--muted") || "#888";
  g.textAlign = "left";
  g.fillText(format(max), padL, 12);
}
