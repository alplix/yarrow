//! Commands that talk to the device and read its stored log.

use crate::state::{blocking, err, lock, now, App, Res, Session};
use serde::Serialize;
use std::sync::atomic::Ordering;
use std::sync::Arc;
use std::time::Duration;
use tauri::{AppHandle, Emitter, State};
use yarrow_core::fake::FakeDevice;
use yarrow_core::{device_hash, fallback_id, Calibration, Detector, DetectorInfo, Event, Info, PortEntry, SerialTransport, Tools, YarrowDetector};
use yarrow_store::{Device, SeriesPoint};

type Ap<'a> = State<'a, Arc<App>>;

/// The name of the built-in device without hardware.
pub const DEMO_PORT: &str = "demo";

#[tauri::command]
pub async fn ports() -> Res<Vec<PortEntry>> {
    blocking(|| {
        let mut v = yarrow_core::list_ports().map_err(err)?;
        v.extend(yarrow_core::list_counters());
        v.push(PortEntry {
            name: DEMO_PORT.into(),
            vid: None,
            pid: None,
            serial_number: None,
            product: Some("demo".into()),
            is_yarrow: false,
        });
        Ok(v)
    })
    .await
}

#[derive(Serialize)]
pub struct Connected {
    info: DetectorInfo,
    cal: Option<Calibration>,
    /// Whether the device offers clock, calibration, events and theme.
    tools: bool,
    device: Device,
    port: String,
    /// "device" when the id comes from the firmware, "port" when it is the USB serial port's, "demo" for the demo.
    id_source: &'static str,
}

#[tauri::command]
pub async fn connect(app: Ap<'_>, port: String) -> Res<Connected> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        *lock(&a.session) = None;
        let (mut det, usb): (Box<dyn Detector>, Option<PortEntry>) = if port == DEMO_PORT {
            let mut d = FakeDevice::demo();
            d.realtime = true;
            (Box::new(YarrowDetector::new(d)), None)
        } else if port.starts_with(yarrow_core::RAH_PREFIX) {
            let entry = yarrow_core::list_counters().into_iter().find(|p| p.name == port);
            (Box::new(yarrow_core::rah::open_counter(&port).map_err(err)?), entry)
        } else {
            let entry = yarrow_core::list_ports().map_err(err)?.into_iter().find(|p| p.name == port);
            (Box::new(YarrowDetector::new(SerialTransport::open(&port).map_err(err)?)), entry)
        };
        let info = det.info().map_err(err)?;
        let cal = match det.tools() {
            Some(t) => Some(t.cal().map_err(err)?),
            None => None,
        };
        let (raw_id, id_source) = match (&info.unique_id, usb) {
            (Some(u), _) => (u.clone(), "device"),
            (None, Some(p)) => (fallback_id(p.vid.unwrap_or(0), p.pid.unwrap_or(0), p.serial_number.as_deref()), "port"),
            (None, None) => (DEMO_PORT.to_string(), "demo"),
        };
        let device_id = device_hash(&raw_id);
        let device = Device {
            id: device_id.clone(),
            name: info.model.rsplit(' ').next().filter(|_| info.driver == "yarrow").unwrap_or(&info.model).to_string(),
            firmware: info.firmware.clone(),
            lang: info.lang.clone().unwrap_or_default(),
            last_seen: now(),
            sens_x100: info.sens_x100.unwrap_or(0),
            dead_us: info.dead_us.unwrap_or(0),
        };
        {
            let store = lock(&a.store);
            store.upsert_device(&device).map_err(err)?;
        }
        let device = lock(&a.store).device(&device_id).map_err(err)?.unwrap_or(device);
        {
            let mut s = lock(&a.settings);
            s.last_port = port.clone();
            s.selected_device = device_id.clone();
            let _ = s.save(&a.dir);
        }
        *lock(&a.session) = Some(Session { det, device_id });
        let tools = lock(&a.session).as_mut().is_some_and(|s| s.det.tools().is_some());
        Ok(Connected { info, cal, tools, device, port, id_source })
    })
    .await
}

#[tauri::command]
pub async fn disconnect(app: Ap<'_>) -> Res<()> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        if let Some(mut s) = lock(&a.session).take() {
            let _ = s.det.live_stop();
        }
        Ok(())
    })
    .await
}

