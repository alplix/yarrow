# BOINC monitor

A volunteer's detector joins a BOINC project by running the **Yarrow monitor**, a long-lived BOINC task that reads the
detector live and reports it. The BOINC account is the identity; nothing is uploaded without it.

## Principles

- **Live only.** The monitor reports what it counted while it was running. The device's stored log is never uploaded, so
  a stream cannot be edited or replayed afterwards.
- **Counts, not dose.** Each bin carries the pulse count plus the calibration in force; the server converts.
- **Choice of bin length.** 10 minutes (shortest), 30 minutes or 1 hour, set in the task's preferences.
- **Nothing finer than a city.** The place is a country code and a city name picked from a list.

## Parts

| Part | Where | Job |
|---|---|---|
| Monitor app | volunteer's machine, BOINC science app | talks to the detector, cuts bins, sends trickles |
| Detector driver | inside the monitor | one per detector family; the first is the Yarrow/GC-03 serial protocol |
| Trickle handler | project server | stores bins, answers with nothing (credit is granted after validation) |
| Validator job | project server | statistical checks, marks bins valid |
| Credit batch | project server | grants credit for valid bins |
| Map page | project website | city medians from places with at least three devices |

## Monitor app

One BOINC result stays running for as long as the BOINC client does. It looks for a detector every 15 seconds, polls it
while it is attached, and keeps the unsent bins in a checkpoint. When the detector disappears or the client closes, the
open bin is dropped, never padded; the bins already finished are still sent.

The monitor is a non-CPU-intensive (NCI) app: the server sends one task per computer and the BOINC client runs it next to a full set of CPU tasks, so it does not take a thread away from other work. Only one monitor runs per computer anyway: a lock file in the BOINC project folder keeps a second task out, which puts itself aside for six hours. The server gives every task a large work estimate, so the client does not ask for more than one.

Settings come from the project preferences on the web site (`yarrow_interval`, `yarrow_country`, `yarrow_city`), so a
volunteer sets them once for every computer. Source: `monitor/` (`yarrow_bin.c` cuts bins, `yarrow_trickle.c` writes
trickles, `yarrow_serial.c` talks to the port, `yarrow_boinc.cpp` is the BOINC host, `yarrow_standalone.c` a command-line
host that prints the trickles). `make monitor-check` tests the bin cutter against a simulated detector.

The GC-03 driver uses the serial commands `UID`, `INFO`, `CAL` and `COUNTS` ([serial-protocol.md](serial-protocol.md)).
`COUNTS` returns the raw pulse counter, the seconds the tube voltage has been on and the device clock in one line.

## Supported counters

The monitor reads the raw tube pulses of a counter through a source (`monitor/yarrow_src_*.c`); every source hands over the
same things: cumulative raw pulses with a time, the counter's identity, the sensitivity of its tube and its dead time.

| Counter | Source | Notes |
|---|---|---|
| Yarrow firmware (FNIRSI GC-03) | `yarrow_src_yarrow.c` | USB serial; the device has a clock of its own and latches its counters every second |
| Radioactive@Home USB counter (SBM-20, HID `16c0:05df`) | `yarrow_src_rah.c` | USB HID feature report; sensitivity 171.2 counts per minute for 1 uSv/h, dead time 190 us |
| GQ GMC-280, 300, 320, 500 | `yarrow_src_gmc.c` | USB serial (115200 or 57600 baud); one `<HEARTBEAT1>>` count per second; the sensitivity is read from the calibration stored in the counter, dead time 90 us |

A counter without a clock of its own is cut on the host's clock (the GQ GMC counts seconds of its own, so its edge reading is the one-second heartbeat that arrives within half a second of the edge, and a missing or bunched heartbeat drops the bin): a reading counts as the bin edge when it was taken within
50 ms before to 100 ms after the edge, which is a few hundredths of a count at background level. All other readings are
marked inexact and never close or open a bin. Only counters that give raw pulses are accepted; a counter that reports a
smoothed rate cannot pass the statistical checks of the server. The identity of a counter without a serial number is made
from its kind and the name of the computer. Looking for a counter sends nothing but `<GETVER>>` to the serial ports.

