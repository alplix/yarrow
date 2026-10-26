//! A device that lives in memory: speaks the same text protocol, serves a recorded log image.
//! Used by the tests and by the demo mode of the app.

use crate::transport::Transport;
use crate::{decode_image, Result};
use base64::Engine;
use std::collections::VecDeque;
use std::time::Duration;

/// A log image made by the firmware's own writer (`make hostcheck`): 12 pages of 1 KB, five sessions of
/// one-minute samples, one of them ended by a power cut.
pub const DEMO_IMAGE: &[u8] = include_bytes!("../data/demo_flash.bin");
pub const DEMO_PAGE_SIZE: usize = 1024;

pub struct FakeDevice {
    image: Vec<u8>,
    page_size: usize,
    pub uid: Option<String>,
    pub lang: Option<String>,
    pub version: String,
    pub clock: u32,
    pub theme: u32,
    pub cal: [u32; 4],
    pub events: Vec<[u32; 4]>,
    live: bool,
    /// Wait a second between live lines, as the real device does.
    pub realtime: bool,
    last_live: Option<std::time::Instant>,
    rng: u64,
    out: VecDeque<String>,
    /// Lines received, for tests.
    pub log: Vec<String>,
}

impl FakeDevice {
    pub fn new(image: Vec<u8>, page_size: usize) -> Self {
        let clock = decode_image(&image, page_size).last().map_or(1_790_000_000, |s| s.time + 3600);
        FakeDevice {
            image,
            page_size,
            uid: None,
            lang: None,
            version: "0.1.0".into(),
            clock,
            theme: 0,
            cal: [18000, 90, 100, 100],
            events: vec![[1_791_980_000, 95, 640, 1], [1_792_050_000, 12, 1210, 2]],
            live: false,
            realtime: false,
            last_live: None,
            rng: 0x9E37_79B9_7F4A_7C15,
            out: VecDeque::new(),
            log: Vec::new(),
        }
    }

    /// The device with the demo log.
    pub fn demo() -> Self {
        Self::new(DEMO_IMAGE.to_vec(), DEMO_PAGE_SIZE)
    }


    fn rnd(&mut self) -> u32 {
        self.rng ^= self.rng << 13;
        self.rng ^= self.rng >> 7;
        self.rng ^= self.rng << 17;
        (self.rng >> 32) as u32
    }

    fn live_line(&mut self) -> String {
        self.clock += 1;
        let cpm = 6 + self.rnd() % 5;
        let nsv = cpm * 100_000 / self.cal[0].max(1) * 10;
        let e = 12 + self.rnd() % 6;
        let h = 5 + self.rnd() % 3;
        format!("L {} {} {} {} {} 87", self.clock, nsv, cpm, e, h)
    }

    fn reply(&mut self, cmd: &str) {
        let mut it = cmd.split_whitespace();
        let name = it.next().unwrap_or("");
        let args: Vec<&str> = it.collect();
        let o = &mut self.out;
        match (name, args.as_slice()) {
            ("PING", []) => o.push_back("OK pong".into()),
            ("HEALTH", []) => o.push_back("OK boots=3 wdt=0 fault=0 usb_restarts=0 reason=power_on streak=0 fault_pc=0 uptime=5400 stage=5 suspect=0".into()),
            ("INFO", []) => {
                let mut s = format!(
                    "OK yarrow={} stage=5 verified=1 time={} theme={} profile=2 pages={} pagesize={}",
                    self.version,
                    self.clock,
                    self.theme,
                    self.image.len() / self.page_size,
                    self.page_size
                );
                if let Some(u) = &self.uid {
                    s.push_str(&format!(" uid={u}"));
                }
                if let Some(l) = &self.lang {
                    s.push_str(&format!(" lang={l}"));
                }
                o.push_back(s);
            }
            ("TIME", []) => o.push_back(format!("OK {}", self.clock)),
            ("TIME", [t]) => match t.parse::<u32>() {
                Ok(t) => {
                    self.clock = t;
                    o.push_back("OK".into());
                }
                Err(_) => o.push_back("ERR args".into()),
            },
            ("LIVE", ["1"]) => {
                self.live = true;
                o.push_back("OK".into());
            }
            ("LIVE", ["0"]) => {
                self.live = false;
                o.push_back("OK".into());
            }
            ("LOGINFO", []) => {
                let s = decode_image(&self.image, self.page_size);
                let used = self.image.chunks(self.page_size).filter(|p| p[0] == 0x59).count();
                o.push_back(format!(
                    "OK pages={} pagesize={} used={} oldest={} newest={}",
                    (self.image.len() / self.page_size),
                    self.page_size,
                    used,
                    s.first().map_or(0, |x| x.time),
                    s.last().map_or(0, |x| x.time)
                ));
            }
            ("LOGPAGE", [n]) => match n.parse::<usize>() {
                Ok(n) if n < (self.image.len() / self.page_size) => {
                    let pg = &self.image[n * self.page_size..(n + 1) * self.page_size];
                    o.push_back(format!("OK {n} {}", pg.len()));
                    let b = base64::engine::general_purpose::STANDARD.encode(pg);
                    for chunk in b.as_bytes().chunks(76) {
                        o.push_back(String::from_utf8_lossy(chunk).into_owned());
                    }
                    o.push_back("END".into());
                }
                _ => o.push_back("ERR range".into()),
            },
            ("EVENTS", []) => {
                o.push_back(format!("OK {}", self.events.len()));
                for e in &self.events {
                    o.push_back(format!("E {} {} {} {}", e[0], e[1], e[2], e[3]));
                }
                o.push_back("END".into());
            }
            ("CAL", []) => {
                o.push_back(format!("OK sens={} dead={} e={} h={}", self.cal[0], self.cal[1], self.cal[2], self.cal[3]))
            }
            ("CAL", [a, b, c, d]) => {
                let v: Vec<Option<u32>> = [a, b, c, d].iter().map(|x| x.parse().ok()).collect();
                if v.iter().any(|x| x.is_none()) {
                    o.push_back("ERR args".into());
                } else {
                    let v: Vec<u32> = v.into_iter().flatten().collect();
                    if v[0] < 100 || v[0] > 100_000 || v[1] > 1000 || v[2] > 400 || v[3] > 400 {
                        o.push_back("ERR range".into());
                    } else {
                        self.cal = [v[0], v[1], v[2], v[3]];
                        o.push_back("OK".into());
                    }
                }
            }
            ("SET", ["THEME", n]) => match n.parse::<u32>() {
                Ok(n) if n <= 20 => {
                    self.theme = n;
                    o.push_back("OK".into());
                }
                _ => o.push_back("ERR range".into()),
            },
            _ => o.push_back("ERR unknown".into()),
        }
    }
}

impl Transport for FakeDevice {
    fn send_line(&mut self, line: &str) -> Result<()> {
        self.log.push(line.to_string());
        self.reply(line.trim());
        Ok(())
    }

    fn recv_line(&mut self, timeout: Duration) -> Result<Option<String>> {
        if let Some(l) = self.out.pop_front() {
            return Ok(Some(l));
        }
        if self.live {
            if self.realtime {
                let due = self.last_live.map_or(Duration::ZERO, |t| Duration::from_secs(1).saturating_sub(t.elapsed()));
                if due > timeout {
                    std::thread::sleep(timeout);
                    return Ok(None);
                }
                std::thread::sleep(due);
                self.last_live = Some(std::time::Instant::now());
            }
            return Ok(Some(self.live_line()));
        }
        Ok(None)
    }
}
