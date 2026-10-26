//! Driver for the GQ GMC counters (GMC-280, 300, 320, 500 and relatives) on their USB serial port. Same facts as
//! `monitor/yarrow_src_gmc.c`.
//!
//! The protocol is binary and has no line ending: `<GETVER>>` answers 14 bytes of text ("GMC-300Re 4.54"), `<GETSERIAL>>` 7 bytes,
//! `<GETCFG>>` the 256 (GMC-500: 512) bytes of the configuration, and `<HEARTBEAT1>>` starts a stream of one 16 bit big endian
//! count per second (the reserved bits 14 and 15 are never set). The counter's own calibration (counts per minute for a dose rate)
//! sits in the configuration and is read from it. The counter has no log the Hub reads; it records what it sees while connected.

use crate::detector::{Detector, DetectorInfo, Reading};
use crate::{Error, Result, Sample};
use std::collections::VecDeque;
use std::time::{Duration, Instant};

/// Dead time of the M4011 tube of these counters, from its data sheet.
pub const GMC_DEAD_US: u32 = 90;
/// The ports a GQ counter shows up on: the CH340 bridge of the 300 and 320 series, or the counter's own ids.
pub const GMC_BAUDS: [u32; 2] = [115_200, 57_600];
/// Seconds of counts a rate is averaged over, and the least before one is shown.
const WINDOW_S: usize = 60;
const MIN_S: usize = 10;

/// Bytes in, bytes out: the real serial port, or a script in the tests.
pub trait GmcIo: Send {
    fn write(&mut self, data: &[u8]) -> Result<()>;
    /// Whatever arrives within `timeout`, at most `buf.len()` bytes; 0 when nothing came.
    fn read(&mut self, buf: &mut [u8], timeout: Duration) -> Result<usize>;
    /// Throws away what is waiting.
    fn drain(&mut self);
}

fn read_exact(io: &mut dyn GmcIo, n: usize, timeout: Duration) -> Result<Vec<u8>> {
    let end = Instant::now() + timeout;
    let mut out = vec![0u8; n];
    let mut got = 0;
    while got < n {
        let left = end.saturating_duration_since(Instant::now());
        if left.is_zero() {
            break;
        }
        got += io.read(&mut out[got..], left)?;
    }
    out.truncate(got);
    Ok(out)
}

fn be_float(b: &[u8]) -> f64 {
    f64::from(f32::from_be_bytes([b[0], b[1], b[2], b[3]]))
}

fn le_float(b: &[u8]) -> f64 {
    f64::from(f32::from_le_bytes([b[0], b[1], b[2], b[3]]))
}

/// The device's own calibration: counts per minute for 1 uSv/h, times 100; `None` when the page holds nothing usable. The counts
/// are big endian everywhere; the dose rate next to them is a float whose byte order depends on the model (a real GMC-300Re 4.54
/// stores it little endian: 60 cpm = 0.39 uSv/h is `00 3c 14 ae c7 3e`). Both readings are tried; only one gives a ratio a Geiger
/// tube can have.
pub fn sensitivity_x100(cfg: &[u8]) -> Option<u32> {
    for o in [0x08usize, 0x0e, 0x14] {
        if o + 6 > cfg.len() {
            break;
        }
        let cpm = f64::from(u16::from_be_bytes([cfg[o], cfg[o + 1]]));
        for sv in [be_float(&cfg[o + 2..]), le_float(&cfg[o + 2..])] {
            if cpm > 0.0 && sv > 0.0 && sv < 1e6 && (10.0..=2000.0).contains(&(cpm / sv)) {
                return Some((cpm / sv * 100.0 + 0.5).floor() as u32);
            }
        }
    }
    None
}

/// Counts per minute to dose rate in nSv/h, with the dead time of the tube corrected.
pub fn nsv_from_cpm(cpm: f64, sens_x100: u32, dead_us: u32) -> f64 {
    let loss = (cpm / 60.0) * f64::from(dead_us) * 1e-6;
    let true_cpm = if loss < 0.5 { cpm / (1.0 - loss) } else { cpm * 2.0 };
    true_cpm * 100.0 / f64::from(sens_x100.max(1)) * 1000.0
}

