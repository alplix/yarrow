//! The Hub's local database: one SQLite file, every device in it.
//!
//! Samples with an unset device clock are not stored: their times mean nothing.

use rusqlite::{params, Connection, OptionalExtension};
use serde::{Deserialize, Serialize};
use std::io::Write;
use std::path::Path;
use yarrow_core::Sample;

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("database: {0}")]
    Db(#[from] rusqlite::Error),
    #[error("{0}")]
    Io(#[from] std::io::Error),
}

pub type Result<T> = std::result::Result<T, Error>;

/// A device the Hub has seen.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Device {
    /// Hash of the device's unique id (or of the serial port's own id when the firmware has none).
    pub id: String,
    pub name: String,
    pub firmware: String,
    pub lang: String,
    pub last_seen: u32,
    /// Last known calibration, so counts can be converted without the device.
    pub sens_x100: u32,
    pub dead_us: u32,
}

/// One point of a downsampled chart series: counts per minute, minimum / mean / maximum inside the bucket.
#[derive(Debug, Clone, Serialize, PartialEq)]
pub struct SeriesPoint {
    pub t: u32,
    pub min: f64,
    pub avg: f64,
    pub max: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct UploadRecord {
    pub id: i64,
    pub device: String,
    /// Unix time the upload was made.
    pub at: u32,
    pub server: String,
    pub interval_s: u32,
    pub start: u32,
    pub bins: u32,
    pub accepted: u32,
    pub rejected: u32,
    pub credit: f64,
    pub ok: bool,
    pub message: String,
}

pub struct Store {
    conn: Connection,
}

impl Store {
    pub fn open(path: &Path) -> Result<Self> {
        Self::init(Connection::open(path)?)
    }

    pub fn open_memory() -> Result<Self> {
        Self::init(Connection::open_in_memory()?)
    }

    fn init(conn: Connection) -> Result<Self> {
        conn.execute_batch(
            "PRAGMA journal_mode = WAL;
             PRAGMA synchronous = NORMAL;
             CREATE TABLE IF NOT EXISTS device (
                 id TEXT PRIMARY KEY, name TEXT NOT NULL, firmware TEXT NOT NULL, lang TEXT NOT NULL,
                 last_seen INTEGER NOT NULL, sens_x100 INTEGER NOT NULL, dead_us INTEGER NOT NULL);
             CREATE TABLE IF NOT EXISTS sample (
                 device TEXT NOT NULL, t INTEGER NOT NULL, iv INTEGER NOT NULL, value INTEGER NOT NULL,
                 PRIMARY KEY (device, t, iv)) WITHOUT ROWID;
             CREATE TABLE IF NOT EXISTS upload (
                 id INTEGER PRIMARY KEY, device TEXT NOT NULL, at INTEGER NOT NULL, server TEXT NOT NULL,
                 interval_s INTEGER NOT NULL, start INTEGER NOT NULL, bins INTEGER NOT NULL,
                 accepted INTEGER NOT NULL, rejected INTEGER NOT NULL, credit REAL NOT NULL,
                 ok INTEGER NOT NULL, message TEXT NOT NULL);
             CREATE TABLE IF NOT EXISTS cursor (
                 device TEXT NOT NULL, server TEXT NOT NULL, interval_s INTEGER NOT NULL, next_after INTEGER NOT NULL,
                 PRIMARY KEY (device, server, interval_s));",
        )?;
        Ok(Store { conn })
    }

    pub fn upsert_device(&self, d: &Device) -> Result<()> {
        self.conn.execute(
            "INSERT INTO device (id, name, firmware, lang, last_seen, sens_x100, dead_us) VALUES (?1,?2,?3,?4,?5,?6,?7)
             ON CONFLICT(id) DO UPDATE SET firmware=?3, lang=?4, last_seen=?5, sens_x100=?6, dead_us=?7,
                 name = CASE WHEN device.name = '' THEN ?2 ELSE device.name END",
            params![d.id, d.name, d.firmware, d.lang, d.last_seen, d.sens_x100, d.dead_us],
        )?;
        Ok(())
    }

    pub fn rename_device(&self, id: &str, name: &str) -> Result<()> {
        self.conn.execute("UPDATE device SET name=?2 WHERE id=?1", params![id, name])?;
        Ok(())
    }

    pub fn devices(&self) -> Result<Vec<Device>> {
        let mut st = self.conn.prepare(
            "SELECT id, name, firmware, lang, last_seen, sens_x100, dead_us FROM device ORDER BY last_seen DESC",
        )?;
        let rows = st.query_map([], |r| {
            Ok(Device {
                id: r.get(0)?,
                name: r.get(1)?,
                firmware: r.get(2)?,
                lang: r.get(3)?,
                last_seen: r.get(4)?,
                sens_x100: r.get(5)?,
                dead_us: r.get(6)?,
            })
        })?;
        Ok(rows.collect::<std::result::Result<_, _>>()?)
    }

