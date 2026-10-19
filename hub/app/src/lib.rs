//! Yarrow Hub: the desktop program for the Yarrow radiation-detector firmware.

mod device;
mod drives;
mod firmware;
mod settings;
mod state;

use settings::Settings;
use state::{err, lock, App, Res};
use std::sync::Arc;
use tauri::{Manager, State};

#[tauri::command]
fn get_settings(app: State<'_, Arc<App>>) -> Settings {
    lock(&app.settings).clone()
}

/// Stores the settings the interface owns.
#[tauri::command]
fn save_settings(app: State<'_, Arc<App>>, new: Settings) -> Res<Settings> {
    let mut s = lock(&app.settings);
    s.language = new.language;
    s.theme = new.theme;
    s.fw_folder = new.fw_folder;
    s.unit = new.unit;
    s.alarm_caution = new.alarm_caution;
    s.alarm_danger = new.alarm_danger;
    s.alarm_sound = new.alarm_sound;
    s.auto_live = new.auto_live;
    s.record_live = new.record_live;
    s.save(&app.dir).map_err(err)?;
    Ok(s.clone())
}

/// Writes a text file the user chose in the save dialog (CSV exports).
#[tauri::command]
fn write_text_file(path: String, contents: String) -> Res<u64> {
    std::fs::write(&path, contents.as_bytes()).map_err(err)?;
    Ok(contents.len() as u64)
}

/// Writes a binary file the user chose in the save dialog (a chart as PNG); the data comes as base64.
#[tauri::command]
fn write_base64_file(path: String, data: String) -> Res<u64> {
    let bytes = decode_base64(&data).ok_or_else(|| "the picture data is not valid".to_string())?;
    std::fs::write(&path, &bytes).map_err(err)?;
    Ok(bytes.len() as u64)
}

fn decode_base64(s: &str) -> Option<Vec<u8>> {
    let mut out = Vec::with_capacity(s.len() / 4 * 3);
    let (mut acc, mut bits) = (0u32, 0u32);
    for c in s.bytes() {
        let v = match c {
            b'A'..=b'Z' => c - b'A',
            b'a'..=b'z' => c - b'a' + 26,
            b'0'..=b'9' => c - b'0' + 52,
            b'+' => 62,
            b'/' => 63,
            b'=' | b'\n' | b'\r' => continue,
            _ => return None,
        } as u32;
        acc = (acc << 6) | v;
        bits += 6;
        if bits >= 8 {
            bits -= 8;
            out.push((acc >> bits) as u8);
            acc &= (1 << bits) - 1;
        }
    }
    Some(out)
}

#[tauri::command]
fn about() -> serde_json::Value {
    serde_json::json!({ "version": env!("CARGO_PKG_VERSION"), "repo": "https://github.com/alplix/yarrow" })
}

pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_opener::init())
        .setup(|app| {
            let dir = app.path().app_data_dir()?;
            let a = App::new(dir)?;
            app.manage(Arc::new(a));
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            get_settings,
            save_settings,
            about,
            write_text_file,
            write_base64_file,
            device::ports,
            device::connect,
            device::disconnect,
            device::device_info,
            device::live_start,
            device::live_stop,
            device::download_log,
            device::devices,
            device::rename_device,
            device::device_data,
            device::series,
            device::export_csv,
            device::events,
            device::get_cal,
            device::set_cal,
            device::set_theme,
            device::set_clock,
            firmware::fw_scan,
            firmware::fw_default_folder,
            firmware::fw_releases,
            firmware::fw_download,
            firmware::fw_drives,
            firmware::fw_copy,
        ])
        .run(tauri::generate_context!())
        .expect("error while running Yarrow Hub");
}

#[cfg(test)]
mod tests {
    use super::decode_base64;

    #[test]
    fn base64_decodes() {
        assert_eq!(decode_base64("aGVsbG8=").unwrap(), b"hello");
        assert_eq!(decode_base64("aGVsbG8gd29ybGQ=").unwrap(), b"hello world");
        assert_eq!(decode_base64("").unwrap(), b"");
        assert!(decode_base64("@@@").is_none());
    }
}
