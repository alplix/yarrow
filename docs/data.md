# Measurement data

What Yarrow sends, what the server keeps and where you can read it. Only the BOINC monitor
([boinc-monitor.md](boinc-monitor.md)) sends anything. Yarrow Hub is a local program: it reads the counter, keeps the
history on your own computer and never uploads.

## What is sent

Counts, not dose: the server converts with the calibration the device had, so a wrong sensitivity can be corrected later.
Only live counts are sent, never the device's stored log.

- **Bins.** The pulse count of each fixed bin. The volunteer chooses the bin length in the project preferences: 10 minutes
  (the shortest), 30 minutes or 1 hour. A gap (counter off, bin not exact) is sent as `-`; nothing is estimated or padded.
- **Device.** A hash of the device's unique id (never the id itself), the firmware or driver name, the sensitivity and the
  dead time the counter was read with.
- **Place.** Country (ISO 3166-1 alpha-2) and the city name the volunteer typed. No coordinates, nothing finer than a city.
- **Detector type.** The type the volunteer names in the project preferences (optional). It is read from the account by the
  server, not sent with the bins.
- **Account.** Nothing extra: the trickle belongs to the volunteer's BOINC account and computer.

The record is a BOINC trickle-up of variety `yarrow`:

```xml
<yarrow v="1" start="1792347600" interval="600" dev="9f3c…" fw="rah-2.51" sens="17123" dead="190"
        cc="TR" city="Denizli" bins="12,9,14,-,11"/>
```

`start` is the UTC Unix time of the first bin and a multiple of `interval`. At most one trickle is sent every 10 minutes.

## What the server checks

- the account exists and the device is not banned
- bins are aligned, not in the future and not already received for this device
- counts per bin are physically possible for the stated calibration
- after a day the whole stream of a device must follow a Poisson process: variance close to the mean, neighbouring bins
  as different as independent counts are, never too smooth, never too noisy
- devices side by side: a copy of another device, a replayed day or week, and a dose rate no place has or one far from the
  other detectors of the same city hold the device until the project owner has looked at it
- limits per device and per account per day

Credit is given for validated bins, scaled by how long the device has been sending (see
[boinc-monitor.md](boinc-monitor.md)).

## Reading the data

- **Map.** [bitboinc.athena.org.tr/yarrow.php](https://bitboinc.athena.org.tr/yarrow.php): the median dose rate of every
  city over 1 hour, 3 hours, 24 hours or 7 days, with a notice where a city is clearly above its usual level.
- **Data by city.** [bitboinc.athena.org.tr/yarrow_history.php](https://bitboinc.athena.org.tr/yarrow_history.php): a table
  with one row per city (median of the last 24 hours, 7 days, 30 days and of all time, the detector types, the hours of
  data) and, for each city, its history per hour or per day with the lowest and the highest detector. Every history can be
  downloaded as CSV (`&csv=1`).

A city's value is the median over its detectors, so one wrong detector cannot move it. Volunteers are never named on
these pages and nothing finer than a city is shown.
