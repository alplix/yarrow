//! Driver for the USB counter of the Radioactive@Home project (SBM-20 tube, V-USB or PIC firmware, a HID device).
//!
//! The device answers a HID feature report with its own millisecond timer and the cumulative count of raw tube pulses, both
//! 32 bit little endian after the report id:  `[0] id, [1..4] timer ms, [5..8] pulses` (confirmed on a real counter, revision
//! 2.51). It has no clock and no stored log, so the Hub reads it once a second and builds the counts per minute itself.
//! The tube's sensitivity is 171.232876 counts per minute for 1 uSv/h (the factor of the project's own program); the SBM-20
//! data sheet gives about 190 us of dead time. Same facts as `monitor/yarrow_src_rah.c`.

use crate::detector::{Detector, DetectorInfo, Reading};
use crate::{Error, PortEntry, Result, Sample};
use std::collections::VecDeque;
use std::time::{Duration, Instant};

pub const RAH_SENS_X100: u32 = 17123;
pub const RAH_DEAD_US: u32 = 190;
/// Name of a port entry made for a counter: `rah:` and the HID path.
pub const RAH_PREFIX: &str = "rah:";
/// The boards of the project: V-USB 16c0:05df (revisions 2.01 to 2.60, the kit) and the PIC board 04d8:f6fe (3.01).
const RAH_IDS: [(u16, u16); 2] = [(0x16c0, 0x05df), (0x04d8, 0xf6fe)];
const RAH_NAME: &str = "radioactiveathome";
const RAH_NAME_KIT: &str = "radiation detector kit";
const REPORT_LEN: usize = 129;
/// Shortest span of device time a rate is calculated from, and the longest window it is averaged over.
const MIN_SPAN_MS: u32 = 10_000;
const WINDOW_MS: u32 = 60_000;

/// Where the (timer ms, pulses) pairs come from: the real HID device, or a script in the tests.
pub trait RahSource: Send {
    fn read(&mut self) -> Result<(u32, u32)>;
}

/// The counter on the radio-active@home protocol, polled once a second.
pub struct RahDetector<S: RahSource> {
    src: S,
    revision: u16,
    now_unix: fn() -> u32,
    window: VecDeque<(u32, u32)>,
    last_poll: Option<Instant>,
}

fn unix_now() -> u32 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_secs() as u32).unwrap_or(0)
}

/// The revision the device reports (bcdDevice) as written on the project's boards: 0x0251 is "2.51".
pub fn revision_text(rev: u16) -> String {
    if rev == 0 {
        "rah".into()
    } else {
        format!("rah-{:x}.{:02x}", rev >> 8, rev & 0xff)
    }
}

/// Counts per minute to dose rate in nSv/h, with the dead time of the tube corrected.
pub fn nsv_from_cpm(cpm: f64) -> f64 {
    let dead = f64::from(RAH_DEAD_US) * 1e-6;
    let loss = (cpm / 60.0) * dead;
    let true_cpm = if loss < 0.5 { cpm / (1.0 - loss) } else { cpm * 2.0 };
    true_cpm * 100.0 / f64::from(RAH_SENS_X100) * 1000.0
}

impl<S: RahSource> RahDetector<S> {
    pub fn new(src: S, revision: u16) -> Self {
        RahDetector { src, revision, now_unix: unix_now, window: VecDeque::new(), last_poll: None }
    }

    #[cfg(test)]
    fn with_clock(mut self, f: fn() -> u32) -> Self {
        self.now_unix = f;
        self
    }

    /// The device must behave like the counter: a timer in milliseconds and a pulse count that never goes down.
    pub fn check(&mut self) -> Result<()> {
        let (t1, p1) = self.src.read()?;
        std::thread::sleep(Duration::from_millis(300));
        let (t2, p2) = self.src.read()?;
        let dt = t2.wrapping_sub(t1);
        if (200..=600).contains(&dt) && p2.wrapping_sub(p1) < 100_000 {
            Ok(())
        } else {
            Err(Error::Protocol("this device does not behave like a Radioactive@Home counter".into()))
        }
    }

