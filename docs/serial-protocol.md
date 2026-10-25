# USB serial protocol

When the device is plugged in outside update mode it shows up as a USB serial port (CDC ACM). The protocol is plain
text: one command per line, one or more lines back, and every reply starts with `OK` or `ERR`. Lines are at most 95
characters.

| Command | Reply |
|---|---|
| `PING` | `OK pong` |
| `INFO` | `OK yarrow=<version> stage=<n> verified=<n> time=<unix> theme=<n> profile=<n> pages=<n> pagesize=<bytes>` |
| `TIME` | `OK <unix seconds>` |
| `TIME <unix>` | sets the clock, `OK` |
| `LIVE 1` / `LIVE 0` | starts / stops a live stream, one line a second: `L <unix> <nSv/h> <cpm> <E x10 V/m> <H x100 µT> <battery %>` |
| `COUNTS` | `OK <pulses> <hv seconds> <unix>`: raw pulses since power on, seconds the tube voltage has been on since power on, clock |
| `BAT` | `OK <battery mV> <cable 0/1> <charging 0/1>` |
| `STATUS` | `OK 10` (the number of groups), then `S <group> key=value ...` lines (radiation, tube, fields, battery, clock, log, settings, thresholds, session, device), then `END`; a group continues on another line with the same group name |
| `CAPS` | `OK proto=<n>`, then `C <command> ...` lines, then `END`: what this firmware understands |
| `UID` | `OK <16 hex digits>`: a hash of the chip's unique id, the id itself is never sent |
| `LOGINFO` | `OK pages=<n> pagesize=<bytes> used=<n> oldest=<unix> newest=<unix>` |
| `LOGPAGE <n>` | `OK <n> <size>`, the raw logical page as base64 lines, then `END` |
| `EVENTS` | `OK <count>`, then lines `E <start> <duration s> <peak nSv/h> <level>`, then `END` |
| `CAL` | `OK sens=<x100> dead=<µs> e=<%> h=<%>` |
| `CAL <sens> <dead> <e> <h>` | sets the calibration, `OK`, or `ERR range` / `ERR args` |
| `SET THEME <n>` | choose a theme (0–20) |
| `HEALTH` | `OK boots=<n> wdt=<n> fault=<n> usb_restarts=<n> reason=<power_on\|pin\|watchdog\|window_watchdog\|software\|low_power\|fault> streak=<n> fault_pc=<address> uptime=<s> stage=<n> suspect=<mask>`: how the device has been running |

`<unix>` is Unix time in seconds (UTC). Sensitivity is counts per minute for 1 µSv/h times 100.

## Reading the log

A log page is a self-contained unit: a header with a sequence number followed by segments, each a stream of
range-coded samples. `LOGPAGE` returns the page exactly as it sits in flash, so a host decodes it without the device.
The format is described in [log-format.md](log-format.md); `tools/ylog_decode.py` is a reference decoder and
`make hostcheck` proves that it agrees with the firmware's own decoder sample for sample.