fn unix_now() -> u32 {
    std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_secs() as u32).unwrap_or(0)
}

/// A GQ counter that has been recognised and is streaming (once `live_start` was called).
pub struct GmcDetector<I: GmcIo> {
    io: I,
    version: String,
    uid: Option<String>,
    sens_x100: u32,
    window: VecDeque<u32>,
    pending: Vec<u8>,
    now_unix: fn() -> u32,
}

impl<I: GmcIo> GmcDetector<I> {
    /// Asks the port who it is; an error when it is not a GQ counter or has no usable calibration.
    pub fn open(mut io: I) -> Result<Self> {
        io.drain();
        io.write(b"<GETVER>>")?;
        let ver = read_exact(&mut io, 14, Duration::from_millis(800))?;
        if ver.len() != 14 {
            return Err(Error::Protocol(format!("a serial port answered <GETVER>> with {} bytes instead of 14", ver.len())));
        }
        if !ver.starts_with(b"GMC-") || ver.iter().any(|&b| !(0x20..=0x7e).contains(&b)) {
            return Err(Error::Protocol("this port does not answer like a GQ GMC counter".into()));
        }
        let version = String::from_utf8_lossy(&ver).trim().to_string();
        io.drain();
        io.write(b"<GETSERIAL>>")?;
        let ser = read_exact(&mut io, 7, Duration::from_millis(800))?;
        io.drain();
        let want = if version.starts_with("GMC-500") { 512 } else { 256 };
        io.write(b"<GETCFG>>")?;
        let cfg = read_exact(&mut io, want, Duration::from_millis(1500))?;
        if cfg.len() != want {
            return Err(Error::Protocol(format!("<GETCFG>> returned {} of {} bytes", cfg.len(), want)));
        }
        let sens_x100 = sensitivity_x100(&cfg).ok_or_else(|| Error::Protocol("the counter's configuration has no usable calibration point".into()))?;
        io.drain();
        let model: String = version.chars().take(7).collect();
        let blank = ser.len() != 7 || ser.iter().all(|&b| b == 0 || b == 0xff);
        let uid = (!blank).then(|| format!("gmc|{}|{}", model, ser.iter().map(|b| format!("{b:02x}")).collect::<String>()));
        Ok(GmcDetector { io, version, uid, sens_x100, window: VecDeque::new(), pending: Vec::new(), now_unix: unix_now })
    }

    #[cfg(test)]
    fn with_clock(mut self, f: fn() -> u32) -> Self {
        self.now_unix = f;
        self
    }
}

impl<I: GmcIo> Detector for GmcDetector<I> {
    fn info(&mut self) -> Result<DetectorInfo> {
        Ok(DetectorInfo {
            driver: "gmc",
            model: format!("GQ {}", self.version.split(' ').next().unwrap_or("GMC")),
            firmware: self.version.clone(),
            lang: None,
            unique_id: self.uid.clone(),
            sens_x100: Some(self.sens_x100),
            dead_us: Some(GMC_DEAD_US),
        })
    }

    fn live_start(&mut self) -> Result<()> {
        self.window.clear();
        self.pending.clear();
        self.io.drain();
        self.io.write(b"<HEARTBEAT1>>")
    }

    fn live_stop(&mut self) -> Result<()> {
        self.io.write(b"<HEARTBEAT0>>")
    }

