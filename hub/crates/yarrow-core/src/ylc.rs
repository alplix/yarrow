//! Range decoder for the `ylc` sample streams (port of `src/ylc.c`, decoder half).

const CTX: usize = 44;
const UCAP: usize = 11;
const PROB_BITS: u32 = 12;
const PROB_INIT: u16 = 1 << (PROB_BITS - 1);
const PROB_MIN: u32 = 31;
const PROB_MAX: u32 = (1 << PROB_BITS) - 31;
const ADAPT: u32 = 5;
const MEAN_CAP: u32 = (1 << 22) - 1;
const TOP: u32 = 1 << 24;

struct Model {
    z: [u16; CTX],
    s: [u16; CTX],
    u: [[[u16; UCAP]; 2]; CTX],
    m: [[[[u16; 2]; UCAP]; 2]; CTX],
    mean_q8: u32,
}

impl Model {
    fn new(mean_q8: u32) -> Self {
        Model {
            z: [PROB_INIT; CTX],
            s: [PROB_INIT; CTX],
            u: [[[PROB_INIT; UCAP]; 2]; CTX],
            m: [[[[PROB_INIT; 2]; UCAP]; 2]; CTX],
            mean_q8,
        }
    }

    /// Context from the running mean: half octaves up to a mean of 64, whole octaves above.
    fn ctx(&self) -> usize {
        let v = self.mean_q8.wrapping_add(1);
        let l = 31 - v.leading_zeros();
        if l < 15 {
            let half = if l >= 1 { (v >> (l - 1)) & 1 } else { 0 };
            (2 * l + half) as usize
        } else {
            ((30 + (l - 15)) as usize).min(CTX - 1)
        }
    }

    fn pred(&self) -> u32 {
        self.mean_q8.wrapping_add(128) >> 8
    }

    fn update_mean(&mut self, x: u32) {
        let target = (x.min(MEAN_CAP) << 8) as i32;
        let mean = self.mean_q8 as i32;
        self.mean_q8 = (mean + ((target - mean) >> ADAPT)) as u32;
    }
}

fn adapt(p: &mut u16, bit: u32) {
    let mut v = *p as u32;
    if bit == 0 {
        v += ((1 << PROB_BITS) - v) >> ADAPT;
    } else {
        v -= v >> ADAPT;
    }
    *p = v.clamp(PROB_MIN, PROB_MAX) as u16;
}

struct Coder<'a> {
    data: &'a [u8],
    pos: usize,
    decision_pos: usize,
    range: u32,
    code: u32,
}

impl Coder<'_> {
    fn byte(&mut self) -> u8 {
        let b = self.data.get(self.pos).copied().unwrap_or(0xFF);
        self.pos += 1;
        b
    }

    fn normalize(&mut self) {
        while self.range < TOP {
            self.range <<= 8;
            self.code = (self.code << 8) | self.byte() as u32;
        }
    }

    fn bit(&mut self, p: &mut u16) -> u32 {
        let bound = (self.range >> PROB_BITS) * *p as u32;
        self.decision_pos = self.pos;
        let bit = if self.code < bound {
            self.range = bound;
            0
        } else {
            self.code -= bound;
            self.range -= bound;
            1
        };
        adapt(p, bit);
        self.normalize();
        bit
    }

    fn raw(&mut self) -> u32 {
        self.decision_pos = self.pos;
        self.range >>= 1;
        let bit = if self.code >= self.range {
            self.code -= self.range;
            1
        } else {
            0
        };
        self.normalize();
        bit
    }

    /// The "more follows" flag, fixed probability 65535/65536.
    fn flag(&mut self) -> bool {
        let bound = (self.range >> 16) * 65535;
        self.decision_pos = self.pos;
        let more = if self.code < bound {
            self.range = bound;
            true
        } else {
            self.code -= bound;
            self.range -= bound;
            false
        };
        self.normalize();
        more
    }
}

/// Decoder for one stream. Reading past the end of the stored bytes yields erased flash (0xFF), exactly as the
/// device does, and a sample is only returned when every decision was taken from stored bytes.
pub struct Decoder<'a> {
    coder: Coder<'a>,
    ended: bool,
    model: Box<Model>,
}

impl<'a> Decoder<'a> {
    pub fn new(data: &'a [u8], mean_q8: u32) -> Self {
        let mut coder = Coder { data, pos: 0, decision_pos: 0, range: 0xFFFF_FFFF, code: 0 };
        for _ in 0..4 {
            coder.code = (coder.code << 8) | coder.byte() as u32;
        }
        Decoder { coder, ended: false, model: Box::new(Model::new(mean_q8)) }
    }

    /// Next sample, or `None` at the end of the stream, when the stored bytes run out or the stream is damaged.
    pub fn get(&mut self) -> Option<u32> {
        if self.ended {
            return None;
        }
        let more = self.coder.flag();
        if self.coder.decision_pos > self.coder.data.len() {
            return None;
        }
        if !more {
            self.ended = true;
            return None;
        }
        let m = &mut *self.model;
        let cd = &mut self.coder;
        let c = m.ctx();
        let pred = m.pred();
        let mut v = pred as i64;

        if cd.bit(&mut m.z[c]) != 0 {
            let mut neg = 0usize;
            if pred > 0 {
                neg = cd.bit(&mut m.s[c]) as usize;
            }
            let mut nb = 0usize;
            while cd.bit(&mut m.u[c][neg][nb.min(UCAP - 1)]) != 0 {
                nb += 1;
                if nb > 32 {
                    return None;
                }
            }
            let nbc = nb.min(UCAP - 1);
            let mut n: u64 = 1;
            for j in 0..nb {
                let bit = if j < 2 { cd.bit(&mut m.m[c][neg][nbc][j]) } else { cd.raw() };
                n = (n << 1) | bit as u64;
            }
            v = if neg != 0 { v - n as i64 } else { v + n as i64 };
        }

        if cd.decision_pos > cd.data.len() {
            return None;
        }
        if !(0..=0xFFFF_FFFF).contains(&v) {
            return None;
        }
        m.update_mean(v as u32);
        Some(v as u32)
    }
}