fn stop_live(a: &App) {
    a.live_gen.fetch_add(1, Ordering::SeqCst);
}

/// Runs `f` on the connected device.
fn with_client<T>(a: &App, f: impl FnOnce(&mut Session) -> Res<T>) -> Res<T> {
    let mut g = lock(&a.session);
    let s = g.as_mut().ok_or_else(|| "no device connected".to_string())?;
    f(s)
}

#[tauri::command]
pub async fn live_start(app: Ap<'_>, handle: AppHandle) -> Res<()> {
    let a = app.inner().clone();
    blocking(move || {
        with_client(&a, |s| s.det.live_start().map_err(err))?;
        let gen = a.live_gen.fetch_add(1, Ordering::SeqCst) + 1;
        let device_id = lock(&a.session).as_ref().map(|s| s.device_id.clone()).unwrap_or_default();
        std::thread::spawn(move || {
            let mut minute: Option<(u32, f64, u32)> = None;
            while a.live_gen.load(Ordering::SeqCst) == gen {
                let r = {
                    let mut g = lock(&a.session);
                    match g.as_mut() {
                        Some(s) => s.det.next_live(Duration::from_millis(250)),
                        None => break,
                    }
                };
                match r {
                    Ok(Some(sample)) => {
                        record_minute(&a, &device_id, &mut minute, sample.time, sample.cpm);
                        let _ = handle.emit("live", sample);
                    }
                    Ok(None) => {}
                    Err(e) => {
                        let _ = handle.emit("live-error", e.to_string());
                        break;
                    }
                }
                // Give commands waiting for the device a chance to take the lock.
                std::thread::sleep(Duration::from_millis(5));
            }
        });
        Ok(())
    })
    .await
}

/// Keeps a per-minute record of what the live view shows: the mean counts per minute of every whole minute, stored as
/// the pulses of that minute. A minute with fewer than 30 readings (the stream started or stopped inside it) is skipped.
fn record_minute(a: &App, id: &str, acc: &mut Option<(u32, f64, u32)>, time: u32, cpm: f64) {
    if time < 1_700_000_000 || id.is_empty() {
        return;                                  // the device clock was never set: such times mean nothing
    }
    let minute = time / 60 * 60;
    if let Some((m, sum, n)) = acc.as_mut() {
        if *m == minute {
            *sum += cpm;
            *n += 1;
            return;
        }
    }
    if let Some((m, sum, n)) = acc.take() {
        if n >= 30 && lock(&a.settings).record_live {
            let s = yarrow_core::Sample { time: m, interval: 60, time_valid: true, value: (sum / n as f64).round().max(0.0) as u32 };
            let _ = lock(&a.store).insert_samples(id, &[s]);
        }
    }
    *acc = Some((minute, cpm, 1));
}

#[tauri::command]
pub async fn live_stop(app: Ap<'_>) -> Res<()> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        with_client(&a, |s| s.det.live_stop().map_err(err))
    })
    .await
}

#[derive(Serialize)]
pub struct LogSummary {
    decoded: usize,
    new: usize,
    first: u32,
    last: u32,
}

#[tauri::command]
pub async fn download_log(app: Ap<'_>, handle: AppHandle) -> Res<LogSummary> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        let (samples, id) = with_client(&a, |s| {
            let _ = s.det.live_stop();
            let samples = s
                .det
                .download_log(&mut |done, total| {
                    let _ = handle.emit("log-progress", (done, total));
                })
                .map_err(err)?;
            Ok((samples, s.device_id.clone()))
        })?;
        let new = lock(&a.store).insert_samples(&id, &samples).map_err(err)?;
        let valid = samples.iter().filter(|s| s.is_dated());
        Ok(LogSummary {
                        decoded: samples.len(),
            new,
            first: valid.clone().map(|s| s.time).min().unwrap_or(0),
            last: valid.map(|s| s.time + s.interval).max().unwrap_or(0),
        })
    })
    .await
}

#[tauri::command]
pub fn devices(app: Ap<'_>) -> Res<Vec<Device>> {
    lock(&app.store).devices().map_err(err)
}

