//! The firmware tab: find language files, fetch releases when the user asks, copy a file onto the update drive.

use crate::state::{blocking, err, lock, App, Res};
use serde::Serialize;
use std::path::{Path, PathBuf};
use std::sync::Arc;
use tauri::State;

type Ap<'a> = State<'a, Arc<App>>;

const REPO: &str = "alplix/yarrow";
const DOWNLOAD_PREFIX: &str = "https://github.com/alplix/yarrow/releases/download/";
/// The largest image any language needs (the CJK files use a 128 KB area).
const MAX_FIRMWARE_BYTES: u64 = 160 * 1024;

#[derive(Debug, Serialize, Clone, PartialEq, Eq)]
pub struct FirmwareFile {
    pub name: String,
    pub lang: String,
    pub size: u64,
    pub path: String,
}

/// `yarrow-gc03-<lang>.bin` gives `<lang>`.
pub fn lang_of(name: &str) -> Option<&str> {
    let l = name.strip_prefix("yarrow-gc03-")?.strip_suffix(".bin")?;
    (!l.is_empty() && l.len() <= 8 && l.chars().all(|c| c.is_ascii_lowercase() || c == '-')).then_some(l)
}

pub fn scan(dir: &Path) -> std::io::Result<Vec<FirmwareFile>> {
    let mut v = Vec::new();
    for e in std::fs::read_dir(dir)? {
        let e = e?;
        let name = e.file_name().to_string_lossy().to_string();
        if let Some(lang) = lang_of(&name) {
            if e.metadata()?.is_file() {
                v.push(FirmwareFile { lang: lang.into(), size: e.metadata()?.len(), path: e.path().to_string_lossy().into(), name });
            }
        }
    }
    v.sort_by(|a, b| a.lang.cmp(&b.lang));
    Ok(v)
}

#[tauri::command]
pub async fn fw_scan(dir: String) -> Res<Vec<FirmwareFile>> {
    blocking(move || scan(Path::new(&dir)).map_err(err)).await
}

/// The folder downloads go to when the user has not chosen one.
#[tauri::command]
pub fn fw_default_folder(app: Ap<'_>) -> Res<String> {
    let s = lock(&app.settings);
    if !s.fw_folder.is_empty() {
        return Ok(s.fw_folder.clone());
    }
    let p = app.dir.join("firmware");
    std::fs::create_dir_all(&p).map_err(err)?;
    Ok(p.to_string_lossy().into())
}

#[derive(Serialize)]
pub struct Asset {
    name: String,
    lang: String,
    size: u64,
    url: String,
}

#[derive(Serialize)]
pub struct Release {
    tag: String,
    name: String,
    published: String,
    url: String,
    assets: Vec<Asset>,
}

/// Lists the releases of the project on GitHub. Only called when the user clicks.
#[tauri::command]
pub async fn fw_releases() -> Res<Vec<Release>> {
    blocking(|| {
        let r = ureq::get(&format!("https://api.github.com/repos/{REPO}/releases"))
            .set("User-Agent", "yarrow-hub")
            .set("Accept", "application/vnd.github+json")
            .timeout(std::time::Duration::from_secs(20))
            .call()
            .map_err(|e| format!("could not reach GitHub: {e}"))?;
        let v: serde_json::Value = r.into_json().map_err(err)?;
        let mut out = Vec::new();
        for rel in v.as_array().ok_or("unexpected answer from GitHub")? {
            let assets = rel["assets"]
                .as_array()
                .map(|a| {
                    a.iter()
                        .filter_map(|x| {
                            let name = x["name"].as_str()?;
                            let lang = lang_of(name)?;
                            let url = x["browser_download_url"].as_str()?;
                            url.starts_with(DOWNLOAD_PREFIX).then(|| Asset {
                                name: name.into(),
                                lang: lang.into(),
                                size: x["size"].as_u64().unwrap_or(0),
                                url: url.into(),
                            })
                        })
                        .collect::<Vec<_>>()
                })
                .unwrap_or_default();
            out.push(Release {
                tag: rel["tag_name"].as_str().unwrap_or("").into(),
                name: rel["name"].as_str().unwrap_or("").into(),
                published: rel["published_at"].as_str().unwrap_or("").into(),
                url: rel["html_url"].as_str().unwrap_or("").into(),
                assets,
            });
        }
        Ok(out)
    })
    .await
}

