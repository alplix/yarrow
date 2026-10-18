//! The text protocol, see `docs/serial-protocol.md`.

use crate::transport::Transport;
use crate::{Error, Result};
use base64::Engine;
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, VecDeque};
use std::time::Duration;

const REPLY_TIMEOUT: Duration = Duration::from_millis(2000);

/// Reply of `INFO`. Fields the firmware may add later (`uid`, `lang`) are optional; everything else the device
/// reports is kept in `extra`.
#[derive(Debug, Clone, Serialize, Default, PartialEq, Eq)]
pub struct Info {
    pub version: String,
    pub stage: u32,
    pub verified: u32,
    pub time: u32,
    pub theme: u32,
    pub profile: u32,
    pub pages: u32,
    pub pagesize: u32,
    pub uid: Option<String>,
    pub lang: Option<String>,
    pub extra: BTreeMap<String, String>,
}

#[derive(Debug, Clone, Serialize, Default, PartialEq, Eq)]
pub struct LogInfo {
    pub pages: u32,
    pub pagesize: u32,
    pub used: u32,
    pub oldest: u32,
    pub newest: u32,
}

/// One line of the live stream.
#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct LiveSample {
    pub time: u32,
    pub nsv_h: f64,
    pub cpm: f64,
    /// Electric field in V/m.
    pub e_vm: f64,
    /// Magnetic field in µT.
    pub h_ut: f64,
    pub battery: u32,
}

/// An alarm event.
#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct Event {
    pub start: u32,
    pub duration_s: u32,
    pub peak_nsv_h: f64,
    pub level: u32,
}

/// Calibration as the device stores it.
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
pub struct Calibration {
    /// Counts per minute for 1 µSv/h, times 100.
    pub sens_x100: u32,
    /// Dead time in µs.
    pub dead_us: u32,
    pub e_pct: u32,
    pub h_pct: u32,
}

fn kv_map(s: &str) -> BTreeMap<String, String> {
    s.split_whitespace()
        .filter_map(|t| t.split_once('='))
        .map(|(k, v)| (k.to_string(), v.to_string()))
        .collect()
}

fn num<T: std::str::FromStr>(m: &mut BTreeMap<String, String>, k: &str) -> Result<T> {
    let v = m.remove(k).ok_or_else(|| Error::Protocol(format!("missing {k}")))?;
    v.parse().map_err(|_| Error::Protocol(format!("bad {k}={v}")))
}

pub fn parse_info(s: &str) -> Result<Info> {
    let mut m = kv_map(s);
    Ok(Info {
        version: m.remove("yarrow").ok_or_else(|| Error::Protocol("missing yarrow".into()))?,
        stage: num(&mut m, "stage")?,
        verified: num(&mut m, "verified")?,
        time: num(&mut m, "time")?,
        theme: num(&mut m, "theme")?,
        profile: num(&mut m, "profile")?,
        pages: num(&mut m, "pages")?,
        pagesize: num(&mut m, "pagesize")?,
        uid: m.remove("uid"),
        lang: m.remove("lang"),
        extra: m,
    })
}

pub fn parse_live(line: &str) -> Result<LiveSample> {
    let bad = || Error::Protocol(format!("bad live line: {line}"));
    let mut it = line.split_whitespace();
    if it.next() != Some("L") {
        return Err(bad());
    }
    let mut f = || it.next().and_then(|x| x.parse::<f64>().ok()).ok_or_else(bad);
    let (time, nsv, cpm, e, h, bat) = (f()?, f()?, f()?, f()?, f()?, f()?);
    Ok(LiveSample { time: time as u32, nsv_h: nsv, cpm, e_vm: e / 10.0, h_ut: h / 100.0, battery: bat as u32 })
}

pub struct Client<T: Transport> {
    t: T,
    live: VecDeque<LiveSample>,
}

impl<T: Transport> Client<T> {
    pub fn new(t: T) -> Self {
        Client { t, live: VecDeque::new() }
    }

    pub fn into_inner(self) -> T {
        self.t
    }

    /// Next line that is not part of the live stream; live lines are queued for [`Self::next_live`].
    fn line(&mut self, timeout: Duration) -> Result<Option<String>> {
        loop {
            match self.t.recv_line(timeout)? {
                Some(l) if l.starts_with("L ") => {
                    if let Ok(s) = parse_live(&l) {
                        if self.live.len() < 600 {
                            self.live.push_back(s);
                        }
                    }
                }
                other => return Ok(other),
            }
        }
    }

    fn reply(&mut self) -> Result<String> {
        loop {
            let l = self.line(REPLY_TIMEOUT)?.ok_or(Error::Timeout)?;
            if l == "OK" {
                return Ok(String::new());
            }
            if let Some(r) = l.strip_prefix("OK ") {
                return Ok(r.to_string());
            }
            if l == "ERR" {
                return Err(Error::Device(String::new()));
            }
            if let Some(r) = l.strip_prefix("ERR ") {
                return Err(Error::Device(r.to_string()));
            }
            // Stray text (a banner, an echo): skip it.
        }
    }

    pub fn request(&mut self, cmd: &str) -> Result<String> {
        self.t.send_line(cmd)?;
        self.reply()
    }