    /// One reading a second: the rate over the last minute of counts (shorter in the first minute, nothing in the first ten seconds).
    fn next_live(&mut self, timeout: Duration) -> Result<Option<Reading>> {
        let end = Instant::now() + timeout;
        loop {
            if self.pending.len() >= 2 {
                let v = u16::from_be_bytes([self.pending[0], self.pending[1]]);
                if v & 0xc000 != 0 {
                    self.pending.remove(0); // the reserved bits are never set: a stray byte came first, find the framing again
                    self.window.clear();
                    continue;
                }
                self.pending.drain(..2);
                self.window.push_back(u32::from(v));
                while self.window.len() > WINDOW_S {
                    self.window.pop_front();
                }
                if self.window.len() < MIN_S {
                    return Ok(None);
                }
                let cpm = f64::from(self.window.iter().sum::<u32>()) * 60.0 / self.window.len() as f64;
                return Ok(Some(Reading {
                    time: (self.now_unix)(),
                    cpm,
                    nsv_h: nsv_from_cpm(cpm, self.sens_x100, GMC_DEAD_US),
                    battery: None,
                    local_only: Vec::new(),
                }));
            }
            let left = end.saturating_duration_since(Instant::now());
            if left.is_zero() {
                return Ok(None);
            }
            let mut b = [0u8; 16];
            let n = self.io.read(&mut b, left)?;
            self.pending.extend_from_slice(&b[..n]);
        }
    }

    fn download_log(&mut self, _progress: &mut dyn FnMut(u32, u32)) -> Result<Vec<Sample>> {
        Err(Error::Device("the Hub does not read the stored history of this counter: it records what it sees while it is connected".into()))
    }
}

// ---- the real serial port

/// The serial port of a counter at one of the baud rates these counters use.
pub struct GmcPort {
    port: Box<dyn serialport::SerialPort>,
}

impl GmcIo for GmcPort {
    fn write(&mut self, data: &[u8]) -> Result<()> {
        use std::io::Write;
        self.port.write_all(data)?;
        self.port.flush()?;
        Ok(())
    }

    fn read(&mut self, buf: &mut [u8], timeout: Duration) -> Result<usize> {
        use std::io::Read;
        self.port.set_timeout(timeout.max(Duration::from_millis(1)))?;
        match self.port.read(buf) {
            Ok(n) => Ok(n),
            Err(e) if e.kind() == std::io::ErrorKind::TimedOut => Ok(0),
            Err(e) => Err(Error::Io(e)),
        }
    }

    fn drain(&mut self) {
        let _ = self.port.clear(serialport::ClearBuffer::Input);
    }
}

