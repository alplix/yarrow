use yarrow_core::bins::bin_range;
use yarrow_core::fake::{DEMO_IMAGE, DEMO_PAGE_SIZE};
use yarrow_core::{decode_image, Sample};
use yarrow_share::creds::{KeyStore, MemoryKeyStore};
use yarrow_share::mock::MockServer;
use yarrow_share::upload::{self, DeviceMeta, Location, MAX_BINS_PER_REQUEST};
use yarrow_share::{boinc, check_server_url, device_hash, fallback_id};

fn meta() -> DeviceMeta {
    DeviceMeta { uid_hash: device_hash(&fallback_id(0x1209, 1, Some("SN1"))), firmware: "0.1.0".into(), lang: "tr".into(), sensitivity_x100: 8000, dead_time_us: 90 }
}

fn loc() -> Location {
    Location { country: "TR".into(), city: "Denizli".into() }
}

fn samples() -> Vec<Sample> {
    decode_image(DEMO_IMAGE, DEMO_PAGE_SIZE)
}

#[test]
fn account_login_and_creation_against_the_mock() {
    let srv = MockServer::start();
    srv.add_account("Me@Example.org", "pw-123456", "key-1");
    assert_eq!(boinc::login(&srv.url, "me@example.org", "pw-123456").unwrap(), "key-1");
    let bad = boinc::login(&srv.url, "me@example.org", "wrong").unwrap_err().to_string();
    assert_eq!(bad, "Bad password");
    let k = boinc::create_account(&srv.url, "new@example.org", "pw-123456", "Newbie").unwrap();
    assert_eq!(boinc::login(&srv.url, "new@example.org", "pw-123456").unwrap(), k);
    assert!(boinc::create_account(&srv.url, "new@example.org", "pw-123456", "Again").unwrap_err().to_string().contains("taken"));
    assert!(boinc::create_account(&srv.url, "x@example.org", "short", "X").is_err());
}

#[test]
fn the_account_key_goes_to_the_key_store_and_not_the_password() {
    let store = MemoryKeyStore::default();
    let srv = MockServer::start();
    srv.add_account("a@b.c", "pw-123456", "the-key");
    let key = boinc::login(&srv.url, "a@b.c", "pw-123456").unwrap();
    store.set(&srv.url, &key).unwrap();
    assert_eq!(store.get(&srv.url).unwrap().as_deref(), Some("the-key"));
    store.delete(&srv.url).unwrap();
    assert_eq!(store.get(&srv.url).unwrap(), None);
}

#[test]
fn server_addresses_must_be_https_except_on_this_computer() {
    assert!(check_server_url("https://bitboinc.athena.org.tr/").is_ok());
    assert!(check_server_url("http://localhost:8787").is_ok());
    assert!(check_server_url("http://127.0.0.1:1").is_ok());
    assert!(check_server_url("http://example.org").is_err());
    assert!(check_server_url("ftp://example.org").is_err());
    assert!(check_server_url("https://").is_err());
}

#[test]
fn device_hash_is_a_hash() {
    let h = device_hash("abc");
    assert_eq!(h.len(), 64);
    assert!(!h.contains("abc"));
    assert_ne!(device_hash("abc"), device_hash("abd"));
    assert_ne!(fallback_id(0x1209, 1, Some("A")), fallback_id(0x1209, 1, Some("B")));
}

#[test]
fn plan_makes_aligned_chunks_without_edge_gaps() {
    let s = samples();
    let end = s.last().unwrap().time + 60;
    let chunks = upload::plan(&s, 600, None, end + 10 * 86400).unwrap();
    assert!(!chunks.is_empty());
    for c in &chunks {
        assert_eq!(c.start % 600, 0);
        assert!(c.bins.len() <= MAX_BINS_PER_REQUEST);
        assert!(c.bins.first().unwrap().is_some() && c.bins.last().unwrap().is_some());
    }
    // Every complete bin of the log is in exactly one chunk.
    let total: usize = chunks.iter().map(|c| c.bins.iter().flatten().count()).sum();
    let (_, all) = bin_range(&s, 600, s[0].time, end);
    assert_eq!(total, all.iter().flatten().count());
    // Continuing after the end of the first chunk skips what was sent.
    let after = chunks[0].end(600);
    let rest = upload::plan(&s, 600, Some(after), end + 10 * 86400).unwrap();
    assert!(rest.iter().all(|c| c.start >= after));
    // Nothing is sent that is newer than the settle time.
    let early = upload::plan(&s, 600, None, s[0].time + 600).unwrap();
    assert!(early.is_empty());
    assert!(upload::plan(&s, 123, None, end).is_err());
}

