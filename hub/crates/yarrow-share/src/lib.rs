//! Opt-in sharing of measurements with a project server, see `docs/upload-api.md`.
//!
//! Nothing here runs unless the user starts it. The password is only ever used to compute the hash BOINC wants and
//! is dropped at once; the account key goes to the operating system's credential store.

pub mod boinc;
pub mod creds;
pub mod geo;
pub mod mock;
pub mod upload;

use sha2::{Digest, Sha256};

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("network: {0}")]
    Network(String),
    #[error("{0}")]
    Server(String),
    #[error("{0}")]
    Invalid(String),
    #[error("credential store: {0}")]
    Credentials(String),
}

pub type Result<T> = std::result::Result<T, Error>;

/// The default project the Hub offers.
pub const DEFAULT_SERVER: &str = "https://bitboinc.athena.org.tr";

/// A server URL must use https; plain http is only accepted for this computer (tests, development).
pub fn check_server_url(url: &str) -> Result<String> {
    let url = url.trim().trim_end_matches('/');
    let rest = if let Some(r) = url.strip_prefix("https://") {
        r
    } else if let Some(r) = url.strip_prefix("http://") {
        let host = r.split(['/', ':']).next().unwrap_or("");
        let host = if r.starts_with('[') { "[::1]" } else { host };
        if !matches!(host, "localhost" | "127.0.0.1" | "[::1]") {
            return Err(Error::Invalid("the server address must start with https://".into()));
        }
        r
    } else {
        return Err(Error::Invalid("the server address must start with https://".into()));
    };
    if rest.is_empty() || rest.contains(char::is_whitespace) {
        return Err(Error::Invalid("not a server address".into()));
    }
    Ok(url.to_string())
}

/// The id the server sees for a device: a hash, never the id itself. `unique_id` is the chip id when the firmware
/// reports one; otherwise the Hub falls back to the identity of the USB serial port (see [`fallback_id`]).
pub fn device_hash(unique_id: &str) -> String {
    let mut h = Sha256::new();
    h.update(b"yarrow-device-v1:");
    h.update(unique_id.as_bytes());
    hex::encode(h.finalize())
}

/// Identity used while the firmware has no unique id of its own: the USB vendor, product and serial number of the
/// port. Devices without a USB serial number cannot be told apart this way.
pub fn fallback_id(vid: u16, pid: u16, serial_number: Option<&str>) -> String {
    format!("usb:{vid:04x}:{pid:04x}:{}", serial_number.unwrap_or(""))
}