/// Downloads one release file into `dir`.
#[tauri::command]
pub async fn fw_download(url: String, dir: String) -> Res<FirmwareFile> {
    blocking(move || {
        if !url.starts_with(DOWNLOAD_PREFIX) {
            return Err("not a release file of the project".to_string());
        }
        let name = url.rsplit('/').next().unwrap_or("");
        let lang = lang_of(name).ok_or("not a firmware file")?.to_string();
        let r = ureq::get(&url)
            .set("User-Agent", "yarrow-hub")
            .timeout(std::time::Duration::from_secs(120))
            .call()
            .map_err(|e| format!("download failed: {e}"))?;
        let mut data = Vec::new();
        std::io::Read::read_to_end(&mut std::io::Read::take(r.into_reader(), MAX_FIRMWARE_BYTES + 1), &mut data)
            .map_err(err)?;
        if data.is_empty() || data.len() as u64 > MAX_FIRMWARE_BYTES {
            return Err("the file has an unexpected size".into());
        }
        std::fs::create_dir_all(&dir).map_err(err)?;
        let path = PathBuf::from(&dir).join(name);
        std::fs::write(&path, &data).map_err(err)?;
        Ok(FirmwareFile { name: name.into(), lang, size: data.len() as u64, path: path.to_string_lossy().into() })
    })
    .await
}

#[derive(Serialize, Clone, Debug, PartialEq, Eq)]
pub struct Drive {
    pub path: String,
    pub label: String,
    pub size: u64,
    /// A small removable volume: what the device's update mode looks like.
    pub likely: bool,
}

/// Removable drives that are mounted right now.
#[tauri::command]
pub async fn fw_drives() -> Res<Vec<Drive>> {
    blocking(|| Ok(crate::drives::list())).await
}

/// Copies a firmware file onto a drive that [`fw_drives`] listed.
#[tauri::command]
pub async fn fw_copy(file: String, drive: String) -> Res<String> {
    blocking(move || {
        let src = PathBuf::from(&file);
        let name = src.file_name().and_then(|n| n.to_str()).unwrap_or("").to_string();
        lang_of(&name).ok_or("only yarrow-gc03-<language>.bin files can be copied")?;
        if !crate::drives::list().iter().any(|d| d.path == drive) {
            return Err("that drive is not mounted any more".into());
        }
        let data = std::fs::read(&src).map_err(err)?;
        if data.is_empty() || data.len() as u64 > MAX_FIRMWARE_BYTES {
            return Err("the file has an unexpected size".into());
        }
        let dest = PathBuf::from(&drive).join(&name);
        std::fs::write(&dest, &data).map_err(err)?;
        Ok(dest.to_string_lossy().into())
    })
    .await
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn language_files_are_recognised() {
        assert_eq!(lang_of("yarrow-gc03-tr.bin"), Some("tr"));
        assert_eq!(lang_of("yarrow-gc03-zh-hans.bin"), Some("zh-hans"));
        assert_eq!(lang_of("yarrow-gc03.bin"), None);
        assert_eq!(lang_of("yarrow-gc03-../x.bin"), None);
        assert_eq!(lang_of("other.bin"), None);
    }

    #[test]
    fn scan_lists_only_firmware_files() {
        let dir = std::env::temp_dir().join(format!("yarrow-fw-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        std::fs::write(dir.join("yarrow-gc03-tr.bin"), [1u8; 10]).unwrap();
        std::fs::write(dir.join("yarrow-gc03-en.bin"), [1u8; 20]).unwrap();
        std::fs::write(dir.join("notes.txt"), "x").unwrap();
        let v = scan(&dir).unwrap();
        assert_eq!(v.iter().map(|f| f.lang.as_str()).collect::<Vec<_>>(), ["en", "tr"]);
        assert_eq!(v[1].size, 10);
        let _ = std::fs::remove_dir_all(&dir);
    }
}