#[test]
fn full_upload_round_trip_with_continuation() {
    let srv = MockServer::start();
    srv.add_account("a@b.c", "pw-123456", "the-key");
    let s = samples();
    let now = s.last().unwrap().time + 5 * 86400;
    srv.state.lock().unwrap().now = Some(now);
    let chunks = upload::plan(&s, 600, None, now).unwrap();
    let mut accepted_total = 0;
    let mut cursor = None;
    for c in &chunks {
        let b = upload::body("the-key", &meta(), &loc(), 600, c);
        let r = upload::send(&srv.url, &b).unwrap();
        assert!(r.ok, "{}", r.describe());
        assert_eq!(r.rejected, 0);
        accepted_total += r.accepted;
        cursor = upload::next_cursor(c, 600, &r);
    }
    let expected: usize = chunks.iter().map(|c| c.bins.iter().flatten().count()).sum();
    assert_eq!(accepted_total as usize, expected);
    assert_eq!(cursor, Some(chunks.last().unwrap().end(600)));
    // The server now has them; sending again is rejected as duplicates and the cursor still moves on.
    let again = upload::send(&srv.url, &upload::body("the-key", &meta(), &loc(), 600, &chunks[0])).unwrap();
    assert_eq!((again.accepted, again.rejected as usize), (0, chunks[0].bins.iter().flatten().count()));
    assert!(upload::next_cursor(&chunks[0], 600, &again).unwrap() >= chunks[0].end(600));
    // What the server saw is exactly the contract: no extra fields, no password, no coordinates.
    let seen = srv.state.lock().unwrap().uploads[0].clone();
    let mut keys: Vec<&str> = seen.as_object().unwrap().keys().map(String::as_str).collect();
    keys.sort();
    assert_eq!(keys, ["account_key", "bins", "device", "interval_s", "location", "start"]);
    let mut dk: Vec<&str> = seen["device"].as_object().unwrap().keys().map(String::as_str).collect();
    dk.sort();
    assert_eq!(dk, ["dead_time_us", "firmware", "lang", "sensitivity_x100", "uid_hash"]);
    assert_eq!(seen["location"], serde_json::json!({"country": "TR", "city": "Denizli"}));
}

#[test]
fn server_errors_come_back_as_replies() {
    let srv = MockServer::start();
    srv.add_account("a@b.c", "pw-123456", "the-key");
    let s = samples();
    let now = s.last().unwrap().time + 5 * 86400;
    let chunks = upload::plan(&s, 600, None, now).unwrap();
    let r = upload::send(&srv.url, &upload::body("wrong", &meta(), &loc(), 600, &chunks[0])).unwrap();
    assert!(!r.ok);
    assert_eq!(r.error.as_deref(), Some("bad_account"));
    assert_eq!(upload::next_cursor(&chunks[0], 600, &r), None);
    srv.state.lock().unwrap().fail_next = Some("rate_limited".into());
    let r = upload::send(&srv.url, &upload::body("the-key", &meta(), &loc(), 600, &chunks[0])).unwrap();
    assert_eq!(r.error.as_deref(), Some("rate_limited"));
    // A server that is not there is a network error, not a panic.
    assert!(upload::send("http://127.0.0.1:1", &upload::body("k", &meta(), &loc(), 600, &chunks[0])).is_err());
}

#[test]
fn preview_shows_the_request_without_the_key() {
    let s = samples();
    let now = s.last().unwrap().time + 5 * 86400;
    let chunks = upload::plan(&s, 1800, None, now).unwrap();
    let p = upload::preview("https://x.example", &meta(), &loc(), 1800, &chunks);
    assert_eq!(p.requests, chunks.len());
    assert!(p.first_request.contains("\"city\": \"Denizli\""));
    assert!(!p.first_request.contains("the-key"));
    assert!(p.first_request.contains(upload::KEY_PLACEHOLDER));
    assert!(p.pulses > 0 && p.bins > 0);
}