    pub fn ping(&mut self) -> Result<()> {
        let r = self.request("PING")?;
        if r == "pong" {
            Ok(())
        } else {
            Err(Error::Protocol(r))
        }
    }

    pub fn info(&mut self) -> Result<Info> {
        parse_info(&self.request("INFO")?)
    }

    pub fn time(&mut self) -> Result<u32> {
        self.request("TIME")?.trim().parse().map_err(|_| Error::Protocol("bad time".into()))
    }

    pub fn set_time(&mut self, unix: u32) -> Result<()> {
        self.request(&format!("TIME {unix}")).map(|_| ())
    }

    pub fn set_theme(&mut self, n: u32) -> Result<()> {
        self.request(&format!("SET THEME {n}")).map(|_| ())
    }

    pub fn live_start(&mut self) -> Result<()> {
        self.request("LIVE 1").map(|_| ())
    }

    pub fn live_stop(&mut self) -> Result<()> {
        let r = self.request("LIVE 0");
        self.live.clear();
        r.map(|_| ())
    }

    /// The next live sample, waiting at most `timeout`.
    pub fn next_live(&mut self, timeout: Duration) -> Result<Option<LiveSample>> {
        if let Some(s) = self.live.pop_front() {
            return Ok(Some(s));
        }
        match self.t.recv_line(timeout)? {
            Some(l) if l.starts_with("L ") => Ok(Some(parse_live(&l)?)),
            _ => Ok(None),
        }
    }

    pub fn log_info(&mut self) -> Result<LogInfo> {
        let mut m = kv_map(&self.request("LOGINFO")?);
        Ok(LogInfo {
            pages: num(&mut m, "pages")?,
            pagesize: num(&mut m, "pagesize")?,
            used: num(&mut m, "used")?,
            oldest: num(&mut m, "oldest")?,
            newest: num(&mut m, "newest")?,
        })
    }

    /// One raw logical page.
    pub fn log_page(&mut self, n: u32) -> Result<Vec<u8>> {
        let head = self.request(&format!("LOGPAGE {n}"))?;
        let mut it = head.split_whitespace();
        let (got, size): (u32, usize) = match (
            it.next().and_then(|x| x.parse().ok()),
            it.next().and_then(|x| x.parse().ok()),
        ) {
            (Some(a), Some(b)) => (a, b),
            _ => return Err(Error::Protocol(head)),
        };
        if got != n {
            return Err(Error::Protocol(format!("asked for page {n}, got {got}")));
        }
        let mut b64 = String::new();
        loop {
            let l = self.line(REPLY_TIMEOUT)?.ok_or(Error::Timeout)?;
            if l == "END" {
                break;
            }
            b64.push_str(l.trim());
        }
        let data = base64::engine::general_purpose::STANDARD
            .decode(b64.as_bytes())
            .map_err(|e| Error::Protocol(format!("bad base64: {e}")))?;
        if data.len() != size {
            return Err(Error::Protocol(format!("page {n}: expected {size} bytes, got {}", data.len())));
        }
        Ok(data)
    }

    /// The whole log window, page after page, in the order of the device's logical pages.
    /// `progress` is called with (pages done, pages total).
    pub fn download_log(&mut self, mut progress: impl FnMut(u32, u32)) -> Result<(LogInfo, Vec<u8>)> {
        let li = self.log_info()?;
        let mut image = Vec::with_capacity((li.pages * li.pagesize) as usize);
        for n in 0..li.pages {
            let pg = self.log_page(n)?;
            if pg.len() != li.pagesize as usize {
                return Err(Error::Protocol(format!("page {n} has {} bytes, expected {}", pg.len(), li.pagesize)));
            }
            image.extend_from_slice(&pg);
            progress(n + 1, li.pages);
        }
        Ok((li, image))
    }

    pub fn events(&mut self) -> Result<Vec<Event>> {
        let count: usize = self.request("EVENTS")?.trim().parse().map_err(|_| Error::Protocol("bad count".into()))?;
        let mut v = Vec::new();
        loop {
            let l = self.line(REPLY_TIMEOUT)?.ok_or(Error::Timeout)?;
            if l == "END" {
                break;
            }
            let p: Vec<&str> = l.split_whitespace().collect();
            if p.len() == 5 && p[0] == "E" {
                let bad = || Error::Protocol(format!("bad event: {l}"));
                v.push(Event {
                    start: p[1].parse().map_err(|_| bad())?,
                    duration_s: p[2].parse().map_err(|_| bad())?,
                    peak_nsv_h: p[3].parse().map_err(|_| bad())?,
                    level: p[4].parse().map_err(|_| bad())?,
                });
            }
        }
        if v.len() != count {
            return Err(Error::Protocol(format!("expected {count} events, got {}", v.len())));
        }
        Ok(v)
    }

    pub fn cal(&mut self) -> Result<Calibration> {
        let mut m = kv_map(&self.request("CAL")?);
        Ok(Calibration {
            sens_x100: num(&mut m, "sens")?,
            dead_us: num(&mut m, "dead")?,
            e_pct: num(&mut m, "e")?,
            h_pct: num(&mut m, "h")?,
        })
    }

    pub fn set_cal(&mut self, c: &Calibration) -> Result<()> {
        self.request(&format!("CAL {} {} {} {}", c.sens_x100, c.dead_us, c.e_pct, c.h_pct)).map(|_| ())
    }
}
