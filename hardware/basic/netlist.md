# Yarrow Counter Basic: nets

The schematic is drawn from this list. `R5-R8` are four 5.1 MΩ in series, `R10-R11` two 4.7 MΩ in series (reference
names as in `bom.csv`). Pin names of the MCU are port names; the package pin numbers are taken from the datasheet when
the symbol is drawn.

## Power

| Net | Connections |
|---|---|
| VBUS | J1 VBUS pins, F1 in |
| 5V | F1 out, U2 in, C5, U3 VBUS, U5 VCC, R4 in, Q2/BZ1 supply |
| 5V_BOOST | R4 out, C7, L1 in |
| 3V3 | U2 out, C6, U1 VDD pins with C9-C13, U1 VDDA with C14, U4 V+ with C15, R12, R15, D2 pin 2, J2 pin 1, J3 jumper |
| GND | J1 GND, all grounds; the high-voltage return is the same ground |

## USB

| Net | Connections |
|---|---|
| USB_DP | J1 D+ (both pins), U3 I/O1, U1 PA12 (through DNP2 footprint, a 0 Ω link in the first spin) |
| USB_DM | J1 D- (both pins), U3 I/O2, U1 PA11 (through DNP1, a 0 Ω link) |
| CC1, CC2 | J1 CC1, CC2, each to R1, R2 to ground |
| SHIELD | J1 shell, R3 to ground, C17 to ground |

## MCU

| Net | Connections |
|---|---|
| NRST | U1 NRST, SW1, 100 nF to ground (C16) |
| BOOT0 | U1 BOOT0, J3 pin 1 to 3V3 through the jumper, R17 to ground |
| OSC_IN / OSC_OUT | U1 PD0 / PD1 to Y1, C3 and C4 to ground |
| SWDIO / SWCLK | U1 PA13 / PA14, J2 pins 2 and 3 |
| PULSE | U4 out, U1 PA0 (TIM2_CH1) |
| PWM | U1 PA8 (TIM1_CH1), U5 A |
| HV_ADC | R9 top, C8, U1 PA1 |
| BUZ | U1 PB0, R19 to ground, Q2 gate |
| LED | U1 PC13, R18, D3 anode; D3 cathode to ground |

## High voltage

| Net | Connections |
|---|---|
| GATE | U5 Y, R20 |
| Q1G | R20, Q1 gate |
| SW | L1 out, Q1 drain, D1 anode |
| HV | D1 cathode, C2, R5 (divider top), R10 (anode resistor) |
| HV_DIV | R5-R8 in series from HV down to node HV_ADC; R9 from HV_ADC to ground |
| ANODE_R | R10-R11 in series from HV to ANODE |
| ANODE | R11 out, tube anode clip, C1 |
| Q1S | Q1 source to ground |

Tube cathode clip to ground. Keep-out and slot around HV, SW, ANODE_R, ANODE and the divider.

## Pulse input

| Net | Connections |
|---|---|
| PNODE | C1 (from ANODE), R12 to 3V3, R13 to ground, D2 cathode-to-anode clamp node (D2 pin 3), R14 |
| PIN | R14 out, U4 IN+ |
| VREF1 | R15 to 3V3, R16 to ground, U4 IN- |

D2 (BAV99): pin 1 to ground, pin 2 to 3V3, pin 3 (common) is PNODE. The comparator output is high at rest; a pulse
pulls PIN below VREF1 and the output falls.

## Buzzer

Q2 source to ground, drain to BZ1 negative, BZ1 positive to 5V.
