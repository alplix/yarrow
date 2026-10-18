//! A stand-in project server that follows `docs/upload-api.md` and the BOINC account RPCs, for tests and for trying
//! the Hub before the real endpoint exists:
//!
//! ```text
//! cargo run -p yarrow-share --example mock_server
//! ```

use crate::boinc::passwd_hash;
use serde_json::{json, Value};
use std::collections::{BTreeSet, HashMap};
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{Arc, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Default)]
pub struct MockState {
    /// email -> (passwd_hash, account key)
    pub accounts: HashMap<String, (String, String)>,
    /// (device hash, interval) -> start times of the bins already received
    pub received: HashMap<(String, u32), BTreeSet<u32>>,
    /// Every upload body that reached the server.
    pub uploads: Vec<Value>,
    /// Fail the next upload with this error code.
    pub fail_next: Option<String>,
    /// Pretend the clock of the server is here (default: the real clock).
    pub now: Option<u32>,
}

pub struct MockServer {
    pub url: String,
    pub state: Arc<Mutex<MockState>>,
}

impl MockServer {
    /// Starts listening on a free port of this computer; the server thread lives as long as the process.
    pub fn start() -> MockServer {
        Self::start_on("127.0.0.1:0")
    }

    pub fn start_on(addr: &str) -> MockServer {
        let listener = TcpListener::bind(addr).expect("bind mock server");
        let url = format!("http://{}", listener.local_addr().unwrap());
        let state = Arc::new(Mutex::new(MockState::default()));
        let st = state.clone();
        std::thread::spawn(move || {
            for conn in listener.incoming().flatten() {
                let st = st.clone();
                std::thread::spawn(move || {
                    let _ = handle(conn, &st);
                });
            }
        });
        MockServer { url, state }
    }

    /// Adds an account the RPCs will know.
    pub fn add_account(&self, email: &str, password: &str, key: &str) {
        self.state
            .lock()
            .unwrap()
            .accounts
            .insert(email.to_lowercase(), (passwd_hash(password, email), key.to_string()));
    }
}

fn read_request(s: &mut TcpStream) -> std::io::Result<(String, String, Vec<u8>)> {
    let mut buf = Vec::new();
    let mut chunk = [0u8; 4096];
    let head_end = loop {
        let n = s.read(&mut chunk)?;
        if n == 0 {
            return Err(std::io::ErrorKind::UnexpectedEof.into());
        }
        buf.extend_from_slice(&chunk[..n]);
        if let Some(p) = buf.windows(4).position(|w| w == b"\r\n\r\n") {
            break p + 4;
        }
    };
    let head = String::from_utf8_lossy(&buf[..head_end]).to_string();
    let mut lines = head.lines();
    let first = lines.next().unwrap_or("");
    let mut parts = first.split_whitespace();
    let method = parts.next().unwrap_or("").to_string();
    let path = parts.next().unwrap_or("").to_string();
    let len = lines
        .filter_map(|l| l.split_once(':'))
        .find(|(k, _)| k.eq_ignore_ascii_case("content-length"))
        .and_then(|(_, v)| v.trim().parse::<usize>().ok())
        .unwrap_or(0);
    let mut body = buf[head_end..].to_vec();
    while body.len() < len {
        let n = s.read(&mut chunk)?;
        if n == 0 {
            break;
        }
        body.extend_from_slice(&chunk[..n]);
    }
    Ok((method, path, body))
}

fn respond(s: &mut TcpStream, status: &str, ctype: &str, body: &str) -> std::io::Result<()> {
    write!(
        s,
        "HTTP/1.1 {status}\r\nContent-Type: {ctype}\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{body}",
        body.len()
    )?;
    s.flush()
}

fn form(body: &[u8]) -> HashMap<String, String> {
    fn dec(s: &str) -> String {
        let b = s.replace('+', " ");
        let bytes = b.as_bytes();
        let mut out = Vec::new();
        let mut i = 0;
        while i < bytes.len() {
            if bytes[i] == b'%' && i + 2 < bytes.len() + 0 {
                if let Ok(v) = u8::from_str_radix(&b[i + 1..i + 3], 16) {
                    out.push(v);
                    i += 3;
                    continue;
                }
            }
            out.push(bytes[i]);
            i += 1;
        }
        String::from_utf8_lossy(&out).to_string()
    }
    String::from_utf8_lossy(body)
        .split('&')
        .filter_map(|kv| kv.split_once('='))
        .map(|(k, v)| (dec(k), dec(v)))
        .collect()
}

