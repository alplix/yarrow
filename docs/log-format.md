# Log format

The log lives in the last flash sectors as a ring of 4 KB pages. It is written for years of unattended use on NOR flash,
so it only ever clears bits after an erase and is valid or ignored after any power cut.

## Pages and segments

- A **page** starts with a small header (magic and a sequence number). The page with the highest sequence number is
  the newest; the ring wraps by erasing the oldest page.
- A **segment** is one uninterrupted run of samples with a fixed interval, stored as one stream. Switching the device
  on and off appends a new segment to the page that still has room, so it does not burn pages.
- The last halfword of an unclosed stream is never trusted. Power-fail tests cut the supply 3000 times at random
  points and found no wrong sample.

## Samples

A sample is the number of pulses counted in one interval. The streams are compressed with `ylc`, an adaptive binary
range coder:

- 12-bit probabilities with shift-5 adaptation
- the value is coded relative to a slowly moving mean (shift 5) with a context per half-octave of the mean
- each sample carries a "more follows" flag so a stream can end anywhere without a length field
- carry handling as in LZMA's range coder

It lands within 3.5 – 8 % of the entropy of a Poisson source with the same mean, which is the limit for counting
data. Decoding is exact: the stored samples are bit for bit what was counted.

## Profiles

| Profile | Interval |
|---|---|
| Detailed | 1 s |
| Dense | 10 s |
| Balanced | 1 min |
| Long | 5 min |
| Archive | 1 h |

Alarm moments are always stored with one-second samples next to the chosen profile.

## Reading it

`tools/ylog_decode.py` decodes a page dump. `src/ylog.c` and `src/ylc.c` are the reference.
