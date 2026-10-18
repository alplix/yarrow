//! Where the account key lives: the operating system's credential store.

use crate::{Error, Result};
use std::collections::HashMap;
use std::sync::Mutex;

const SERVICE: &str = "yarrow-hub";

/// A place to keep one secret per project URL.
pub trait KeyStore: Send + Sync {
    fn get(&self, server: &str) -> Result<Option<String>>;
    fn set(&self, server: &str, key: &str) -> Result<()>;
    fn delete(&self, server: &str) -> Result<()>;
}

/// The credential store of the operating system (Windows Credential Manager, macOS Keychain, Secret Service).
pub struct OsKeyStore;

impl OsKeyStore {
    fn entry(server: &str) -> Result<keyring::Entry> {
        keyring::Entry::new(SERVICE, server).map_err(|e| Error::Credentials(e.to_string()))
    }
}

impl KeyStore for OsKeyStore {
    fn get(&self, server: &str) -> Result<Option<String>> {
        match Self::entry(server)?.get_password() {
            Ok(k) => Ok(Some(k)),
            Err(keyring::Error::NoEntry) => Ok(None),
            Err(e) => Err(Error::Credentials(e.to_string())),
        }
    }

    fn set(&self, server: &str, key: &str) -> Result<()> {
        Self::entry(server)?.set_password(key).map_err(|e| Error::Credentials(e.to_string()))
    }

    fn delete(&self, server: &str) -> Result<()> {
        match Self::entry(server)?.delete_credential() {
            Ok(()) | Err(keyring::Error::NoEntry) => Ok(()),
            Err(e) => Err(Error::Credentials(e.to_string())),
        }
    }
}

/// In-memory store for tests.
#[derive(Default)]
pub struct MemoryKeyStore(Mutex<HashMap<String, String>>);

impl KeyStore for MemoryKeyStore {
    fn get(&self, server: &str) -> Result<Option<String>> {
        Ok(self.0.lock().unwrap().get(server).cloned())
    }

    fn set(&self, server: &str, key: &str) -> Result<()> {
        self.0.lock().unwrap().insert(server.to_string(), key.to_string());
        Ok(())
    }

    fn delete(&self, server: &str) -> Result<()> {
        self.0.lock().unwrap().remove(server);
        Ok(())
    }
}