/// Opens `name` at each baud rate in turn and returns the counter that answers, or an error when none does.
pub fn open_counter(name: &str) -> Result<GmcDetector<GmcPort>> {
    let mut last = Error::Protocol("no GQ counter answered".into());
    for baud in GMC_BAUDS {
        let port = match serialport::new(name, baud).timeout(Duration::from_millis(100)).open() {
            Ok(p) => p,
            Err(e) => {
                last = Error::Port(e);
                continue;
            }
        };
        match GmcDetector::open(GmcPort { port }) {
            Ok(d) => return Ok(d),
            Err(e) => last = e,
        }
    }
    Err(last)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// A counter on a script: answers the commands with fixed bytes and streams one count per `read` after <HEARTBEAT1>>.
    struct Script {
        out: VecDeque<u8>,
        cfg: Vec<u8>,
        beating: bool,
        counts: Vec<u16>,
        next: usize,
        serial: [u8; 7],
        junk_first: bool,
    }

    impl Script {
        fn new(counts: Vec<u16>) -> Self {
            let mut cfg = vec![0u8; 256];
            cfg[8..14].copy_from_slice(&[0x00, 0x3c, 0x14, 0xae, 0xc7, 0x3e]); // real GMC-300Re 4.54: 60 cpm = 0.39 uSv/h
            Script { out: VecDeque::new(), cfg, beating: false, counts, next: 0, serial: [1, 2, 3, 4, 5, 6, 7], junk_first: false }
        }
    }

    impl GmcIo for Script {
        fn write(&mut self, data: &[u8]) -> Result<()> {
            match data {
                b"<GETVER>>" => self.out.extend(b"GMC-300Re 4.54"),
                b"<GETSERIAL>>" => self.out.extend(self.serial),
                b"<GETCFG>>" => self.out.extend(self.cfg.clone()),
                b"<HEARTBEAT1>>" => {
                    self.beating = true;
                    if self.junk_first {
                        self.out.push_back(0xAA);
                    }
                }
                b"<HEARTBEAT0>>" => self.beating = false,
                _ => {}
            }
            Ok(())
        }
        fn read(&mut self, buf: &mut [u8], _t: Duration) -> Result<usize> {
            if self.out.is_empty() && self.beating && self.next < self.counts.len() {
                let c = self.counts[self.next];
                self.next += 1;
                self.out.extend(c.to_be_bytes());
            }
            let mut n = 0;
            while n < buf.len() {
                match self.out.pop_front() {
                    Some(b) => {
                        buf[n] = b;
                        n += 1;
                    }
                    None => break,
                }
            }
            Ok(n)
        }
        fn drain(&mut self) {
            if !self.beating {
                self.out.clear();
            }
        }
    }

    fn clock() -> u32 {
        1_790_000_000
    }

    #[test]
    fn calibration_of_a_real_counter() {
        let mut cfg = vec![0u8; 256];
        cfg[8..14].copy_from_slice(&[0x00, 0x3c, 0x14, 0xae, 0xc7, 0x3e]);
        assert_eq!(sensitivity_x100(&cfg), Some(15385));
        let mut be = vec![0u8; 256];
        be[8..14].copy_from_slice(&[0x00, 0x3c, 0x3e, 0xc7, 0xae, 0x14]);
        assert_eq!(sensitivity_x100(&be), Some(15385));
        assert_eq!(sensitivity_x100(&[0u8; 256]), None);
    }

    #[test]
    fn recognises_the_counter_and_reads_its_identity() {
        let mut d = GmcDetector::open(Script::new(vec![])).unwrap();
        let i = d.info().unwrap();
        assert_eq!((i.driver, i.firmware.as_str(), i.sens_x100, i.dead_us), ("gmc", "GMC-300Re 4.54", Some(15385), Some(90)));
        assert_eq!(i.model, "GQ GMC-300Re");
        assert_eq!(i.unique_id.as_deref(), Some("gmc|GMC-300|01020304050607"));
    }

    #[test]
    fn refuses_what_is_not_a_counter() {
        struct Dumb;
        impl GmcIo for Dumb {
            fn write(&mut self, _: &[u8]) -> Result<()> {
                Ok(())
            }
            fn read(&mut self, _: &mut [u8], _: Duration) -> Result<usize> {
                Ok(0)
            }
            fn drain(&mut self) {}
        }
        assert!(GmcDetector::open(Dumb).is_err());
    }

    #[test]
    fn rate_from_the_counts_of_the_last_minute() {
        // 2 counts every second: 120 cpm
        let mut d = GmcDetector::open(Script::new(vec![2; 120])).unwrap().with_clock(clock);
        d.live_start().unwrap();
        let mut first = None;
        for n in 1..=40 {
            if let Some(r) = d.next_live(Duration::from_millis(5)).unwrap() {
                first = Some((n, r));
                break;
            }
        }
        let (at, r) = first.unwrap();
        assert_eq!(at, 10, "nothing in the first nine seconds, the first reading at ten");
        assert!((r.cpm - 120.0).abs() < 1e-9);
        // 120 cpm on 153.85 cpm per uSv/h is 0.78 uSv/h, a little more with the dead time
        assert!((r.nsv_h - 780.0).abs() < 10.0, "{}", r.nsv_h);
        assert_eq!(r.time, 1_790_000_000);
    }

    #[test]
    fn finds_the_framing_after_a_stray_byte() {
        let mut s = Script::new(vec![3; 40]);
        s.junk_first = true;
        let mut d = GmcDetector::open(s).unwrap();
        d.live_start().unwrap();
        let mut got = None;
        for _ in 0..80 {
            if let Some(r) = d.next_live(Duration::from_millis(5)).unwrap() {
                got = Some(r);
                break;
            }
        }
        assert!((got.unwrap().cpm - 180.0).abs() < 1e-9);
    }
}
