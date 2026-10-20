#!/usr/bin/env python3
"""Reads the exported net list and prints every net with its pins, then checks the nets that matter against the design.
Run:  python3 check_nets.py ../kicad/yarrow_counter_basic.net.xml"""
import sys
import xml.etree.ElementTree as ET

root = ET.parse(sys.argv[1]).getroot()
nets = {}
for net in root.iter("net"):
    name = net.get("name").lstrip("/")
    nets[name] = sorted(f'{n.get("ref")}.{n.get("pin")}' for n in net.iter("node"))

for name in sorted(nets):
    print(f"{name:12s} {' '.join(nets[name])}")

expect = {
    "HV": {"D1.2", "C2.1", "R5.1", "R10.1"},
    "SW": {"L1.2", "Q1.2", "D1.1"},
    "HV_ADC": {"R8.2", "R9.1", "C8.1"},
    "ANODE": {"R11.2", "J4.1", "C1.1"},
    "PNODE": {"C1.2", "R12.2", "R13.1", "D2.3", "R14.1"},
    "PIN": {"R14.2"},
    "PULSE": {"U1.10"},
    "PWM": {"U1.29", "U5.2"},
    "GATE": {"U5.4", "R20.1"},
}
bad = 0
for n, must in expect.items():
    got = set(nets.get(n, []))
    if not must <= got:
        bad += 1
        print("MISSING in", n, sorted(must - got))
print("checked", len(expect), "nets,", bad, "problems")
