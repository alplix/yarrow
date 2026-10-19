import type { UnlistenFn } from "@tauri-apps/api/event";
import { api, on, type Reading } from "./api";
import { ctx, guard, onConnectionChange, toast } from "./app";
import { fmtDose, t } from "./i18n";

const KEEP_S = 6 * 3600;            // a session is kept for charts and statistics for six hours

export type Level = 0 | 1 | 2;      // normal, caution, danger

export interface AlarmSpan { start: number; end: number; peak: number; level: Level }

type Listener = () => void;

/** The one live stream of the connected device, shared by every page: the readings of this session, the dose collected,
 *  the alarm periods, and the start/stop of the stream. The device is read as soon as it is connected (a setting). */
class LiveStore {
  buf: Reading[] = [];
  started = 0;
  doseUsv = 0;
  running = false;
  level: Level = 0;
  alarms: AlarmSpan[] = [];
  private listeners = new Set<Listener>();
  private unlisten: UnlistenFn[] = [];
  private audio: AudioContext | null = null;

  subscribe(cb: Listener): () => void {
    this.listeners.add(cb);
    return () => this.listeners.delete(cb);
  }

  private emit(): void {
    this.listeners.forEach((cb) => cb());
  }

  get last(): Reading | undefined {
    return this.buf[this.buf.length - 1];
  }

  reset(): void {
    this.buf = [];
    this.started = 0;
    this.doseUsv = 0;
    this.level = 0;
    this.alarms = [];
  }

  async init(): Promise<void> {
    this.unlisten.push(await on<Reading>("live", (r) => this.push(r)));
    this.unlisten.push(
      await on<string>("live-error", (m) => {
        this.running = false;
        toast(m, true);
        this.emit();
      }),
    );
    onConnectionChange(() => {
      this.running = false;
      this.reset();
      this.emit();
      if (ctx.connected && ctx.settings?.auto_live) void this.start();
    });
  }

  async start(): Promise<boolean> {
    if (this.running) return true;
    this.reset();
    if ((await guard(api.liveStart())) === undefined) return false;
    this.running = true;
    this.emit();
    return true;
  }

  async stop(): Promise<void> {
    if (!this.running) return;
    this.running = false;
    await guard(api.liveStop());
    this.emit();
  }

  /** Brings the stream back after something else (reading the log) stopped it. */
  async resume(): Promise<void> {
    this.running = false;
    if (ctx.connected) await this.start();
  }

  private levelOf(nsv: number): Level {
    const s = ctx.settings;
    if (!s) return 0;
    return nsv >= s.alarm_danger ? 2 : nsv >= s.alarm_caution ? 1 : 0;
  }

  private push(r: Reading): void {
    const prev = this.last;
    if (!prev) this.started = r.time;
    else if (r.time > prev.time && r.time - prev.time < 10) this.doseUsv += (r.nsv_h * (r.time - prev.time)) / 3600 / 1000;
    this.buf.push(r);
    while (this.buf.length && r.time - this.buf[0].time > KEEP_S) this.buf.shift();

    const lv = this.levelOf(r.nsv_h);
    const open = this.alarms[this.alarms.length - 1];
    if (lv > 0) {
      if (open && open.end === 0) {
        open.peak = Math.max(open.peak, r.nsv_h);
        open.level = Math.max(open.level, lv) as Level;
      } else {
        this.alarms.push({ start: r.time, end: 0, peak: r.nsv_h, level: lv });
      }
    } else if (open && open.end === 0) {
      open.end = r.time;
    }
    if (lv > this.level) {
      this.beep(lv);
      toast(t(lv === 2 ? "alarm.danger_now" : "alarm.caution_now", { value: fmtDose(r.nsv_h) }), lv === 2);
    }
    this.level = lv;
    this.emit();
  }

  private beep(lv: Level): void {
    if (!ctx.settings?.alarm_sound) return;
    try {
      this.audio ??= new AudioContext();
      const a = this.audio;
      for (let i = 0; i < (lv === 2 ? 3 : 1); i++) {
        const o = a.createOscillator();
        const g = a.createGain();
        o.frequency.value = lv === 2 ? 880 : 660;
        o.type = "square";
        g.gain.value = 0.06;
        o.connect(g).connect(a.destination);
        o.start(a.currentTime + i * 0.28);
        o.stop(a.currentTime + i * 0.28 + 0.18);
      }
    } catch {
      /* no sound is not an error */
    }
  }
}

export const live = new LiveStore();
