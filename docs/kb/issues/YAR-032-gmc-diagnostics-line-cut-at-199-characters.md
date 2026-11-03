---
id: YAR-032
title: GQ GMC diagnostics line in stderr.txt is cut at 199 characters, so the count histogram never appears
type: bug
status: confirmed
severity: low
platforms: [all]
area: hardware-io
action:
next: Give the diagnostics room: split the note() call at monitor/yarrow_src_gmc.c:134-136 in two (statistics, histogram) or enlarge line[] and seen[][] (51-53)
blocked_by: []
pattern:
aliases: []
reported: 2026-11-03
source: make check output 2026-11-03
where: monitor/yarrow_src_gmc.c:51-53, 134-136 @ 2c1dbb4
fixed_in:
---

## Symptom

The summary of the per-second counts that a GQ GMC source writes to stderr.txt after 300 and 1800 seconds (added in
71a4b6e) ends before its histogram. `make monitor-check` on macOS (Apple clang 14), 2026-11-03, at 2c1dbb4:

```
ygmc: diagnostics after 300 seconds: mean 0.497 counts/s, variance 0.503 (variance/mean 1.01, 1.00 for a Geiger tube), lag-1 correlation -0.02, byte order low byte first, histogram of counts 0,1,..,8,>=9: 
ygmc: diagnostics after 1800 seconds: mean 0.480 counts/s, variance 0.480 (variance/mean 1.00, 1.00 for a Geiger tube), lag-1 correlation -0.03, byte order high byte first, histogram of counts 0,1,..,8,>=9
```

All eight diagnostics lines of that run are exactly 199 characters after the `ygmc: ` prefix, and none shows the ten
counts.
The histogram is the part meant to tell a counter that sends something other than pulses from one that does.

## Cause

`note()` formats every message into `char line[200]` and remembers it in `seen[24][200]`
(monitor/yarrow_src_gmc.c:49-63), so a message is cut at 199 characters. The diagnostics message
(monitor/yarrow_src_gmc.c:134-136) is about 200 characters before the histogram `h` (up to 159 characters) is
appended. Verified by reading the code at 2c1dbb4 and by the test output above.

The diagnostics also take two of the 24 slots of `seen[]`, which make each distinct message appear only once; after
24 messages every further one is printed each time it happens. Not seen in a test; noted because a fix might add
lines.

## Fix

Proposed: write the histogram as a second `note()` call (each well under 199 characters), or make `line[]` and
`seen[][]` large enough for the longest message (about 360 characters). Splitting keeps the memory of `seen[]` as it
is. No effect on bins, credit or the trickle format.

## Test

`make monitor-check` and look at the `ygmc: diagnostics after` lines: each must end with ten numbers.

## Log

- 2026-11-03: created; found in the `make monitor-check` output at 2c1dbb4 while the knowledge base was rebased on it.
