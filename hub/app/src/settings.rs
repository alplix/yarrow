//! User settings, kept as one JSON file in the application data folder. Nothing secret lives here.

use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(default)]
pub struct Settings {
    /// Interface language code; empty follows the computer.
    pub language: String,
    /// "system", "dark" or "light".
    pub theme: String,
    /// Reserved for a later BOINC monitor: the account name and the place (country code and city) the user picks.
    /// Nothing reads or sends them yet.
    pub account: String,
    pub country: String,
    pub city: String,
    pub fw_folder: String,
    pub last_port: String,
    pub selected_device: String,
}

impl Default for Settings {
    fn default() -> Self {
        Settings {
            language: String::new(),
            theme: "system".into(),
            account: String::new(),
            country: String::new(),
            city: String::new(),
            fw_folder: String::new(),
            last_port: String::new(),
            selected_device: String::new(),
        }
    }
}

impl Settings {
    fn path(dir: &Path) -> PathBuf {
        dir.join("settings.json")
    }

    pub fn load(dir: &Path) -> Settings {
        std::fs::read_to_string(Self::path(dir)).ok().and_then(|s| serde_json::from_str(&s).ok()).unwrap_or_default()
    }

    pub fn save(&self, dir: &Path) -> std::io::Result<()> {
        let tmp = Self::path(dir).with_extension("json.tmp");
        std::fs::write(&tmp, serde_json::to_string_pretty(self)?)?;
        std::fs::rename(&tmp, Self::path(dir))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn settings_round_trip() {
        let s = Settings::default();
        assert_eq!(s.theme, "system");
        let dir = std::env::temp_dir().join(format!("yarrow-settings-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let t = Settings { language: "tr".into(), city: "Denizli".into(), ..Settings::default() };
        t.save(&dir).unwrap();
        assert_eq!(Settings::load(&dir), t);
        // An older or damaged file falls back to the defaults.
        std::fs::write(dir.join("settings.json"), "{\"language\":\"de\",\"future\":1}").unwrap();
        assert_eq!(Settings::load(&dir).language, "de");
        std::fs::write(dir.join("settings.json"), "nonsense").unwrap();
        assert_eq!(Settings::load(&dir), Settings::default());
        let _ = std::fs::remove_dir_all(&dir);
    }
}