    pub fn device(&self, id: &str) -> Result<Option<Device>> {
        Ok(self.devices()?.into_iter().find(|d| d.id == id))
    }

    /// Stores the samples that are new and have a valid time; returns how many were new.
    pub fn insert_samples(&mut self, device: &str, samples: &[Sample]) -> Result<usize> {
        let tx = self.conn.transaction()?;
        let mut n = 0;
        {
            let mut st = tx.prepare_cached("INSERT OR IGNORE INTO sample (device, t, iv, value) VALUES (?1,?2,?3,?4)")?;
            for s in samples.iter().filter(|s| s.time_valid) {
                n += st.execute(params![device, s.time, s.interval, s.value])?;
            }
        }
        tx.commit()?;
        Ok(n)
    }

    pub fn sample_count(&self, device: &str) -> Result<u64> {
        Ok(self.conn.query_row("SELECT COUNT(*) FROM sample WHERE device=?1", [device], |r| r.get::<_, i64>(0))? as u64)
    }

    /// Time of the first and the end of the last stored sample.
    pub fn time_range(&self, device: &str) -> Result<Option<(u32, u32)>> {
        let (lo, hi) = self.conn.query_row("SELECT MIN(t), MAX(t + iv) FROM sample WHERE device=?1", [device], |r| {
            Ok((r.get::<_, Option<u32>>(0)?, r.get::<_, Option<u32>>(1)?))
        })?;
        Ok(lo.zip(hi))
    }

    /// Samples with a start in `[from, to)`, oldest first.
    pub fn samples_between(&self, device: &str, from: u32, to: u32) -> Result<Vec<Sample>> {
        let mut st = self
            .conn
            .prepare_cached("SELECT t, iv, value FROM sample WHERE device=?1 AND t>=?2 AND t<?3 ORDER BY t, iv")?;
        let rows = st.query_map(params![device, from, to], |r| {
            Ok(Sample { time: r.get(0)?, interval: r.get(1)?, time_valid: true, value: r.get(2)? })
        })?;
        Ok(rows.collect::<std::result::Result<_, _>>()?)
    }

    /// A chart series of counts per minute over `[from, to)` in at most `max_points` buckets. Where a finer stream
    /// (the alarm's one-second samples) runs next to the profile samples, both feed the same bucket.
    pub fn series(&self, device: &str, from: u32, to: u32, max_points: u32) -> Result<(u32, Vec<SeriesPoint>)> {
        let span = to.saturating_sub(from).max(1);
        let bucket = span.div_ceil(max_points.max(1)).max(1);
        let mut st = self.conn.prepare_cached(
            "SELECT (t - ?2) / ?4 AS b, MIN(value * 60.0 / iv), AVG(value * 60.0 / iv), MAX(value * 60.0 / iv)
             FROM sample WHERE device=?1 AND t>=?2 AND t<?3 GROUP BY b ORDER BY b",
        )?;
        let rows = st.query_map(params![device, from, to, bucket], |r| {
            Ok(SeriesPoint { t: from + r.get::<_, u32>(0)? * bucket + bucket / 2, min: r.get(1)?, avg: r.get(2)?, max: r.get(3)? })
        })?;
        Ok((bucket, rows.collect::<std::result::Result<_, _>>()?))
    }

    /// Writes every stored sample of the device as CSV: `time,interval,value`. Returns the number of rows.
    pub fn export_csv(&self, device: &str, mut w: impl Write) -> Result<u64> {
        writeln!(w, "time,interval,value")?;
        let mut st = self.conn.prepare("SELECT t, iv, value FROM sample WHERE device=?1 ORDER BY t, iv")?;
        let mut rows = st.query([device])?;
        let mut n = 0;
        while let Some(r) = rows.next()? {
            writeln!(w, "{},{},{}", r.get::<_, u32>(0)?, r.get::<_, u32>(1)?, r.get::<_, u32>(2)?)?;
            n += 1;
        }
        Ok(n)
    }

    // ---- sharing ----

    /// Where the last accepted upload for this device, server and bin length ended.
    pub fn next_after(&self, device: &str, server: &str, interval_s: u32) -> Result<Option<u32>> {
        Ok(self
            .conn
            .query_row(
                "SELECT next_after FROM cursor WHERE device=?1 AND server=?2 AND interval_s=?3",
                params![device, server, interval_s],
                |r| r.get(0),
            )
            .optional()?)
    }

    pub fn set_next_after(&self, device: &str, server: &str, interval_s: u32, next_after: u32) -> Result<()> {
        self.conn.execute(
            "INSERT INTO cursor (device, server, interval_s, next_after) VALUES (?1,?2,?3,?4)
             ON CONFLICT(device, server, interval_s) DO UPDATE SET next_after=MAX(next_after, ?4)",
            params![device, server, interval_s, next_after],
        )?;
        Ok(())
    }

