//! Shared state of the running application.

use crate::settings::Settings;
use std::path::PathBuf;
use std::sync::atomic::AtomicU64;
use std::sync::{Mutex, MutexGuard};
use yarrow_core::Detector;
use yarrow_store::Store;

pub type Res<T> = Result<T, String>;

pub fn err<E: std::fmt::Display>(e: E) -> String {
    e.to_string()
}

/// Locks a mutex even if a thread panicked while holding it: the data is plain state, not an invariant.
pub fn lock<T>(m: &Mutex<T>) -> MutexGuard<'_, T> {
    m.lock().unwrap_or_else(|e| e.into_inner())
}

/// Runs blocking work (serial port, network, database) off the thread that serves the window.
pub async fn blocking<T: Send + 'static>(f: impl FnOnce() -> Res<T> + Send + 'static) -> Res<T> {
    tauri::async_runtime::spawn_blocking(f).await.map_err(err)?
}

pub fn now() -> u32 {
    std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs() as u32)
        .unwrap_or(0)
}

/// A connected device.
pub struct Session {
    pub det: Box<dyn Detector>,
    /// Hash that stands for this device in the database.
    pub device_id: String,
}

pub struct App {
    pub dir: PathBuf,
    pub settings: Mutex<Settings>,
    pub store: Mutex<Store>,
    pub session: Mutex<Option<Session>>,
    /// Bumped to stop the live-stream thread.
    pub live_gen: AtomicU64,
}

impl App {
    pub fn new(dir: PathBuf) -> Res<App> {
        std::fs::create_dir_all(&dir).map_err(err)?;
        let settings = Settings::load(&dir);
        let store = Store::open(&dir.join("hub.db")).map_err(err)?;
        Ok(App {
            dir,
            settings: Mutex::new(settings),
            store: Mutex::new(store),
            session: Mutex::new(None),
            live_gen: AtomicU64::new(0),
        })
    }

}
