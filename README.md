# Yarrow

Open firmware for the FNIRSI GC-03 radiation detector. Readable at a glance, measures honestly, logs for years.

![Yarrow screens](docs/images/hero.png)

**Website:** [yarrow.athena.org.tr](https://yarrow.athena.org.tr) · **Releases:** one firmware file per language

## What it does

**Radiation**
- Dose rate, counts per minute, dose today, this week and in total
- An averaging window that follows the rate: long and quiet at background, quick when the rate really changes, without leaning high or low
- Tube dead-time correction
- Three alarm levels with fixed thresholds you set yourself, hysteresis, an event log and an alarm that never fires from a handful of counts
- Dose budget, sample test (background against sample), sessions for trips, walks and surveys, a calibration wizard

**Fields**
- Electric and magnetic field readings with trends
- A finder that follows wires and field sources by sound and bars

**Device**
- Battery health, remaining run time, charge cycles, a charging screen
- A log designed for years in the mountains: lossless range-coded samples, power-fail safe, within a few percent of the theoretical minimum size
- Offline emergency guide
- 21 colour themes, pixel-art decoration, a self test
- USB serial protocol for the host tools

## Install

Download the file for your language from [Releases](https://github.com/alplix/yarrow/releases) and copy it to the device in update mode. The stock bootloader is never touched, so the stock firmware can always be put back the same way. Step by step: [docs/install.md](docs/install.md).

## Build

Needs `arm-none-eabi-gcc`, GNU make and Python 3 for the build helpers.

    make                 # Turkish, build/yarrow-gc03.bin
    make YLANG=en        # English, build/en/yarrow-gc03.bin
    make release YLANG=en  # copies it to dist/yarrow-gc03-en.bin
    make check           # host tests
    make test            # runs the image in an ARM emulator
    make sim             # renders every screen to build/sim

Details in [docs/building.md](docs/building.md).

## Languages

Every language is its own firmware file, because the fonts take most of the room. Turkish and English are done. Adding one means copying `src/lang/en.h`, translating it and running `tools/langcheck.py`: [docs/translating.md](docs/translating.md).

## Documentation

| | |
|---|---|
| [docs/install.md](docs/install.md) | putting Yarrow on the device and going back |
| [docs/usage.md](docs/usage.md) | keys, screens, alarms, sessions |
| [docs/calibration.md](docs/calibration.md) | matching the readings to a reference |
| [docs/hardware.md](docs/hardware.md) | pins and memory map of the GC-03 |
| [docs/serial-protocol.md](docs/serial-protocol.md) | the USB command protocol |
| [docs/log-format.md](docs/log-format.md) | how the log is stored and compressed |
| [docs/building.md](docs/building.md) | toolchain, tests, simulator |
| [docs/translating.md](docs/translating.md) | adding a language |
| [docs/bringup.md](docs/bringup.md) | test images for a new board |

## Licence

GPL-3.0, see [LICENSE](LICENSE). The Inter font in `third_party/inter` is under the SIL Open Font License.
