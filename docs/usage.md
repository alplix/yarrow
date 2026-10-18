# Using Yarrow

## Keys

| Key | Does |
|---|---|
| Knob, turn | next / previous page, move the selection, change a value |
| Knob press or **OK** | open the menu, open the selected item, confirm |
| **Back** | go back |
| Power, short press | screen off and on |
| Power, hold 1.5 s | switch the device off |

The screen dims after 20 seconds and goes off after 90 seconds. A running alarm, a calibration measurement and a
sample test keep it on. The first key press after the screen went dark only wakes it.

## Pages

Turn the knob on the main screen to move through: **Radiation**, **Live trend**, **Electric field**,
**Magnetic field**, **Battery & device**, **History** (last seven days).

- **Radiation**: dose rate with a zone bar from 0.1 to 100 µSv/h, counts per minute, dose today, a ten-minute trend.
- **Electric and magnetic field**: the reading, trend, average and peak. Scale factors are in Calibration.
- **Battery & device**: charge, remaining time, health, charge cycles, total run time, tube life.

## Menu

| Entry | What it is |
|---|---|
| Emergency guide | what to do in a nuclear emergency, offline |
| Event log | past alarms with peak rate and duration |
| Dose budget | counts the dose against an amount you choose and warns at 80 % and 100 % |
| Sessions | a separate dose record for a trip, walk or survey, see below |
| Sample test | background first, then the sample; shows the difference |
| Field & wire finder | sound and bars follow an electric or magnetic source; turn the knob to switch between the electric and the magnetic field, OK switches the sound |
| Log & storage | how long the log holds and how it is thinned out |
| Alarm levels | the three thresholds |
| Tube status | factory high-voltage value, total pulses, dead-time correction |
| Self test | checks the parts one by one |
| PC link | status of the USB link |
| Themes | 21 colour themes |
| Settings | theme, brightness, alarm sound, vibration, time zone, clock |
| Calibration, Calibration wizard | see [calibration.md](calibration.md) |
| About | version and links |

## Alarms

Three levels with thresholds that you set: **Caution**, **Danger**, **Evacuate** (defaults 0.5, 10 and 100 µSv/h).
A level is entered only when the rate rests on enough counts (about a hundred, or a clear jump) and stays above the
threshold for two seconds, and it is left only after ten seconds below its hysteresis, so an alarm does not flicker and
does not come out of chance counts.
An alarm shows the matching steps, sounds the buzzer, vibrates and flashes the LED; the left key mutes it.

## Sessions

Start a session before a trip, a walk or a survey. While it runs Yarrow adds up the dose, the time and the highest
rate separately from everything else. It keeps counting when you leave the screen, and its totals are saved, so after switching the device off and on it
carries on from there (time with the device off is not counted). Finish it with OK twice. The last eight sessions are kept with date, duration, dose, average and
peak, and the dose is compared with the natural background of a day.

## Log

The log stores a sample every second, every ten seconds, every minute and so on, depending on the profile (Detailed, Dense, Balanced, Long, Archive); the *Log & storage* screen shows how many days the
flash holds with the chosen profile. Alarm moments are always saved with one-second samples. The log is read
over USB with the host tools, see [serial-protocol.md](serial-protocol.md).
