# Building and testing

## Tools

- `arm-none-eabi-gcc` 12 or newer
- GNU make (`mingw32-make` on Windows)
- Python 3.10+ with `pillow`, `numpy` and `unicorn` for the font generator, the emulator tests and the contact sheets:
  `pip install pillow numpy unicorn`
- A host C compiler (`gcc`) for the unit tests and the simulator

## Firmware

    make                  build/yarrow-gc03.bin (Turkish)
    make YLANG=en         build/en/yarrow-gc03.bin
    make release YLANG=en copy it to dist/yarrow-gc03-en.bin

The `.bin` carries an image header (magic, length, CRC-32). The start-up code checks it before anything else and
switches the device off if it does not match, so a half-copied file cannot run.

## Tests

    make check      host unit tests: range coder, log, measurement, alarm, battery, settings store, formatting,
                    calibration, estimator bias, false alarms, redraw engine, the whole application
    make test       the firmware image in an ARM emulator: normal start, truncated and damaged images,
                    the power latch, the watchdog and the flash windows
    make hostcheck  the Python log decoder against the C decoder on a generated log
    make sim        renders every screen in every theme to build/sim, checks the redraw engine against a full draw
                    and lists texts that do not fit (make sim YLANG=en for another language)

## Layout

| Directory | Contents |
|---|---|
| `src/` | portable code: measurement, alarm, log, settings store, screens, application |
| `src/lang/` | the text of every language |
| `hal/gc03/` | everything specific to the GC-03: start-up, clocks, display, USB, flash windows |
| `tests/`, `sim/` | host tests and the screen simulator |
| `tools/` | font generator, image finaliser, emulator tests, log decoder |
| `canary/` | tiny single-purpose images for bringing up a board, see [bringup.md](bringup.md) |
| `third_party/` | the Inter font (OFL) |
