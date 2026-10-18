//! What the Hub (and a headless monitor) needs from any Geiger counter. The Yarrow/GC-03 is the first driver; another
//! counter is another implementation of [`Detector`].
//!
//! Radiation counts are the shared currency. Anything else a device measures (the GC-03's electric and magnetic field)
//! travels in [`Reading::local_only`]: it is for the local views and must never be stored for sharing or sent anywhere.

use crate::client::{Calibration, Client, Event, Info};
use crate::{Result, Sample, Transport};
use serde::Serialize;
use std::time::Duration;

#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct DetectorInfo {
    /// Name of the driver, e.g. "yarrow".
    pub driver: &'static str,
    pub model: String,
    pub firmware: String,
    pub lang: Option<String>,
    /// The device's own unique id, when it reports one.
    pub unique_id: Option<String>,
    pub sens_x100: Option<u32>,
    pub dead_us: Option<u32>,
}

#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct LocalValue {
    pub name: String,
    pub unit: String,
    pub value: f64,
}

/// One live reading.
#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct Reading {
    pub time: u32,
    pub cpm: f64,
    pub nsv_h: f64,
    pub battery: Option<u32>,
    /// Values for the local views only, never for sharing.
    pub local_only: Vec<LocalValue>,
}

pub trait Detector: Send {
    fn info(&mut self) -> Result<DetectorInfo>;
    fn live_start(&mut self) -> Result<()>;
    fn live_stop(&mut self) -> Result<()>;
    fn next_live(&mut self, timeout: Duration) -> Result<Option<Reading>>;
    /// Reads the stored history and decodes it. `progress` gets (done, total) steps.
    fn download_log(&mut self, progress: &mut dyn FnMut(u32, u32)) -> Result<Vec<Sample>>;
    /// Extras only some devices have (clock, calibration, alarm events, theme).
    fn tools(&mut self) -> Option<&mut dyn Tools> {
        None
    }
}

pub trait Tools {
    fn events(&mut self) -> Result<Vec<Event>>;
    fn cal(&mut self) -> Result<Calibration>;
    fn set_cal(&mut self, c: &Calibration) -> Result<()>;
    fn set_theme(&mut self, n: u32) -> Result<()>;
    fn time(&mut self) -> Result<u32>;
    fn set_time(&mut self, unix: u32) -> Result<()>;
    fn raw_info(&mut self) -> Result<Info>;
}

/// The Yarrow firmware on a GC-03.
pub struct YarrowDetector<T: Transport>(pub Client<T>);

impl<T: Transport> YarrowDetector<T> {
    pub fn new(t: T) -> Self {
        YarrowDetector(Client::new(t))
    }
}

impl<T: Transport> Detector for YarrowDetector<T> {
    fn info(&mut self) -> Result<DetectorInfo> {
        self.0.ping()?;
        let i = self.0.info()?;
        let c = self.0.cal()?;
        Ok(DetectorInfo {
            driver: "yarrow",
            model: "FNIRSI GC-03".into(),
            firmware: i.version,
            lang: i.lang,
            unique_id: i.uid,
            sens_x100: Some(c.sens_x100),
            dead_us: Some(c.dead_us),
        })
    }

    fn live_start(&mut self) -> Result<()> {
        self.0.live_start()
    }

    fn live_stop(&mut self) -> Result<()> {
        self.0.live_stop()
    }

    fn next_live(&mut self, timeout: Duration) -> Result<Option<Reading>> {
        Ok(self.0.next_live(timeout)?.map(|s| Reading {
            time: s.time,
            cpm: s.cpm,
            nsv_h: s.nsv_h,
            battery: Some(s.battery),
            local_only: vec![
                LocalValue { name: "e_field".into(), unit: "V/m".into(), value: s.e_vm },
                LocalValue { name: "h_field".into(), unit: "µT".into(), value: s.h_ut },
            ],
        }))
    }

    fn download_log(&mut self, progress: &mut dyn FnMut(u32, u32)) -> Result<Vec<Sample>> {
        let (li, image) = self.0.download_log(|d, t| progress(d, t))?;
        Ok(crate::decode_image(&image, li.pagesize as usize))
    }

    fn tools(&mut self) -> Option<&mut dyn Tools> {
        Some(self)
    }
}

impl<T: Transport> Tools for YarrowDetector<T> {
    fn events(&mut self) -> Result<Vec<Event>> {
        self.0.events()
    }
    fn cal(&mut self) -> Result<Calibration> {
        self.0.cal()
    }
    fn set_cal(&mut self, c: &Calibration) -> Result<()> {
        self.0.set_cal(c)
    }
    fn set_theme(&mut self, n: u32) -> Result<()> {
        self.0.set_theme(n)
    }
    fn time(&mut self) -> Result<u32> {
        self.0.time()
    }
    fn set_time(&mut self, unix: u32) -> Result<()> {
        self.0.set_time(unix)
    }
    fn raw_info(&mut self) -> Result<Info> {
        self.0.info()
    }
}