    fn sample(&mut self) -> Result<Option<Reading>> {
        let (t, p) = self.src.read()?;
        if let Some(&(lt, lp)) = self.window.back() {
            // a restarted device starts both counters from zero again
            if t < lt || p < lp {
                self.window.clear();
            }
        }
        self.window.push_back((t, p));
        while let Some(&(ot, _)) = self.window.front() {
            if t.wrapping_sub(ot) > WINDOW_MS + 1500 && self.window.len() > 2 {
                self.window.pop_front();
            } else {
                break;
            }
        }
        let (ot, op) = *self.window.front().unwrap();
        let span = t - ot;
        if span < MIN_SPAN_MS {
            return Ok(None);
        }
        let cpm = f64::from(p - op) * 60_000.0 / f64::from(span);
        Ok(Some(Reading { time: (self.now_unix)(), cpm, nsv_h: nsv_from_cpm(cpm), battery: None, local_only: Vec::new() }))
    }
}

impl<S: RahSource> Detector for RahDetector<S> {
    fn info(&mut self) -> Result<DetectorInfo> {
        Ok(DetectorInfo {
            driver: "rah",
            model: "Radioactive@Home counter".into(),
            firmware: revision_text(self.revision),
            lang: None,
            unique_id: None,
            sens_x100: Some(RAH_SENS_X100),
            dead_us: Some(RAH_DEAD_US),
        })
    }

    fn live_start(&mut self) -> Result<()> {
        self.window.clear();
        self.last_poll = None;
        Ok(())
    }

    fn live_stop(&mut self) -> Result<()> {
        Ok(())
    }

    /// One reading a second: the rate over the last minute of the device's own time (shorter in the first minute, nothing in the
    /// first ten seconds).
    fn next_live(&mut self, timeout: Duration) -> Result<Option<Reading>> {
        let due = match self.last_poll {
            Some(t) => Duration::from_secs(1).saturating_sub(t.elapsed()),
            None => Duration::ZERO,
        };
        if due > timeout {
            std::thread::sleep(timeout);
            return Ok(None);
        }
        std::thread::sleep(due);
        self.last_poll = Some(Instant::now());
        self.sample()
    }

    fn download_log(&mut self, _progress: &mut dyn FnMut(u32, u32)) -> Result<Vec<Sample>> {
        Err(Error::Device("this counter has no stored log: the Hub records what it sees while it is connected".into()))
    }
}

// ---- the real device

#[cfg(any(windows, target_os = "linux", target_os = "macos"))]
mod hid {
    use super::*;
    use std::ffi::CString;

    fn is_rah(i: &hidapi::DeviceInfo) -> bool {
        if !RAH_IDS.contains(&(i.vendor_id(), i.product_id())) {
            return false;
        }
        // the V-USB and Microchip ids are shared with other devices: only accepted with the project's name in the product string
        let name = i.product_string().unwrap_or("").to_lowercase();
        name.contains(RAH_NAME) || name.contains(RAH_NAME_KIT)
    }

    /// Counters plugged in right now, as port entries named `rah:<hid path>`.
    pub fn list() -> Vec<PortEntry> {
        let Ok(api) = hidapi::HidApi::new() else { return Vec::new() };
        let mut v = Vec::new();
        for i in api.device_list().filter(|i| is_rah(i)) {
            let rev = revision_text(i.release_number());
            v.push(PortEntry {
                name: format!("{}{}", RAH_PREFIX, i.path().to_string_lossy()),
                vid: Some(i.vendor_id()),
                pid: Some(i.product_id()),
                serial_number: i.serial_number().filter(|s| !s.is_empty()).map(str::to_string),
                product: Some(format!("{} ({})", i.product_string().unwrap_or("Radioactive@Home counter"), rev)),
                is_yarrow: false,
            });
        }
        v
    }

    pub struct HidSource {
        dev: hidapi::HidDevice,
    }

    impl RahSource for HidSource {
        fn read(&mut self) -> Result<(u32, u32)> {
            let mut buf = [0u8; REPORT_LEN];
            let n = self.dev.get_feature_report(&mut buf).map_err(|e| Error::Protocol(format!("HID: {e}")))?;
            if n < 9 {
                return Err(Error::Protocol(format!("short HID report ({n} bytes)")));
            }
            let le = |o: usize| u32::from_le_bytes([buf[o], buf[o + 1], buf[o + 2], buf[o + 3]]);
            Ok((le(1), le(5)))
        }
    }

