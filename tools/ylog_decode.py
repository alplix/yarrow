#!/usr/bin/env python3
"""Decoder for the Yarrow log (host side of src/ylc.c and src/ylog.c).

    python tools/ylog_decode.py flash.bin 4096 > samples.csv

flash.bin is the log window read back from the device (pages in ring order do not matter: the page sequence
numbers decide the order). Output: time,interval,time_valid,value"""
import struct
import sys

YLC_CTX = 44
YLC_UCAP = 11
PROB_BITS = 12
PROB_MIN = 31
PROB_MAX = (1 << PROB_BITS) - 31
ADAPT = 5
MEAN_CAP = (1 << 22) - 1
TOP = 1 << 24


class Model:
    def __init__(self, mean_q8):
        self.z = [2048] * YLC_CTX
        self.s = [2048] * YLC_CTX
        self.u = [[[2048] * YLC_UCAP for _ in range(2)] for _ in range(YLC_CTX)]
        self.m = [[[[2048, 2048] for _ in range(YLC_UCAP)] for _ in range(2)] for _ in range(YLC_CTX)]
        self.mean_q8 = mean_q8

    def ctx(self):
        v = self.mean_q8 + 1
        l = v.bit_length() - 1
        if l < 15:
            return 2 * l + (((v >> (l - 1)) & 1) if l >= 1 else 0)
        return min(30 + (l - 15), YLC_CTX - 1)

    def update_mean(self, x):
        target = min(x, MEAN_CAP) << 8
        self.mean_q8 += (target - self.mean_q8) >> ADAPT


def adapt(arr, idx, bit):
    v = arr[idx]
    if bit == 0:
        v += ((1 << PROB_BITS) - v) >> ADAPT
    else:
        v -= v >> ADAPT
    arr[idx] = min(max(v, PROB_MIN), PROB_MAX)


class Decoder:
    def __init__(self, data, mean_q8):
        self.data = data
        self.pos = 0
        self.decision_pos = 0
        self.range = 0xFFFFFFFF
        self.code = 0
        self.ended = False
        self.model = Model(mean_q8)
        for _ in range(4):
            self.code = (self.code << 8) | self._byte()

    def _byte(self):
        b = self.data[self.pos] if self.pos < len(self.data) else 0xFF
        self.pos += 1
        return b

    def _norm(self):
        while self.range < TOP:
            self.range = (self.range << 8) & 0xFFFFFFFF
            self.code = ((self.code << 8) | self._byte()) & 0xFFFFFFFF

    def _bit(self, arr, idx):
        bound = (self.range >> PROB_BITS) * arr[idx]
        self.decision_pos = self.pos
        if self.code < bound:
            self.range = bound
            bit = 0
        else:
            self.code -= bound
            self.range -= bound
            bit = 1
        adapt(arr, idx, bit)
        self._norm()
        return bit

    def _raw(self):
        self.decision_pos = self.pos
        self.range >>= 1
        if self.code >= self.range:
            self.code -= self.range
            bit = 1
        else:
            bit = 0
        self._norm()
        return bit

    def _flag(self):
        bound = (self.range >> 16) * 65535
        self.decision_pos = self.pos
        if self.code < bound:
            self.range = bound
            more = 1
        else:
            self.code -= bound
            self.range -= bound
            more = 0
        self._norm()
        return more

    def get(self):
        """Next sample, or None at the end of the stream or when the stored bytes run out."""
        if self.ended:
            return None
        more = self._flag()
        if self.decision_pos > len(self.data):
            return None
        if not more:
            self.ended = True
            return None
        m = self.model
        c = m.ctx()
        pred = (m.mean_q8 + 128) >> 8
        v = pred
        if self._bit_arr(m.z, c):
            neg = 0
            if pred > 0:
                neg = self._bit_arr(m.s, c)
            nb = 0
            while self._bit(m.u[c][neg], min(nb, YLC_UCAP - 1)):
                nb += 1
                if nb > 32:
                    return None
            nbc = min(nb, YLC_UCAP - 1)
            n = 1
            for j in range(nb):
                bit = self._bit(m.m[c][neg][nbc], j) if j < 2 else self._raw()
                n = (n << 1) | bit
            v = v - n if neg else v + n
        if self.decision_pos > len(self.data):
            return None
        if v < 0 or v > 0xFFFFFFFF:
            return None
        m.update_mean(v)
        return v

    def _bit_arr(self, arr, idx):
        return self._bit(arr, idx)


PAGE_HDR = 8
SEG_HDR = 14


def page_valid(pg):
    if pg[0] != 0x59 or pg[1] != 0x4C:
        return None
    check = 0xA5
    for b in pg[:6]:
        check ^= b
    if pg[6] != check:
        return None
    return struct.unpack_from("<I", pg, 2)[0]


def last_non_ff(pg, lo, hi):
    for i in range(hi, lo, -1):
        if pg[i - 1] != 0xFF:
            return i
    return lo


def segments(pg):
    off = PAGE_HDR
    size = len(pg)
    while off + SEG_HDR <= size:
        h = pg[off:off + SEG_HDR]
        if h[0] != 0xA7:
            return
        check = 0x5A
        for b in h[:11]:
            check ^= b
        if h[11] != check:
            return
        start = struct.unpack_from("<I", h, 2)[0]
        interval = struct.unpack_from("<H", h, 6)[0]
        mean = h[8] | h[9] << 8 | h[10] << 16
        valid_time = bool(h[1] & 1)
        stream_off = off + SEG_HDR
        length = struct.unpack_from("<H", h, 12)[0]
        open_seg = length == 0xFFFF
        torn = not open_seg and stream_off + length > size
        limit = size if (open_seg or torn) else stream_off + length
        data_end = last_non_ff(pg, stream_off, limit)
        used = data_end - stream_off
        valid_len = ((used - 1) & ~1) if used else 0
        end = stream_off + ((used + 1) & ~1) if (open_seg or torn) else stream_off + ((length + 1) & ~1)
        yield start, interval, valid_time, mean, bytes(pg[stream_off:stream_off + valid_len])
        off = end


def decode(image, page_size):
    pages = [image[i:i + page_size] for i in range(0, len(image) - page_size + 1, page_size)]
    seqs = [(page_valid(p), i) for i, p in enumerate(pages)]
    seqs = [(s, i) for s, i in seqs if s is not None]
    if not seqs:
        return
    newest = max(seqs, key=lambda x: (x[0] - max(s for s, _ in seqs)) & 0xFFFFFFFF or 0)
    newest_idx = max(seqs, key=lambda x: x[0])[1]
    n = len(pages)
    for k in range(1, n + 1):
        p = pages[(newest_idx + k) % n]
        if page_valid(p) is None:
            continue
        for start, interval, valid, mean, stream in segments(p):
            d = Decoder(stream, mean)
            i = 0
            while True:
                v = d.get()
                if v is None:
                    break
                yield start + i * interval, interval, int(valid), v
                i += 1


def main():
    image = open(sys.argv[1], "rb").read()
    page_size = int(sys.argv[2]) if len(sys.argv) > 2 else 4096
    print("time,interval,time_valid,value")
    for t, iv, ok, v in decode(image, page_size):
        print(f"{t},{iv},{ok},{v}")


if __name__ == "__main__":
    main()