    pub fn record_upload(&self, u: &UploadRecord) -> Result<i64> {
        self.conn.execute(
            "INSERT INTO upload (device, at, server, interval_s, start, bins, accepted, rejected, credit, ok, message)
             VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11)",
            params![u.device, u.at, u.server, u.interval_s, u.start, u.bins, u.accepted, u.rejected, u.credit, u.ok, u.message],
        )?;
        Ok(self.conn.last_insert_rowid())
    }

    pub fn uploads(&self, device: &str, limit: u32) -> Result<Vec<UploadRecord>> {
        let mut st = self.conn.prepare(
            "SELECT id, device, at, server, interval_s, start, bins, accepted, rejected, credit, ok, message
             FROM upload WHERE device=?1 ORDER BY id DESC LIMIT ?2",
        )?;
        let rows = st.query_map(params![device, limit], |r| {
            Ok(UploadRecord {
                id: r.get(0)?,
                device: r.get(1)?,
                at: r.get(2)?,
                server: r.get(3)?,
                interval_s: r.get(4)?,
                start: r.get(5)?,
                bins: r.get(6)?,
                accepted: r.get(7)?,
                rejected: r.get(8)?,
                credit: r.get(9)?,
                ok: r.get(10)?,
                message: r.get(11)?,
            })
        })?;
        Ok(rows.collect::<std::result::Result<_, _>>()?)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use yarrow_core::fake::{DEMO_IMAGE, DEMO_PAGE_SIZE};
    use yarrow_core::decode_image;

    fn dev() -> Device {
        Device { id: "abc".into(), name: "".into(), firmware: "0.1.0".into(), lang: "en".into(), last_seen: 5, sens_x100: 8000, dead_us: 90 }
    }

    #[test]
    fn samples_are_stored_once_and_read_back() {
        let mut s = Store::open_memory().unwrap();
        s.upsert_device(&dev()).unwrap();
        let samples = decode_image(DEMO_IMAGE, DEMO_PAGE_SIZE);
        assert_eq!(s.insert_samples("abc", &samples).unwrap(), samples.len());
        assert_eq!(s.insert_samples("abc", &samples).unwrap(), 0);
        assert_eq!(s.sample_count("abc").unwrap(), 19484);
        let (a, b) = s.time_range("abc").unwrap().unwrap();
        assert_eq!(a, samples[0].time);
        assert_eq!(b, samples.last().unwrap().time + 60);
        assert_eq!(s.samples_between("abc", a, b).unwrap().len(), 19484);
        let (bucket, series) = s.series("abc", a, b, 500).unwrap();
        assert!(series.len() <= 501 && bucket > 60);
        assert!(series.iter().all(|p| p.min <= p.avg && p.avg <= p.max));
        let mut csv = Vec::new();
        assert_eq!(s.export_csv("abc", &mut csv).unwrap(), 19484);
        assert!(String::from_utf8(csv).unwrap().starts_with("time,interval,value\n"));
    }

    #[test]
    fn unset_clock_samples_are_not_stored() {
        let mut s = Store::open_memory().unwrap();
        let v = Sample { time: 100, interval: 60, time_valid: false, value: 3 };
        assert_eq!(s.insert_samples("abc", &[v]).unwrap(), 0);
    }

    #[test]
    fn device_name_survives_updates_and_cursor_only_moves_forward() {
        let s = Store::open_memory().unwrap();
        s.upsert_device(&dev()).unwrap();
        s.rename_device("abc", "Garage").unwrap();
        s.upsert_device(&Device { last_seen: 9, ..dev() }).unwrap();
        let d = s.device("abc").unwrap().unwrap();
        assert_eq!((d.name.as_str(), d.last_seen), ("Garage", 9));
        assert_eq!(s.next_after("abc", "x", 600).unwrap(), None);
        s.set_next_after("abc", "x", 600, 1000).unwrap();
        s.set_next_after("abc", "x", 600, 500).unwrap();
        assert_eq!(s.next_after("abc", "x", 600).unwrap(), Some(1000));
        assert_eq!(s.next_after("abc", "x", 1800).unwrap(), None);
    }

    #[test]
    fn upload_history_newest_first() {
        let s = Store::open_memory().unwrap();
        for i in 0..3 {
            let u = UploadRecord { id: 0, device: "abc".into(), at: i, server: "x".into(), interval_s: 600, start: 0, bins: 10, accepted: 9, rejected: 1, credit: 2.5, ok: true, message: String::new() };
            s.record_upload(&u).unwrap();
        }
        let l = s.uploads("abc", 2).unwrap();
        assert_eq!(l.len(), 2);
        assert_eq!(l[0].at, 2);
    }

    #[test]
    fn a_file_database_reopens() {
        let dir = tempfile::tempdir().unwrap();
        let p = dir.path().join("hub.db");
        {
            let mut s = Store::open(&p).unwrap();
            s.insert_samples("a", &[Sample { time: 1_700_000_000, interval: 60, time_valid: true, value: 4 }]).unwrap();
        }
        assert_eq!(Store::open(&p).unwrap().sample_count("a").unwrap(), 1);
    }
}
