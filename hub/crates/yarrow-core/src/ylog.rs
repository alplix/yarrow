//! Reader for the Yarrow log image (port of `src/ylog.c`, read side, and `tools/ylog_decode.py`).

use crate::ylc::Decoder;

const PAGE_HDR: usize = 8;
const SEG_HDR: usize = 14;
const PAGE_MAGIC: [u8; 2] = [0x59, 0x4C];
const SEG_MAGIC: u8 = 0xA7;
const FLAG_TIME_VALID: u8 = 1;

/// No real clock reads earlier than this (2026-10-01, before Yarrow existed); see `YARROW_MIN_TIME` in the firmware.
pub const MIN_TIME: u32 = 1_790_812_800;

/// One counted interval of the log.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Sample {
    /// Unix time (UTC) of the start of the interval. Only meaningful when `time_valid`.
    pub time: u32,
    /// Interval length in seconds.
    pub interval: u32,
    /// False when the device clock had not been set while this was recorded.
    pub time_valid: bool,
    /// Pulses counted in the interval.
    pub value: u32,
}

impl Sample {
    /// True when the time is a date: the clock had been set, and the time is not one from before the first release (the
    /// stock firmware's factory clock was read as a valid time by early firmware, and dated samples 381 days back).
    pub fn is_dated(&self) -> bool {
        self.time_valid && self.time >= MIN_TIME
    }
}

fn xor_check(b: &[u8], salt: u8) -> u8 {
    b.iter().fold(salt, |c, x| c ^ x)
}

fn rd32(b: &[u8]) -> u32 {
    u32::from_le_bytes([b[0], b[1], b[2], b[3]])
}

fn rd16(b: &[u8]) -> usize {
    u16::from_le_bytes([b[0], b[1]]) as usize
}

/// Sequence number of a page with a valid header.
fn page_seq(pg: &[u8]) -> Option<u32> {
    if pg.len() < PAGE_HDR || pg[..2] != PAGE_MAGIC || pg[6] != xor_check(&pg[..6], 0xA5) {
        return None;
    }
    Some(rd32(&pg[2..6]))
}

/// One past the last byte that is not blank (0xFF) in `[from, to)`, or `from` if the range is blank.
fn last_non_ff(pg: &[u8], from: usize, to: usize) -> usize {
    (from..to).rev().find(|&i| pg[i] != 0xFF).map_or(from, |i| i + 1)
}

struct Segment<'a> {
    start: u32,
    interval: u32,
    time_valid: bool,
    mean_q8: u32,
    stream: &'a [u8],
    end: usize,
}

fn even_up(v: usize) -> usize {
    (v + 1) & !1
}

fn parse_segment(pg: &[u8], off: usize) -> Option<Segment<'_>> {
    let size = pg.len();
    if off + SEG_HDR > size {
        return None;
    }
    let h = &pg[off..off + SEG_HDR];
    if h[0] != SEG_MAGIC || h[11] != xor_check(&h[..11], 0x5A) {
        return None;
    }
    let stream_off = off + SEG_HDR;
    let len = rd16(&h[12..14]);
    let open = len == 0xFFFF;
    let torn = !open && stream_off + len > size;
    let limit = if open || torn { size } else { stream_off + len };
    let data_end = last_non_ff(pg, stream_off, limit);
    // The last halfword written may be half programmed: never rely on it.
    let used = data_end - stream_off;
    let valid_len = if used > 0 { (used - 1) & !1 } else { 0 };
    let end = if open || torn { stream_off + even_up(used) } else { stream_off + even_up(len) };
    Some(Segment {
        start: rd32(&h[2..6]),
        interval: rd16(&h[6..8]) as u32,
        time_valid: h[1] & FLAG_TIME_VALID != 0,
        mean_q8: h[8] as u32 | (h[9] as u32) << 8 | (h[10] as u32) << 16,
        stream: &pg[stream_off..stream_off + valid_len],
        end,
    })
}

/// Decodes a log window read back from the device. Pages may sit in ring order: the sequence numbers in the page
/// headers decide the order. Samples come out oldest first.
///
/// Stops quietly at damage: whatever was decoded is exact, nothing is guessed.
pub fn decode_image(image: &[u8], page_size: usize) -> Vec<Sample> {
    let mut out = Vec::new();
    if page_size < PAGE_HDR + SEG_HDR {
        return out;
    }
    let pages: Vec<&[u8]> = image.chunks_exact(page_size).collect();
    let mut newest: Option<(usize, u32)> = None;
    for (i, p) in pages.iter().enumerate() {
        if let Some(seq) = page_seq(p) {
            if newest.is_none_or(|(_, best)| (seq.wrapping_sub(best) as i32) > 0) {
                newest = Some((i, seq));
            }
        }
    }
    let Some((newest, _)) = newest else { return out };
    let n = pages.len();
    for k in 1..=n {
        let pg = pages[(newest + k) % n];
        if page_seq(pg).is_none() {
            continue;
        }
        let mut off = PAGE_HDR;
        while let Some(seg) = parse_segment(pg, off) {
            let mut d = Decoder::new(seg.stream, seg.mean_q8);
            let mut i: u32 = 0;
            while let Some(v) = d.get() {
                out.push(Sample {
                    time: seg.start.wrapping_add(i.wrapping_mul(seg.interval)),
                    interval: seg.interval,
                    time_valid: seg.time_valid,
                    value: v,
                });
                i += 1;
            }
            off = seg.end;
        }
    }
    out
}
