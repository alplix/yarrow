# Installing Yarrow

Yarrow is a plain firmware file. The device's own bootloader (the first 24 KB of flash) is never changed, so the
stock firmware can be put back at any time with the same steps.

## What you need

- The GC-03 with a charged battery and a USB cable
- `yarrow-gc03-<language>.bin` from the [Releases](https://github.com/alplix/yarrow/releases) page
- The stock firmware file for your device (keep a copy before you start)

## Put Yarrow on the device

1. Switch the device off and unplug the cable.
2. Hold the **OK** key and press the power key. The screen stays dark.
3. Plug the USB cable into the computer. A small drive appears.
4. Copy the `.bin` file onto the drive. Wait until the drive disappears by itself.
5. Press the power key to start.

The first start shows the flower and the name, a loading bar, then the main screen. The footer reads *stage 1/5* and
moves on to 5 within about a minute: Yarrow switches its parts on one at a time (USB is the last). A part that was cut
off twice in a row, by a crash or by the power being taken away (a cable pulled, a battery that ran out), is held back:
the footer then stays below 5/5, and Settings, About names the held-back stage and offers *OK, retry*. Switching the device
off with the power key never holds anything back.

If the file was copied incompletely the device simply switches itself off at start-up. Repeat the copy.

## Go back to the stock firmware

Repeat the steps above with the stock file. Nothing of Yarrow stays behind that matters: its settings and log live in a
separate part of flash and are overwritten the next time Yarrow is installed.

## Languages

Each language is a separate file (`yarrow-gc03-en.bin`, `yarrow-gc03-tr.bin`, …). Changing language means installing the
other file. Settings and the log are kept.

## Where Yarrow keeps things

| Flash | Use |
|---|---|
| 0x08000000 – 0x08005FFF | stock bootloader, not touched |
| 0x08006000 – 0x0801DFFF | Yarrow firmware (up to 96 KB) |
| 0x0801E000 – 0x0801FFFF | settings (two 4 KB sectors) |
| 0x08020000 – 0x0803EFFF | log (4 KB pages) |
| 0x0803F000 – 0x0803FFFF | factory tube calibration, never written |

The Chinese, Japanese and Korean files carry bigger fonts and use a 128 KB firmware area: settings at 0x08026000 and the
log from 0x08028000. Switching between one of these and any other language file starts with default settings and a different log window, so
read the log out with the host tools first if you want to keep it.