fn account_xml(r: Result<&str, (i32, &str)>) -> String {
    match r {
        Ok(k) => format!("<account_out>\n<authenticator>{k}</authenticator>\n</account_out>\n"),
        Err((n, m)) => format!("<account_out>\n<error_num>{n}</error_num>\n<error_msg>{m}</error_msg>\n</account_out>\n"),
    }
}

fn err_json(code: &str, msg: &str) -> String {
    json!({"ok": false, "error": code, "message": msg}).to_string()
}

fn handle(mut s: TcpStream, st: &Mutex<MockState>) -> std::io::Result<()> {
    let (method, path, body) = read_request(&mut s)?;
    if method != "POST" {
        return respond(&mut s, "405 Method Not Allowed", "text/plain", "POST only");
    }
    let mut state = st.lock().unwrap();
    match path.as_str() {
        "/lookup_account.php" => {
            let f = form(&body);
            let email = f.get("email_addr").map(|e| e.to_lowercase()).unwrap_or_default();
            let hash = f.get("passwd_hash").cloned().unwrap_or_default();
            let x = match state.accounts.get(&email) {
                None => account_xml(Err((-136, "No such user"))),
                Some((h, _)) if *h != hash => account_xml(Err((-206, "Bad password"))),
                Some((_, k)) => account_xml(Ok(k)),
            };
            respond(&mut s, "200 OK", "text/xml", &x)
        }
        "/create_account.php" => {
            let f = form(&body);
            let email = f.get("email_addr").map(|e| e.to_lowercase()).unwrap_or_default();
            let hash = f.get("passwd_hash").cloned().unwrap_or_default();
            let name = f.get("user_name").cloned().unwrap_or_default();
            let x = if email.is_empty() || hash.is_empty() || name.is_empty() {
                account_xml(Err((-1, "missing field")))
            } else if state.accounts.contains_key(&email) {
                account_xml(Err((-137, "Email address already taken")))
            } else {
                let key = format!("mock-key-{}", state.accounts.len() + 1);
                state.accounts.insert(email, (hash, key.clone()));
                account_xml(Ok(&key))
            };
            respond(&mut s, "200 OK", "text/xml", &x)
        }
        "/yarrow/api/v1/upload" => {
            let reply = upload(&mut state, &body);
            respond(&mut s, "200 OK", "application/json", &reply)
        }
        _ => respond(&mut s, "404 Not Found", "text/plain", "not found"),
    }
}

fn upload(state: &mut MockState, body: &[u8]) -> String {
    let Ok(v) = serde_json::from_slice::<Value>(body) else {
        return err_json("bad_data", "not JSON");
    };
    if let Some(code) = state.fail_next.take() {
        return err_json(&code, "forced by the test");
    }
    let key = v["account_key"].as_str().unwrap_or("");
    if !state.accounts.values().any(|(_, k)| k == key) {
        return err_json("bad_account", "unknown account key");
    }
    let (Some(interval), Some(start), Some(bins), Some(uid)) = (
        v["interval_s"].as_u64().map(|x| x as u32),
        v["start"].as_u64().map(|x| x as u32),
        v["bins"].as_array(),
        v["device"]["uid_hash"].as_str(),
    ) else {
        return err_json("bad_data", "missing field");
    };
    if ![600, 1800, 3600].contains(&interval) || start % interval != 0 {
        return err_json("bad_data", "interval or start not valid");
    }
    if bins.len() > 1008 {
        return err_json("too_large", "at most 1008 bins");
    }
    if v["location"]["country"].as_str().map_or(0, str::len) != 2 || v["location"]["city"].as_str().unwrap_or("").is_empty() {
        return err_json("bad_data", "location");
    }
    let now = state.now.unwrap_or_else(|| SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs() as u32);
    let have = state.received.entry((uid.to_string(), interval)).or_default();
    let (mut accepted, mut rejected, mut newest) = (0u32, 0u32, have.iter().next_back().map_or(0, |s| s + interval));
    for (i, b) in bins.iter().enumerate() {
        let Some(c) = b.as_u64() else { continue };
        let t = start + i as u32 * interval;
        // A count above what the tube can deliver in the interval (about 5000 pulses a second) is impossible.
        let possible = c <= interval as u64 * 5000;
        if t + interval > now || have.contains(&t) || !possible {
            rejected += 1;
        } else {
            have.insert(t);
            accepted += 1;
            newest = newest.max(t + interval);
        }
    }
    state.uploads.push(v.clone());
    json!({"ok": true, "accepted": accepted, "rejected": rejected, "credit": accepted as f64 * 0.3, "next_after": newest})
        .to_string()
}
