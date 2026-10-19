# Calibration

A dose rate is only as good as three numbers (Menu, Calibration):

| Value | Meaning | Default |
|---|---|---|
| Sensitivity | counts per minute for 1 µSv/h | 180 (what the factory firmware of the GC-03 uses for its J321 tube) |
| Dead time | the time after a pulse during which the tube is blind, in µs; matters at high rates | 90 |
| Electric factor / magnetic factor | percent scale on the field readings | 100 |

## Calibration wizard

Menu, *Calibration wizard* finds the sensitivity for you.

1. **Reference.** Put Yarrow next to a source whose dose rate you know: the value another instrument shows in the same
   place, or a certified source. Turn the knob to enter that value and press OK.
2. **Measuring.** Yarrow counts. The screen shows the counts, the time and the statistical uncertainty. It beeps at
   1000 counts (about ±3 %). Longer is better; at background level this takes tens of minutes, with a source a minute.
   Alarms stay quiet while you calibrate. OK finishes (at least 100 counts are needed), Back starts over.
3. **Result.** The current and the new sensitivity, the change and the uncertainty. OK applies and saves it.

The new value is `corrected counts per minute / reference in µSv/h`, where the counts are corrected for dead time
second by second, so a strong source is handled correctly. If the tube is too busy (over 90 % dead) the wizard refuses
and asks you to move the source away.

## Fields

The electric and magnetic factors scale the readings to match a reference meter. The raw values are a first estimate
from the sensor circuit; compare with a known field (a mains cable at a given distance, a calibrated magnetometer) and
adjust the percent.

## Host tools

The same values can be read and set over USB (`CAL` command), see [serial-protocol.md](serial-protocol.md).
