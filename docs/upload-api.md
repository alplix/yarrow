# Measurement upload API

The bin record below is the unit the BOINC monitor reports ([boinc-monitor.md](boinc-monitor.md)); only live counts are
sent, never the device's stored log. The HTTPS form is kept for clients that cannot run a BOINC task.

Yarrow Hub can share a device's measurements with a project server (the first one is bitboinc). This page is the
contract between the Hub and a server. Sharing is off until the user switches it on, and it only ever sends what the
user has seen in the preview.

## What is sent

Counts, not dose: the server converts with the calibration the device had, so a wrong sensitivity can be corrected later.

- **Bins.** The Hub cuts the decoded log into fixed bins and sends the pulse count of each. The user chooses the bin
  length: 10 minutes (the shortest), 30 minutes or 1 hour. Gaps (device off) are sent as `null`.
- **Device.** A hash of the chip's unique id (never the id itself), the firmware version and language, the sensitivity
  and dead time the device used.
- **Place.** Country (ISO 3166-1 alpha-2) and city name picked by the user from a list. No coordinates, nothing finer than
  a city.
- **Account.** The project account key. The Hub keeps it in the operating system's credential store.

## Request

`POST /yarrow/api/v1/upload`, `Content-Type: application/json`, over HTTPS.

```json
{
  "account_key": "…",
  "device": {"uid_hash": "…", "firmware": "0.1.0", "lang": "tr", "sensitivity_x100": 8000, "dead_time_us": 90},
  "location": {"country": "TR", "city": "Denizli"},
  "interval_s": 600,
  "start": 1792347600,
  "bins": [12, 9, 14, null, 11]
}
```

`start` is the Unix time (UTC) of the first bin and a multiple of `interval_s`. `interval_s` is one of 600, 1800, 3600.
At most 1008 bins (one week of 10-minute bins) per request.

## Reply

```json
{"ok": true, "accepted": 120, "rejected": 3, "credit": 36.0, "next_after": 1792347600}
```

`next_after` is the end of the newest accepted bin: the Hub continues from there next time. Errors:
`{"ok": false, "error": "bad_account" | "rate_limited" | "bad_data" | "too_large", "message": "…"}`.

## What a server checks

- the account exists and is not banned
- bins are aligned, not in the future, not older than the server allows, and not already received for this device
- counts per bin are physically possible for the stated calibration
- over a few days the counts follow a Poisson process (variance close to the mean); a stream that is too smooth or too
  noisy is rejected
- limits per account and per device per day

Credit is given for validated bins, so finer bins that pass the checks earn proportionally more.

## Reading the map

`GET /yarrow/api/v1/map` returns one entry per place that has at least three different devices:
`{"country": "TR", "city": "Denizli", "devices": 4, "median_nsvh": 118, "updated": 1792347600}`.
Places with fewer devices appear only as a country total.