#[tauri::command]
pub fn rename_device(app: Ap<'_>, id: String, name: String) -> Res<()> {
    lock(&app.store).rename_device(&id, name.trim()).map_err(err)
}

#[derive(Serialize)]
pub struct DeviceData {
    count: u64,
    range: Option<(u32, u32)>,
}

#[tauri::command]
pub fn device_data(app: Ap<'_>, id: String) -> Res<DeviceData> {
    let s = lock(&app.store);
    Ok(DeviceData { count: s.sample_count(&id).map_err(err)?, range: s.time_range(&id).map_err(err)? })
}

#[derive(Serialize)]
pub struct Series {
    bucket: u32,
    points: Vec<SeriesPoint>,
}

#[tauri::command]
pub fn series(app: Ap<'_>, id: String, from: u32, to: u32, max_points: u32) -> Res<Series> {
    let (bucket, points) = lock(&app.store).series(&id, from, to, max_points.clamp(10, 5000)).map_err(err)?;
    Ok(Series { bucket, points })
}

#[tauri::command]
pub async fn export_csv(app: Ap<'_>, id: String, path: String) -> Res<u64> {
    let a = app.inner().clone();
    blocking(move || {
        let f = std::fs::File::create(&path).map_err(err)?;
        let mut w = std::io::BufWriter::new(f);
        let n = lock(&a.store).export_csv(&id, &mut w).map_err(err)?;
        std::io::Write::flush(&mut w).map_err(err)?;
        Ok(n)
    })
    .await
}

/// Runs `f` on the device's extra tools, if its driver has any.
fn with_tools<T>(a: &App, f: impl FnOnce(&mut dyn Tools, &str) -> Res<T>) -> Res<T> {
    with_client(a, |s| {
        let id = s.device_id.clone();
        let t = s.det.tools().ok_or_else(|| "this device has no such function".to_string())?;
        f(t, &id)
    })
}

#[tauri::command]
pub async fn events(app: Ap<'_>) -> Res<Vec<Event>> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        let _ = with_client(&a, |s| s.det.live_stop().map_err(err));
        with_tools(&a, |t, _| t.events().map_err(err))
    })
    .await
}

#[tauri::command]
pub async fn get_cal(app: Ap<'_>) -> Res<Calibration> {
    let a = app.inner().clone();
    blocking(move || with_tools(&a, |t, _| t.cal().map_err(err))).await
}

#[tauri::command]
pub async fn set_cal(app: Ap<'_>, cal: Calibration) -> Res<Calibration> {
    let a = app.inner().clone();
    blocking(move || {
        let (now_cal, id) = with_tools(&a, |t, id| {
            t.set_cal(&cal).map_err(err)?;
            Ok((t.cal().map_err(err)?, id.to_string()))
        })?;
        // Keep the stored copy in step with the device.
        let store = lock(&a.store);
        if let Some(mut d) = store.device(&id).map_err(err)? {
            d.sens_x100 = now_cal.sens_x100;
            d.dead_us = now_cal.dead_us;
            store.upsert_device(&d).map_err(err)?;
        }
        Ok(now_cal)
    })
    .await
}

#[tauri::command]
pub async fn set_theme(app: Ap<'_>, theme: u32) -> Res<()> {
    let a = app.inner().clone();
    blocking(move || with_tools(&a, |t, _| t.set_theme(theme).map_err(err))).await
}

#[derive(Serialize)]
pub struct Clock {
    device: u32,
    computer: u32,
}

/// Sets the device clock from the computer's and reads it back.
#[tauri::command]
pub async fn set_clock(app: Ap<'_>) -> Res<Clock> {
    let a = app.inner().clone();
    blocking(move || {
        stop_live(&a);
        let _ = with_client(&a, |s| s.det.live_stop().map_err(err));
        with_tools(&a, |t, _| {
            let computer = now();
            t.set_time(computer).map_err(err)?;
            Ok(Clock { device: t.time().map_err(err)?, computer })
        })
    })
    .await
}

#[tauri::command]
pub async fn device_info(app: Ap<'_>) -> Res<Info> {
    let a = app.inner().clone();
    blocking(move || with_tools(&a, |t, _| t.raw_info().map_err(err))).await
}
