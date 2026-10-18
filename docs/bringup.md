# Bringing up a board

The `canary/` directory holds tiny single-purpose firmware images for testing one part of the hardware at a time on a
new board or a new unit. Each is built with `make canary N=<letter>` and installed like any image
([install.md](install.md)). They all switch the device off by themselves.

| Image | What it shows |
|---|---|
| A | a green screen with a white square for 20 seconds: size, start-up check, display |
| E | a bar that grows for 35 seconds: the device stays alive and the timing is right |
| F | A plus text drawing: the font path works |
| G | twelve to eighteen squares that turn green one after the other: font, pins, clock, ADC, tick interrupt. The number of green squares when something stops names the step that failed |
| D | the hardware layer without text: pins, clock, ADC and tick, shown as squares |
| B | a read-only diagnostic screen: key states, knob events, battery voltage, sensors, clock, the factory calibration page |
| C | the independent watchdog on its own |

## The rules the start-up code follows

- Hold the power latch (PA10) before anything else can go wrong.
- Verify the image header and CRC first; switch off if it is wrong.
- Any unexpected exception switches the device off instead of leaving the latch held.
- A reset by the watchdog is a hang on the last run: do not run again, switch off.
- Point the vector table at the application and unmask interrupts.
- Start the backlight at full duty: the soft-PWM backlight must not turn the screen off when the tick starts.
