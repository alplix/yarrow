# Yarrow Counter: open hardware

A family of USB Geiger counters designed for the Yarrow network, in three steps from cheap to accurate. Each step is a
board of its own, built and measured before the next one is drawn, so what is learned on the cheap board goes into the
better ones. The boards speak the Yarrow serial protocol ([../docs/serial-protocol.md](../docs/serial-protocol.md)), so
the BOINC monitor and Yarrow Hub read them without new drivers.

Status: design. Nothing here has been built yet.

## What decides the quality of a reading

A Geiger counter measures by counting, so the uncertainty is mostly counting noise: it falls with the square root of the
number of pulses. Everything below serves one of four things.

1. **More pulses for the same dose rate.** A bigger or a second tube. Ten times the pulses is three times less noise.
2. **Every pulse counted exactly.** Pulses are time-stamped by a hardware timer capture (microseconds), not polled, so the
   dead time of the tube can be measured and corrected, and a burst is never lost.
3. **A stable tube voltage.** The count rate of a tube depends on its voltage; a regulated and measured supply keeps the
   sensitivity the same over temperature and battery or USB sag.
4. **Time that does not depend on the computer.** Bins are cut at exact edges. A counter that keeps its own time (RTC, and
   at the top step GPS) does not need the host clock to be right.

## The three steps

| | **Basic** | **Plus** | **Pro** |
|---|---|---|---|
| Idea | the cheapest board that is correct | better tube and voltage control | the best reading one location can get |
| MCU | CH32F203 (as in the GC-03) | CH32F203 | CH32F203 or a larger one, plus ESP32-C3 for Wi-Fi |
| Tube | SBM-20 (the Radioactive@Home tube) | larger or higher-sensitivity tube (footprint for J305 and M4011 class) | two tubes of the same kind |
| High voltage | open-loop boost, trimmed once | closed loop on a measured voltage | closed loop, measured, temperature-compensated |
| Pulses | timer capture, microsecond stamps | the same, with pulse width | the same on both tubes, coincidences logged |
| Time | host clock, edges as the monitor does now | RTC with backup cell | GPS pulse-per-second discipline |
| Link | USB (CDC serial) | USB | USB and Wi-Fi (headless station) |
| Extras | LED, buzzer | small display, temperature sensor | pressure and humidity, SD log |
| Aim | a first kit anyone can solder; the reference for the protocol | trustworthy long-term station in a house | reference station; cross-checks the cheaper ones |

Open points for Plus and Pro are decided by what the Basic board shows: how the tube really behaves with the boost
converter, how much noise reaches the pulse input, how large the temperature drift is.

## Shared design rules

- **Safety.** The tube supply is about 400 V at a few microamps. The high-voltage part sits in one marked corner of the
  board with creepage clearance, nothing with high voltage is reachable from the outside, and the first prototypes are
  tested with a high-impedance probe and a current-limited source.
- **No bootloader lock-in.** Every board has a SWD header and a boot-select jumper, so a bad firmware can always be
  replaced (unlike the GC-03).
- **Only raw counts.** The firmware reports pulses and their time stamps; dose rate is calculated on the host from the
  sensitivity written in the device, so a better calibration can be applied later to old data.
- **Same protocol.** The identity and calibration commands report the board, the tube and its sensitivity and dead time.
- **Openness.** Schematic and layout in KiCad, firmware in this repository, licence CERN-OHL-S for the hardware and
  GPL-3.0 for the firmware.

## Rounds

1. **Basic, first spin.** Schematic, small two-layer board, order five. Measure: tube voltage and ripple, pulse shape,
   count rate against a reference counter (the GC-03 and a Radioactive@Home counter side by side), temperature drift.
2. **Basic, second spin** with what the first one showed, then published as a kit.
3. **Plus.** Closed-loop supply, RTC, display, the better tube; compared against Basic and the reference counters.
4. **Pro.** Two tubes, GPS, Wi-Fi; becomes the station that checks the cheaper ones.

Each round ends with a measurement report in `hardware/reports/` (the numbers, not just whether it works).

## What is here

This page, and next: `basic/` with the schematic, the bill of materials and the firmware port.