A bin is reported only when it is exact:

- both edge readings carry the device's clock second of the edge (the monitor polls ten times a second around edges)
- the tube voltage was on for the whole bin
- the counter did not restart and the device clock did not jump (checked against the host's monotonic clock)
- the calibration did not change inside the bin

Anything else drops that bin; nothing is estimated or padded. Bins waiting to be sent are checkpointed together with the
device and calibration they were counted under.

## Trickle-up

Variety `yarrow`, one trickle per sent batch (at most one every 10 minutes, so the live map is about one bin behind):

```xml
<yarrow v="1" start="1792347600" interval="600" dev="9f3c…" fw="0.1.0" sens="18000" dead="90"
        cc="TR" city="Denizli" bins="12,9,14,-,11"/>
```

`start` is the UTC Unix time of the first bin and a multiple of `interval`. `-` is a gap. The bin record is the one
described in [data.md](data.md).

## Running it on a project

- **Work.** The generator keeps a few monitor tasks ready; a task has no input, a year-long deadline and no time limit, and
  `max_jobs_in_progress` allows one per host, so one detector is never read twice. A task ends only when the client does.
- **Settings.** `yarrow_interval`, `yarrow_country`, `yarrow_city` and (optional) `yarrow_detector` are project preferences on the web site. While they
  are missing the task shows a message in the BOINC Manager and tries again an hour later.
- **Validation.** The task is valid when it ends normally; the bins are checked by the server job (above) and the credit
  comes from there.
- **Linux.** The BOINC client's user needs access to the serial port (`usermod -aG dialout boinc`).
- **Build.** `monitor/` builds with `make monitor-check` (bin cutter tests) and, for the project, against the BOINC
  libraries of every platform; the server pieces (schema, job, generator, scripts, public page) sit with the project.
- **Public pages.** The map (median dose rate per city) and the table of every city with its history; see [data.md](data.md).

## Server checks

The trickle handler only stores bins that are aligned, not in the future and not already received for the device.
Impossible counts (for the stated sensitivity and dead time, or a dose rate above 1,000 uSv/h) are rejected at once.
A day later the validator tests the whole stream of a device:

- the counts follow a Poisson process: variance close to the mean, neither far smaller (a made-up constant) nor far larger
- consecutive bins differ about as much as two independent counts do (a smooth curve scaled to the right spread fails)
- there is enough data to judge (72 bins, or 24 bins with at least 2,000 counts); until then a stream earns nothing

Checks that look at devices side by side hold a device when something is off. A held device keeps sending, its bins stay,
but it earns nothing until the project owner has looked at it:

- **copy**: the counts equal, or move together with, those of another device (independent counters never do); the later
  device is the copy and its bins are rejected
- **replay**: the counts repeat those of one day or one week earlier (a recorded day sent again); the repeats are rejected
- **region**: the median dose rate is one no place has (under 10 or over 2,000 nSv/h), or is far from the median of the
  trusted detectors of its city (needs three of them, so one odd detector never decides the reference)

Region holds lift themselves when the numbers become plausible, unless the owner marked the device as reviewed. Bins that
fail are kept, marked rejected, with the reason.

## Credit

Credit is per valid bin, so a 10-minute stream earns six times a 1-hour stream of the same duration. A new device earns a
part of the credit per bin (30 % at first, 60 % after 3 days and 288 valid bins, 100 % after 14 days and 2,000 valid
bins), so a device made up for the purpose never pays much. Per device and day only the first 150 valid bins pay, and an
account is paid for at most five devices. Every payment is written per device, so the owner can take the credit of a
device back. All amounts and thresholds are server settings.

## Map

Places with at least three different devices show a median dose rate. Fewer devices appear only in the country total.
