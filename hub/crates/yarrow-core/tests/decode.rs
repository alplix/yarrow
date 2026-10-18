//! The Rust decoder against the firmware's own decoder: `make hostcheck` writes the log image and the samples the C
//! code reads back; copies of both live in tests/fixtures.

use std::collections::HashMap;
use yarrow_core::{decode_image, Sample};

const FLASH: &[u8] = include_bytes!("fixtures/flash.bin");
const EXPECTED: &str = include_str!("fixtures/expected.csv");
const PAGE: usize = 1024;

fn expected() -> Vec<Sample> {
    EXPECTED
        .lines()
        .skip(1)
        .map(|l| {
            let f: Vec<u32> = l.split(',').map(|x| x.parse().unwrap()).collect();
            Sample { time: f[0], interval: f[1], time_valid: f[2] == 1, value: f[3] }
        })
        .collect()
}

#[test]
fn matches_the_c_decoder_sample_for_sample() {
    let want = expected();
    let got = decode_image(FLASH, PAGE);
    assert_eq!(want.len(), 19484);
    assert_eq!(got.len(), want.len());
    for (i, (a, b)) in got.iter().zip(&want).enumerate() {
        assert_eq!(a, b, "sample {i}");
    }
}

#[test]
fn fresh_build_from_the_repo_still_matches() {
    // When `make hostcheck` has been run in this checkout, the image it left behind must agree too.
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../build/tests");
    let (Ok(flash), Ok(csv)) = (std::fs::read(dir.join("flash.bin")), std::fs::read_to_string(dir.join("expected.csv")))
    else {
        return;
    };
    let got = decode_image(&flash, PAGE);
    assert_eq!(got.len(), csv.lines().count() - 1);
    for (a, l) in got.iter().zip(csv.lines().skip(1)) {
        assert_eq!(format!("{},{},{},{}", a.time, a.interval, a.time_valid as u32, a.value), l);
    }
}

#[test]
fn ring_order_does_not_matter() {
    // Rotate the pages: the sequence numbers decide the order, not the position.
    let want = decode_image(FLASH, PAGE);
    for k in 1..12 {
        let mut img = FLASH[k * PAGE..].to_vec();
        img.extend_from_slice(&FLASH[..k * PAGE]);
        assert_eq!(decode_image(&img, PAGE), want, "rotated by {k} pages");
    }
}

/// Power-cut style damage: the supply goes away while the newest page is being written. Everything from the cut on
/// reads as erased flash (0xFF), the byte before it may be half programmed. Whatever is decoded must be exactly
/// what was recorded, never a wrong value, and more bytes can never decode to fewer samples.
#[test]
#[allow(unused_assignments)]
fn power_cut_in_the_newest_page_never_gives_a_wrong_sample() {
    let want: HashMap<(u32, u32), u32> = expected().iter().map(|s| ((s.time, s.interval), s.value)).collect();
    let all = decode_image(FLASH, PAGE);
    // Which page is the newest one: the one holding the last sample's segment.
    let newest_page = (0..12)
        .max_by_key(|&p| {
            let pg = &FLASH[p * PAGE..(p + 1) * PAGE];
            if pg[0] == 0x59 {
                u32::from_le_bytes([pg[2], pg[3], pg[4], pg[5]])
            } else {
                0
            }
        })
        .unwrap();
    let base = newest_page * PAGE;
    for garbage in [None, Some(0x00u8), Some(0xA5u8)] {
        let mut last_count = 0usize;
        for cut in 8..=PAGE {
            let mut img = FLASH.to_vec();
            for b in &mut img[base + cut..base + PAGE] {
                *b = 0xFF;
            }
            if let (Some(g), true) = (garbage, cut > 8) {
                img[base + cut - 1] = g;
            }
            let got = decode_image(&img, PAGE);
            for s in &got {
                assert_eq!(want.get(&(s.time, s.interval)), Some(&s.value), "cut {cut}: wrong sample {s:?}");
            }
            assert!(got.len() <= all.len());
            if garbage.is_none() {
                assert!(got.len() >= last_count, "cut {cut}: fewer samples than with a shorter page");
                last_count = got.len();
            }
        }
    }
}

/// The length field of a segment is written when it is closed; a cut before that leaves it blank (open segment), and a
/// half-programmed field can hold an impossible value. For the last segment of a page the decoder has to find the end
/// of the data itself and must still only return recorded samples.
#[test]
fn open_segments_and_impossible_lengths_are_survived() {
    let want: HashMap<(u32, u32), u32> = expected().iter().map(|s| ((s.time, s.interval), s.value)).collect();
    let mut tried = 0;
    for page in 0..12 {
        let base = page * PAGE;
        if FLASH[base] != 0x59 {
            continue;
        }
        // Walk to the last segment header of the page.
        let mut off = 8usize;
        loop {
            let h = &FLASH[base + off..base + (off + 14).min(PAGE)];
            assert_eq!(h[0], 0xA7);
            let len = u16::from_le_bytes([h[12], h[13]]) as usize;
            let next = off + 14 + ((len + 1) & !1);
            if len == 0xFFFF || next + 14 > PAGE || FLASH[base + next] != 0xA7 {
                break;
            }
            off = next;
        }
        for bad in [0xFFFFu16, 0x7FFE, 0xFFFE] {
            let mut img = FLASH.to_vec();
            img[base + off + 12..base + off + 14].copy_from_slice(&bad.to_le_bytes());
            let got = decode_image(&img, PAGE);
            for s in &got {
                assert_eq!(want.get(&(s.time, s.interval)), Some(&s.value), "page {page} len {bad:#x}: {s:?}");
            }
            // The data is still all there: nothing but the last halfword is lost.
            let full = decode_image(FLASH, PAGE).len();
            assert!(got.len() + 4 >= full && got.len() <= full, "page {page} len {bad:#x}: {} of {full}", got.len());
            tried += 1;
        }
    }
    assert!(tried >= 9);
}

#[test]
fn garbage_and_empty_images_give_nothing() {
    assert!(decode_image(&[], PAGE).is_empty());
    assert!(decode_image(&vec![0xFF; 4 * PAGE], PAGE).is_empty());
    assert!(decode_image(&vec![0x00; 4 * PAGE], PAGE).is_empty());
    assert!(decode_image(&FLASH[..100], PAGE).is_empty());
}
