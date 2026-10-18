//! The client side of `docs/upload-api.md`: planning what to send, the preview, and the upload itself.

use crate::{check_server_url, Error, Result};
use serde::{Deserialize, Serialize};
use std::time::Duration;
use yarrow_core::bins::{bin_range, ALLOWED_BIN_SECONDS};
use yarrow_core::Sample;

/// The most bins one request may carry (one week of ten-minute bins).
pub const MAX_BINS_PER_REQUEST: usize = 1008;
/// Path of the upload endpoint, relative to the project URL.
pub const UPLOAD_PATH: &str = "/yarrow/api/v1/upload";
/// A bin is only sent once it is this much in the past, so a late device clock cannot put it in the future.
pub const SETTLE_SECONDS: u32 = 3600;

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct DeviceMeta {
    pub uid_hash: String,
    pub firmware: String,
    pub lang: String,
    pub sensitivity_x100: u32,
    pub dead_time_us: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Location {
    pub country: String,
    pub city: String,
}

/// The request body, field for field as `docs/upload-api.md` lists it.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct UploadBody {
    pub account_key: String,
    pub device: DeviceMeta,
    pub location: Location,
    pub interval_s: u32,
    pub start: u32,
    pub bins: Vec<Option<u32>>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Default)]
pub struct Reply {
    pub ok: bool,
    #[serde(default)]
    pub accepted: u32,
    #[serde(default)]
    pub rejected: u32,
    #[serde(default)]
    pub credit: f64,
    #[serde(default)]
    pub next_after: u32,
    #[serde(default)]
    pub error: Option<String>,
    #[serde(default)]
    pub message: Option<String>,
}

impl Reply {
    pub fn describe(&self) -> String {
        match (&self.error, &self.message) {
            (Some(e), Some(m)) => format!("{e}: {m}"),
            (Some(e), None) => e.clone(),
            (None, Some(m)) => m.clone(),
            _ => String::new(),
        }
    }
}

/// A run of consecutive bins that goes into one request.
#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct Chunk {
    pub start: u32,
    pub bins: Vec<Option<u32>>,
}

impl Chunk {
    pub fn end(&self, interval_s: u32) -> u32 {
        self.start + self.bins.len() as u32 * interval_s
    }
}

/// Cuts the log into the requests that still have to be sent: bins after `after` (the end of the last accepted bin)
/// up to the last bin that is complete and at least [`SETTLE_SECONDS`] old. Gaps stay as `None`, a chunk never starts
/// or ends with one, and a chunk is at most [`MAX_BINS_PER_REQUEST`] bins long.
pub fn plan(samples: &[Sample], interval_s: u32, after: Option<u32>, now: u32) -> Result<Vec<Chunk>> {
    if !ALLOWED_BIN_SECONDS.contains(&interval_s) {
        return Err(Error::Invalid("the bin length must be 10 minutes, 30 minutes or 1 hour".into()));
    }
    let Some(first_sample) = samples.iter().filter(|s| s.time_valid).map(|s| s.time).min() else {
        return Ok(Vec::new());
    };
    let from = after.map_or(first_sample, |a| a.max(first_sample));
    let to = now.saturating_sub(SETTLE_SECONDS);
    let (grid_start, bins) = bin_range(samples, interval_s, from, to);
    let mut chunks = Vec::new();
    let mut i = 0;
    while i < bins.len() {
        if bins[i].is_none() {
            i += 1;
            continue;
        }
        let mut j = (i + MAX_BINS_PER_REQUEST).min(bins.len());
        while bins[j - 1].is_none() {
            j -= 1;
        }
        chunks.push(Chunk { start: grid_start + i as u32 * interval_s, bins: bins[i..j].to_vec() });
        i = j;
    }
    Ok(chunks)
}

/// What the user sees before anything leaves the computer.
#[derive(Debug, Clone, Serialize)]
pub struct Preview {
    pub server: String,
    pub interval_s: u32,
    pub requests: usize,
    pub bins: usize,
    pub gaps: usize,
    pub first: u32,
    pub end: u32,
    pub pulses: u64,
    /// The first request exactly as it will be sent, with the account key hidden.
    pub first_request: String,
}

pub const KEY_PLACEHOLDER: &str = "(your account key, kept in the system credential store)";

pub fn body(key: &str, device: &DeviceMeta, loc: &Location, interval_s: u32, chunk: &Chunk) -> UploadBody {
    UploadBody {
        account_key: key.to_string(),
        device: device.clone(),
        location: loc.clone(),
        interval_s,
        start: chunk.start,
        bins: chunk.bins.clone(),
    }
}

pub fn preview(
    server: &str,
    device: &DeviceMeta,
    loc: &Location,
    interval_s: u32,
    chunks: &[Chunk],
) -> Preview {
    let all: Vec<&Option<u32>> = chunks.iter().flat_map(|c| c.bins.iter()).collect();
    let first_request = chunks
        .first()
        .map(|c| serde_json::to_string_pretty(&body(KEY_PLACEHOLDER, device, loc, interval_s, c)).unwrap_or_default())
        .unwrap_or_default();
    Preview {
        server: server.to_string(),
        interval_s,
        requests: chunks.len(),
        bins: all.iter().filter(|b| b.is_some()).count(),
        gaps: all.iter().filter(|b| b.is_none()).count(),
        first: chunks.first().map_or(0, |c| c.start),
        end: chunks.last().map_or(0, |c| c.end(interval_s)),
        pulses: all.iter().filter_map(|b| **b).map(u64::from).sum(),
        first_request,
    }
}

/// Sends one request.
pub fn send(server: &str, b: &UploadBody) -> Result<Reply> {
    let server = check_server_url(server)?;
    let agent = ureq::AgentBuilder::new().timeout(Duration::from_secs(60)).build();
    let res = agent.post(&format!("{server}{UPLOAD_PATH}")).send_json(b);
    let text = match res {
        Ok(r) => r.into_string().map_err(|e| Error::Network(e.to_string()))?,
        // The contract reports errors as JSON; a status code with a JSON body is still an answer.
        Err(ureq::Error::Status(code, r)) => {
            let t = r.into_string().unwrap_or_default();
            if serde_json::from_str::<Reply>(&t).is_err() {
                return Err(Error::Network(format!("the server answered HTTP {code}")));
            }
            t
        }
        Err(e) => return Err(Error::Network(e.to_string())),
    };
    serde_json::from_str(&text).map_err(|_| Error::Server("the server's answer is not in the expected format".into()))
}

/// Where the next run starts after a reply: the end of the newest accepted bin, and past the whole chunk when the
/// server took none of it (rejected bins are not sent again).
pub fn next_cursor(chunk: &Chunk, interval_s: u32, reply: &Reply) -> Option<u32> {
    if !reply.ok {
        return None;
    }
    let by_server = (reply.next_after > 0).then_some(reply.next_after);
    let past_chunk = (reply.accepted == 0).then(|| chunk.end(interval_s));
    by_server.into_iter().chain(past_chunk).max()
}
