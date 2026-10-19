import { lang } from "./i18n";

export interface Pt {
  t: number;
  avg: number;
  min?: number;
  max?: number;
}

export interface ChartOptions {
  /** Called (debounced) when the user zooms or pans, with the visible time range. */
  onView?: (from: number, to: number) => void;
  /** Wheel zoom, drag to pan, double click to reset. */
  interactive: boolean;
  unit: string;
}

function css(name: string): string {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

/** Rounds a step up to 1, 2 or 5 times a power of ten. */
function niceStep(raw: number): number {
  const p = 10 ** Math.floor(Math.log10(raw));
  const m = raw / p;
  return (m <= 1 ? 1 : m <= 2 ? 2 : m <= 5 ? 5 : 10) * p;
}

/** A small time-series chart on a canvas: min/max band, mean line, zoom and pan. */
export class Chart {
  private ctx: CanvasRenderingContext2D;
  private pts: Pt[] = [];
  private domain: [number, number] = [0, 1];
  private view: [number, number] = [0, 1];
  private hover: number | null = null;
  private drag: { x: number; view: [number, number] } | null = null;
  private timer = 0;
  private w = 0;
  private h = 0;
  private readonly pad = { l: 56, r: 12, t: 10, b: 26 };

  constructor(readonly canvas: HTMLCanvasElement, public opt: ChartOptions) {
    this.ctx = canvas.getContext("2d")!;
    new ResizeObserver(() => this.resize()).observe(canvas);
    if (opt.interactive) this.bind();
    canvas.addEventListener("mousemove", (e) => {
      this.hover = e.offsetX;
      this.draw();
    });
    canvas.addEventListener("mouseleave", () => {
      this.hover = null;
      this.draw();
    });
    matchMedia("(prefers-color-scheme: dark)").addEventListener("change", () => this.draw());
  }

  private bind(): void {
    const c = this.canvas;
    c.addEventListener("wheel", (e) => {
      e.preventDefault();
      const [a, b] = this.view;
      const f = (e.offsetX - this.pad.l) / Math.max(1, this.w - this.pad.l - this.pad.r);
      const at = a + (b - a) * Math.min(1, Math.max(0, f));
      const k = e.deltaY < 0 ? 0.8 : 1.25;
      this.setView(at - (at - a) * k, at + (b - at) * k, true);
    }, { passive: false });
    c.addEventListener("mousedown", (e) => {
      this.drag = { x: e.offsetX, view: [...this.view] };
    });
    window.addEventListener("mouseup", () => {
      this.drag = null;
    });
    c.addEventListener("mousemove", (e) => {
      if (!this.drag) return;
      const span = this.drag.view[1] - this.drag.view[0];
      const dt = ((this.drag.x - e.offsetX) / Math.max(1, this.w - this.pad.l - this.pad.r)) * span;
      this.setView(this.drag.view[0] + dt, this.drag.view[1] + dt, true);
    });
    c.addEventListener("dblclick", () => this.setView(this.domain[0], this.domain[1], true));
  }

  /** The whole range the data may cover; the view is reset to it. */
  setDomain(from: number, to: number): void {
    this.domain = [from, Math.max(to, from + 1)];
    this.view = [...this.domain];
    this.draw();
  }

  getView(): [number, number] {
    return this.view;
  }

  setView(a: number, b: number, notify: boolean): void {
    const [d0, d1] = this.domain;
    const min = Math.min(60, d1 - d0);
    let span = Math.max(b - a, min);
    span = Math.min(span, d1 - d0);
    a = Math.min(Math.max(a, d0), d1 - span);
    this.view = [a, a + span];
    this.draw();
    if (notify && this.opt.onView) {
      clearTimeout(this.timer);
      this.timer = window.setTimeout(() => this.opt.onView!(this.view[0], this.view[1]), 90);
    }
  }

  setData(pts: Pt[]): void {
    this.pts = pts;
    this.draw();
  }

  private resize(): void {
    const dpr = window.devicePixelRatio || 1;
    const r = this.canvas.getBoundingClientRect();
    this.w = r.width;
    this.h = r.height;
    this.canvas.width = Math.round(r.width * dpr);
    this.canvas.height = Math.round(r.height * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.draw();
  }

  private fmtTick(t: number, span: number): string {
    const d = new Date(t * 1000);
    const o: Intl.DateTimeFormatOptions =
      span < 3 * 3600 ? { hour: "2-digit", minute: "2-digit", second: span < 600 ? "2-digit" : undefined }
      : span < 3 * 86400 ? { day: "numeric", month: "short", hour: "2-digit", minute: "2-digit" }
      : { day: "numeric", month: "short" };
    return new Intl.DateTimeFormat(lang(), o).format(d);
  }

  draw(): void {
    const g = this.ctx;
    const { l, r, t: top, b } = this.pad;
    const W = this.w, H = this.h;
    if (!W || !H) return;
    g.clearRect(0, 0, W, H);
    const [x0, x1] = this.view;
    const vis = this.pts.filter((p) => p.t >= x0 - (x1 - x0) * 0.02 && p.t <= x1 + (x1 - x0) * 0.02);
    let ymax = Math.max(...vis.map((p) => p.max ?? p.avg), 0);
    if (!isFinite(ymax) || ymax <= 0) ymax = 1;
    const step = niceStep(ymax / 4);
    const top_y = Math.ceil((ymax * 1.05) / step) * step;
    const X = (t: number) => l + ((t - x0) / (x1 - x0)) * (W - l - r);
    const Y = (v: number) => top + (1 - v / top_y) * (H - top - b);

    g.font = "11px system-ui, sans-serif";
    g.textBaseline = "middle";
    g.strokeStyle = css("--grid");
    g.fillStyle = css("--muted");
    g.lineWidth = 1;
    for (let v = 0; v <= top_y + step / 2; v += step) {
      const y = Math.round(Y(v)) + 0.5;
      g.beginPath();
      g.moveTo(l, y);
      g.lineTo(W - r, y);
      g.stroke();
      g.textAlign = "right";
      g.fillText(String(Math.round(v * 100) / 100), l - 6, y);
    }
    const span = x1 - x0;
    const tstep = niceStep(span / 6);
    const tz = new Date().getTimezoneOffset() * 60;
    g.textAlign = "center";
    g.textBaseline = "top";
    for (let t = Math.ceil((x0 - tz) / tstep) * tstep + tz; t <= x1; t += tstep) {
      const x = Math.round(X(t)) + 0.5;
      g.beginPath();
      g.moveTo(x, top);
      g.lineTo(x, H - b);
      g.stroke();
      g.fillText(this.fmtTick(t, span), x, H - b + 6);
    }
    g.save();
    g.beginPath();
    g.rect(l, top, W - l - r, H - top - b);
    g.clip();
    // Band between the smallest and the largest value of each bucket.
    if (vis.some((p) => p.min !== undefined)) {
      g.fillStyle = css("--band");
      g.beginPath();
      vis.forEach((p, i) => (i ? g.lineTo(X(p.t), Y(p.max ?? p.avg)) : g.moveTo(X(p.t), Y(p.max ?? p.avg))));
      [...vis].reverse().forEach((p) => g.lineTo(X(p.t), Y(p.min ?? p.avg)));
      g.closePath();
      g.fill();
    }
    g.strokeStyle = css("--accent");
    g.lineWidth = 1.6;
    g.beginPath();
    let prev: Pt | null = null;
    const gap = vis.length > 1 ? Math.min(...vis.slice(1).map((p, i) => p.t - vis[i].t)) * 4 : Infinity;
    for (const p of vis) {
      // A long hole in the data (device off) is not joined by a line.
      if (!prev || p.t - prev.t > gap) g.moveTo(X(p.t), Y(p.avg));
      else g.lineTo(X(p.t), Y(p.avg));
      prev = p;
    }
    g.stroke();
    g.restore();
    g.fillStyle = css("--muted");
    g.textAlign = "left";
    g.textBaseline = "top";
    g.fillText(this.opt.unit, l + 4, 0);

    if (this.hover !== null && this.hover >= l && this.hover <= W - r && vis.length) {
      const t = x0 + ((this.hover - l) / (W - l - r)) * span;
      const p = vis.reduce((a, c) => (Math.abs(c.t - t) < Math.abs(a.t - t) ? c : a));
      const x = X(p.t);
      g.strokeStyle = css("--muted");
      g.beginPath();
      g.moveTo(x, top);
      g.lineTo(x, H - b);
      g.stroke();
      g.fillStyle = css("--accent");
      g.fillRect(x - 3, Y(p.avg) - 3, 6, 6);
      const label = `${new Intl.DateTimeFormat(lang(), { dateStyle: "short", timeStyle: "medium" }).format(new Date(p.t * 1000))}  ${
        Math.round(p.avg * 100) / 100
      } ${this.opt.unit}`;
      g.font = "12px system-ui, sans-serif";
      const tw = g.measureText(label).width + 12;
      const bx = Math.min(Math.max(x - tw / 2, l), W - r - tw);
      g.fillStyle = css("--panel");
      g.strokeStyle = css("--border");
      g.fillRect(bx, top + 2, tw, 20);
      g.strokeRect(bx + 0.5, top + 2.5, tw - 1, 19);
      g.fillStyle = css("--text");
      g.textBaseline = "middle";
      g.fillText(label, bx + 6, top + 12);
    }
  }
}
