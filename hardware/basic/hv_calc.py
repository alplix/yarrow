#!/usr/bin/env python3
"""Numbers behind the high-voltage supply and the pulse input of the Yarrow Counter Basic board.

A boost converter in discontinuous mode: the switch is on for t_on, the inductor current rises to Ipk = Vin * t_on / L,
and the energy 1/2 L Ipk^2 goes to the output every cycle. Run:  python hardware/basic/hv_calc.py
"""
from math import sqrt

VIN = 5.0            # USB
L = 1e-3             # H
F = 30e3             # switching frequency, Hz
ETA = 0.70           # switch, diode and inductor losses (assumed; measure on the first board)
R_TOP = 4 * 5.1e6    # high-voltage divider, four 1206 resistors in series
R_BOT = 62e3
RA = 2 * 4.7e6       # anode resistor of the tube
C_OUT = 10e-9        # output capacitor, 630 V
ADC_VREF = 3.3
ADC_BITS = 12


def power(t_on):
    ipk = VIN * t_on / L
    return ipk, 0.5 * L * ipk ** 2 * F * ETA


def v_eq(p, r_load):
    """Output voltage at which the load takes exactly p watts."""
    return sqrt(p * r_load)


r_div = R_TOP + R_BOT
print(f"divider: {r_div / 1e6:.2f} Mohm, ratio {r_div / R_BOT:.1f}, draws {400 / r_div * 1e6:.1f} uA (= {400 ** 2 / r_div * 1e3:.1f} mW) at 400 V")
lsb = ADC_VREF / (2 ** ADC_BITS) * (r_div / R_BOT)
print(f"ADC step: {lsb:.2f} V per count; 400 V reads {400 * R_BOT / r_div:.3f} V ({400 * R_BOT / r_div / ADC_VREF * 100:.0f} % of full scale)")
print(f"tube anode resistor {RA / 1e6:.1f} Mohm; recovery time constant with 10 pF: {RA * 10e-12 * 1e6:.0f} us")
print()
print("t_on [us]  duty [%]  Ipk [mA]  P [mW]  open-loop V on the divider alone [V]")
for t_us in (2, 4, 5, 5.5, 6, 7, 8, 10, 13):
    ipk, p = power(t_us * 1e-6)
    print(f"{t_us:8.1f}  {t_us * 1e-6 * F * 100:8.1f}  {ipk * 1e3:8.1f}  {p * 1e3:6.1f}  {v_eq(p, r_div):8.0f}")
print()
need_p = 400 ** 2 / r_div
t_need = sqrt(2 * need_p / (ETA * L * F)) * L / VIN
print(f"400 V needs {need_p * 1e3:.1f} mW: t_on = {t_need * 1e6:.1f} us ({t_need * F * 100:.0f} % duty), Ipk = {VIN * t_need / L * 1e3:.0f} mA")
t_max = 7.3e-6
ipk, p = power(t_max)
print(f"firmware duty clamp {t_max * F * 100:.0f} % (t_on {t_max * 1e6:.1f} us): Ipk {ipk * 1e3:.0f} mA, at most {p * 1e3:.0f} mW, {v_eq(p, r_div):.0f} V with no tube attached")
print(f"inductor must not saturate below {1.5 * ipk * 1e3:.0f} mA (1.5 x the clamped peak)")
print(f"ripple at 400 V: one cycle adds {power(t_need)[1] / F / (C_OUT * 400):.3f} V on {C_OUT * 1e9:.0f} nF")
print()
# pulse input: a tube pulse pulls the anode down by dv; the coupling capacitor passes it to a clamped node
C1, RB1, RB2, CS = 22e-12, 220e3, 330e3, 10e-12
rpar = RB1 * RB2 / (RB1 + RB2)
vbias = 3.3 * RB2 / (RB1 + RB2)
print(f"pulse input: C1 {C1 * 1e12:.0f} pF into {rpar / 1e3:.0f} kohm, tau {C1 * rpar * 1e6:.1f} us, idle level {vbias:.2f} V")
for dv in (50, 100, 200):
    print(f"  anode step {dv} V -> {dv * C1 / (C1 + CS):.0f} V before the clamp (the clamp diodes hold it to 0.3 V below ground)")
