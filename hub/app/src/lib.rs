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
    s.account = new.account;
    s.country = new.country;
    s.city = new.city;
    s.save(&app.dir).map_err(err)?;
    Ok(s.clone())
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
