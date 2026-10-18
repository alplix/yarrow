//! Finding the drive the device shows in update mode.

use crate::firmware::Drive;

/// The update drive is a few hundred KB at most; anything bigger is somebody's USB stick.
const LIKELY_MAX_BYTES: u64 = 64 * 1024 * 1024;

#[cfg(windows)]
pub fn list() -> Vec<Drive> {
    use windows_sys::Win32::Storage::FileSystem::{
        GetDiskFreeSpaceExW, GetDriveTypeW, GetLogicalDrives, GetVolumeInformationW,
    };
    const DRIVE_REMOVABLE: u32 = 2;
    let mut v = Vec::new();
    // SAFETY: plain Win32 queries with valid, NUL-terminated buffers that outlive the calls.
    unsafe {
        let mask = GetLogicalDrives();
        for i in 0..26u32 {
            if mask & (1 << i) == 0 {
                continue;
            }
            let root: Vec<u16> = format!("{}:\\", (b'A' + i as u8) as char).encode_utf16().chain(Some(0)).collect();
            if GetDriveTypeW(root.as_ptr()) != DRIVE_REMOVABLE {
                continue;
            }
            let mut label = [0u16; 261];
            let ok = GetVolumeInformationW(
                root.as_ptr(),
                label.as_mut_ptr(),
                label.len() as u32,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                0,
            );
            if ok == 0 {
                continue; // no medium in the drive
            }
            let mut total = 0u64;
            GetDiskFreeSpaceExW(root.as_ptr(), std::ptr::null_mut(), &mut total, std::ptr::null_mut());
            let end = label.iter().position(|&c| c == 0).unwrap_or(0);
            v.push(Drive {
                path: format!("{}:\\", (b'A' + i as u8) as char),
                label: String::from_utf16_lossy(&label[..end]),
                size: total,
                likely: total > 0 && total <= LIKELY_MAX_BYTES,
            });
        }
    }
    v
}

#[cfg(not(windows))]
pub fn list() -> Vec<Drive> {
    use std::path::PathBuf;
    let mut roots: Vec<PathBuf> = Vec::new();
    if cfg!(target_os = "macos") {
        roots.push("/Volumes".into());
    } else {
        for user in ["USER", "LOGNAME"] {
            if let Ok(u) = std::env::var(user) {
                roots.push(format!("/media/{u}").into());
                roots.push(format!("/run/media/{u}").into());
            }
        }
        roots.push("/media".into());
    }
    let mut v = Vec::new();
    for r in roots {
        let Ok(rd) = std::fs::read_dir(&r) else { continue };
        for e in rd.flatten() {
            let p = e.path();
            if !p.is_dir() || p == PathBuf::from("/Volumes/Macintosh HD") {
                continue;
            }
            v.push(Drive {
                path: p.to_string_lossy().into(),
                label: e.file_name().to_string_lossy().into(),
                size: 0,
                // The size is not known here; the label of the device's drive is not either.
                likely: false,
            });
        }
    }
    v.dedup_by(|a, b| a.path == b.path);
    v
}
