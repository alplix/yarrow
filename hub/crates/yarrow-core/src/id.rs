//! Stable device ids. The id that is stored and shown is a hash, never the raw chip id or serial number.

use sha2::{Digest, Sha256};

/// Hash of the device's unique id (or of the fallback id below).
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hashes_differ_and_hide_the_input() {
        let h = device_hash("abc");
        assert_eq!(h.len(), 64);
        assert!(!h.contains("abc"));
        assert_ne!(device_hash("abc"), device_hash("abd"));
        assert_ne!(fallback_id(0x1209, 1, Some("A")), fallback_id(0x1209, 1, Some("B")));
    }
}
