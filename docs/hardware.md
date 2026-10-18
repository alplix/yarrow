# FNIRSI GC-03 hardware notes

These are the facts the firmware relies on, found on a real unit.

## Core
- MCU: WCH CH32F203RC, 8 MHz crystal, PLL x12 = 96 MHz. The firmware is built for the Cortex-M3 instruction set.
- Flash 256 KB: stock bootloader 0x08000000–0x08005FFF, application from 0x08006000
- Flash quirks: an erased cell reads 0xE339 (the controller scrambles data with 0x1CC6), the erase unit is 4 KB and
  programming is by halfword. The internal oscillator must stay on while flash is written.
- Last 1 KB page (0x0803FC00): factory tube calibration (signature word 0x4B6E4564, high-voltage frequency as a
  halfword at 0x0803FC1A in units of 100 Hz). Never erase or write it.
- RAM: 31 KB
- Display: ST7789, 240x320 colour, SPI1 (SCK PA5, MOSI PA7), CS PB0, DC PC4, RESET PC5, backlight PB13 (software PWM)
- Update path: hold OK while powering on, a USB drive appears, copy the `.bin`

## Pins
| Function | Pin |
|---|---|
| Tube HV PWM | PA2 (TIM5 CH3) |
| Tube pulse input | PA3 (EXTI3, falling edge, pull-up) |
| Power key | PA9 (pull-up) |
| Power hold | PA10, high = on |
| USB D- / D+ | PA11 / PA12 |
| Vibration motor | PB1 |
| Knob A / button / B | PB5 / PB6 / PB7 |
| Enter / Back keys | PB8 / PB9 |
| UART3 TX / RX | PB10 / PB11 |
| Pulse LED / Alert LED | PB14 / PB15 (active low) |
| Status LED | PC8 (active low) |
| Battery sense (divided by 2) | PC0 (ADC channel 10) |
| E-field / H-field | PC2 (channel 12) / PC3 (channel 13), AC, read as RMS over 320 samples |
| Charger status | PC7 low = charging, PC6 low = full, both high = no cable (no separate cable-sense pin) |
| Buzzer | PC11 (driven from the tick interrupt) |

## Clocks and interrupts
- SysTick at 8 kHz: a 1 ms time base, key scan, buzzer waveform, backlight PWM
- The start-up code keeps the power latch (PA10) high, points the vector table at the application and unmasks
  interrupts.
- USB: full speed, 48 MHz from the PLL divided by 2 (USBPRE), D+ pull-up switched in software

## Battery
Only a voltage divider and a charge-status pin exist, no fuel gauge. State of charge, run time and health are derived
from voltage, a history of the discharge slope and the charge-cycle count.

## Tube
J321 / M4011 Geiger tube driven by a boost converter whose PWM frequency and duty come from the factory page.
Pulses are counted by an EXTI interrupt; the dead time of the tube is about 90 µs.
