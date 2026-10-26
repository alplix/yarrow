import { invoke } from "@tauri-apps/api/core";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";

export interface PortEntry { name: string; vid: number | null; pid: number | null; serial_number: string | null; product: string | null; is_yarrow: boolean }
export interface DetectorInfo { driver: string; model: string; firmware: string; lang: string | null; unique_id: string | null; sens_x100: number | null; dead_us: number | null }
export interface Calibration { sens_x100: number; dead_us: number; e_pct: number; h_pct: number }
export interface Device { id: string; name: string; firmware: string; lang: string; last_seen: number; sens_x100: number; dead_us: number }
export interface Connected { info: DetectorInfo; cal: Calibration | null; tools: boolean; device: Device; port: string; id_source: "device" | "port" | "demo" }
export interface RawInfo { version: string; stage: number; verified: number; time: number; theme: number; profile: number; pages: number; pagesize: number; uid: string | null; lang: string | null; extra: Record<string, string> }
export interface LocalValue { name: string; unit: string; value: number }
export interface Reading { time: number; cpm: number; nsv_h: number; battery: number | null; local_only: LocalValue[] }
export interface LogSummary { decoded: number; new: number; first: number; last: number }
export interface SeriesPoint { t: number; min: number; avg: number; max: number }
export interface Series { bucket: number; points: SeriesPoint[] }
export interface DeviceData { count: number; range: [number, number] | null }
export interface AlarmEvent { start: number; duration_s: number; peak_nsv_h: number; level: number }
export interface Settings { language: string; theme: string; unit: string; alarm_caution: number; alarm_danger: number; alarm_sound: boolean; auto_live: boolean; record_live: boolean; fw_folder: string; last_port: string; selected_device: string }
export interface FirmwareFile { name: string; lang: string; size: number; path: string }
export interface Asset { name: string; lang: string; size: number; url: string }
export interface Release { tag: string; name: string; published: string; url: string; assets: Asset[] }
export interface Drive { path: string; label: string; size: number; likely: boolean }
export interface Clock { device: number; computer: number }

export const api = {
  ports: () => invoke<PortEntry[]>("ports"),
  connect: (port: string) => invoke<Connected>("connect", { port }),
  disconnect: () => invoke<void>("disconnect"),
  deviceInfo: () => invoke<RawInfo>("device_info"),
  health: () => invoke<Record<string, string>>("get_health"),
  liveStart: () => invoke<void>("live_start"),
  liveStop: () => invoke<void>("live_stop"),
  downloadLog: () => invoke<LogSummary>("download_log"),
  devices: () => invoke<Device[]>("devices"),
  renameDevice: (id: string, name: string) => invoke<void>("rename_device", { id, name }),
  deviceData: (id: string) => invoke<DeviceData>("device_data", { id }),
  series: (id: string, from: number, to: number, maxPoints: number) => invoke<Series>("series", { id, from, to, maxPoints }),
  exportCsv: (id: string, path: string) => invoke<number>("export_csv", { id, path }),
  events: () => invoke<AlarmEvent[]>("events"),
  getCal: () => invoke<Calibration>("get_cal"),
  setCal: (cal: Calibration) => invoke<Calibration>("set_cal", { cal }),
  setTheme: (theme: number) => invoke<void>("set_theme", { theme }),
  setClock: () => invoke<Clock>("set_clock"),
  fwScan: (dir: string) => invoke<FirmwareFile[]>("fw_scan", { dir }),
  fwDefaultFolder: () => invoke<string>("fw_default_folder"),
  fwReleases: () => invoke<Release[]>("fw_releases"),
  fwDownload: (url: string, dir: string) => invoke<FirmwareFile>("fw_download", { url, dir }),
  fwDrives: () => invoke<Drive[]>("fw_drives"),
  fwCopy: (file: string, drive: string) => invoke<string>("fw_copy", { file, drive }),
  getSettings: () => invoke<Settings>("get_settings"),
  saveSettings: (s: Settings) => invoke<Settings>("save_settings", { new: s }),
  writeText: (path: string, contents: string) => invoke<number>("write_text_file", { path, contents }),
  writeBase64: (path: string, data: string) => invoke<number>("write_base64_file", { path, data }),
  about: () => invoke<{ version: string; repo: string }>("about"),
};

export function on<T>(event: string, cb: (payload: T) => void): Promise<UnlistenFn> {
  return listen<T>(event, (e) => cb(e.payload));
}
