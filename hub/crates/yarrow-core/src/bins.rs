//! Cutting decoded samples into fixed-length count bins.
//!
//! A bin is the sum of the samples that start inside it. Samples are not aligned to the bin grid (a log segment
//! starts when the device is switched on), so the window a bin really covers is shifted by less than one sample
//! interval. A bin is only produced when its samples follow each other without a gap and add up to exactly the bin
//! length; otherwise it is a gap (`None`). When a finer stream (the one-second samples around an alarm) runs next
//! to the profile samples, the coarser complete one wins.

use crate::Sample;
use std::collections::BTreeMap;

/// Bin lengths the sharing protocol allows, in seconds.
pub const ALLOWED_BIN_SECONDS: [u32; 3] = [600, 1800, 3600];

/// Counts per bin for the bins `first_bin .. first_bin + n`, where bin `k` covers
/// `[k * bin_s, (k + 1) * bin_s)`.
pub fn bin_counts(samples: &[Sample], bin_s: u32, first_bin: u32, n: usize) -> Vec<Option<u32>> {
    let mut result: Vec<Option<u32>> = vec![None; n];
    let mut filled = vec![false; n];
    // Coarser sample streams first.
    let mut by_interval: BTreeMap<u32, Vec<&Sample>> = BTreeMap::new();
    for s in samples.iter().filter(|s| s.time_valid && s.interval > 0 && bin_s % s.interval == 0) {
        by_interval.entry(s.interval).or_default().push(s);
    }
    for (&iv, list) in by_interval.iter().rev() {
        let mut list = list.clone();
        list.sort_by_key(|s| s.time);
        #[derive(Clone, Copy)]
        struct Acc {
            n: u32,
            sum: u64,
            last: u32,
            ok: bool,
        }
        let mut acc: BTreeMap<u32, Acc> = BTreeMap::new();
        for s in list {
            let b = s.time / bin_s;
            if b < first_bin || (b - first_bin) as usize >= n {
                continue;
            }
            let a = acc.entry(b).or_insert(Acc { n: 0, sum: 0, last: 0, ok: true });
            if a.n > 0 && s.time != a.last + iv {
                a.ok = false;
            }
            a.n += 1;
            a.sum += s.value as u64;
            a.last = s.time;
        }
        for (b, a) in acc {
            let i = (b - first_bin) as usize;
            if !filled[i] && a.ok && a.n == bin_s / iv && a.sum <= u32::MAX as u64 {
                result[i] = Some(a.sum as u32);
                filled[i] = true;
            }
        }
    }
    result
}

/// Bins covering `[from, to)` (both rounded to the grid: `from` up, `to` down). Returns the time of the first bin and
/// its counts.
pub fn bin_range(samples: &[Sample], bin_s: u32, from: u32, to: u32) -> (u32, Vec<Option<u32>>) {
    let first = from.div_ceil(bin_s);
    let end = to / bin_s;
    if end <= first {
        return (first * bin_s, Vec::new());
    }
    (first * bin_s, bin_counts(samples, bin_s, first, (end - first) as usize))
}