    /// Opens the counter whose port entry name is `rah:<path>` and checks that it behaves like one.
    pub fn open(name: &str) -> Result<RahDetector<HidSource>> {
        let path = name.strip_prefix(RAH_PREFIX).ok_or_else(|| Error::Protocol("not a Radioactive@Home entry".into()))?;
        let api = hidapi::HidApi::new().map_err(|e| Error::Protocol(format!("HID: {e}")))?;
        let revision = api
            .device_list()
            .find(|i| i.path().to_string_lossy() == path)
            .map(|i| i.release_number())
            .unwrap_or(0);
        let cpath = CString::new(path).map_err(|_| Error::Protocol("bad HID path".into()))?;
        let dev = api.open_path(&cpath).map_err(|e| Error::Protocol(format!("cannot open the counter: {e}")))?;
        let mut d = RahDetector::new(HidSource { dev }, revision);
        d.check()?;
        Ok(d)
    }
}

#[cfg(any(windows, target_os = "linux", target_os = "macos"))]
pub use hid::{list as list_counters, open as open_counter, HidSource};

#[cfg(not(any(windows, target_os = "linux", target_os = "macos")))]
pub fn list_counters() -> Vec<PortEntry> {
    Vec::new()
}

#[cfg(test)]
mod tests {
    use super::*;

    /// 1 pulse every 2.7 s on average is 22 cpm, the background of the real counter; the script gives exact numbers.
    struct Script {
        timer: u32,
        pulses: u32,
        step_ms: u32,
        every: u32,
        n: u32,
    }
    impl RahSource for Script {
        fn read(&mut self) -> Result<(u32, u32)> {
            self.timer += self.step_ms;
            self.n += 1;
            if self.n % self.every == 0 {
                self.pulses += 1;
            }
            Ok((self.timer, self.pulses))
        }
    }

    fn det(step_ms: u32, every: u32) -> RahDetector<Script> {
        RahDetector::new(Script { timer: 538_700_000, pulses: 6_094_000, step_ms, every, n: 0 }, 0x0251).with_clock(|| 1_790_000_000)
    }

    #[test]
    fn revision_is_written_like_the_boards() {
        assert_eq!(revision_text(0x0251), "rah-2.51");
        assert_eq!(revision_text(0), "rah");
    }

    #[test]
    fn dose_rate_matches_the_real_counter() {
        // 22 cpm is the volunteer's background: 0.129 uSv/h
        let n = nsv_from_cpm(22.0);
        assert!((n - 128.5).abs() < 0.5, "{n}");
        // the dead time matters only at high rates: 6000 cpm lose 1 % of the counts
        assert!(nsv_from_cpm(6000.0) > 6000.0 * 100.0 / 17123.0 * 1000.0 * 1.009);
    }

    #[test]
    fn nothing_for_ten_seconds_then_the_rate_of_the_window() {
        let mut d = det(1000, 3);                   // one reading a second, a pulse every third: 20 cpm
        let mut first = None;
        for i in 0..80 {
            let r = d.sample().unwrap();
            if i < 10 {
                assert!(r.is_none(), "reading {i} comes too early");
            } else if first.is_none() {
                first = r;
            }
        }
        assert!((first.unwrap().cpm - 20.0).abs() < 4.0);   // three pulses in ten seconds are 18 cpm
        let r = d.sample().unwrap().unwrap();
        assert!((r.cpm - 20.0).abs() < 1.0, "{}", r.cpm);
        assert_eq!(r.time, 1_790_000_000);
        assert!((r.nsv_h - nsv_from_cpm(r.cpm)).abs() < 1e-9);
    }

    #[test]
    fn the_rate_follows_the_device_timer_not_the_host() {
        // the real timer runs 0.13 % slow: 1.5 s of device time every 1.5 reads is still the right cpm per device second
        let mut d = det(998, 2);                    // a pulse every 1996 ms: 30.06 cpm
        let mut last = None;
        for _ in 0..120 {
            last = d.sample().unwrap();
        }
        assert!((last.unwrap().cpm - 30.06).abs() < 0.6);
    }

    #[test]
    fn a_restarted_device_starts_a_new_window() {
        let mut d = det(1000, 2);
        for _ in 0..30 {
            d.sample().unwrap();
        }
        d.src.timer = 0;
        d.src.pulses = 0;
        assert!(d.sample().unwrap().is_none());
        assert_eq!(d.window.len(), 1);
    }

    #[test]
    fn the_check_accepts_a_counter_and_refuses_a_frozen_timer() {
        let mut ok = det(300, 7);
        assert!(ok.check().is_ok());
        let mut frozen = det(0, 7);
        assert!(frozen.check().is_err());
    }

    #[test]
    fn it_has_no_log() {
        let mut d = det(1000, 3);
        assert!(d.download_log(&mut |_, _| {}).is_err());
        assert_eq!(d.info().unwrap().sens_x100, Some(17123));
        assert!(d.tools().is_none());
    }
}
