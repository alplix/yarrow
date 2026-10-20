#!/usr/bin/env python3
"""Writes the KiCad schematic of the Yarrow Counter Basic board from the parts and nets listed below (the same as
../netlist.md and ../bom.csv). Run inside WSL where KiCad 9 and its libraries are installed:

    python3 make_schematic.py ../kicad

Every pin gets a short wire and a net label (or a power symbol), so the nets are exact and the sheet can be read and edited
by hand in KiCad afterwards. ERC:  kicad-cli sch erc ../kicad/yarrow_counter_basic.kicad_sch
"""
import copy
import os
import re
import sys
import uuid

LIBDIR = "/usr/share/kicad/symbols/"
PROJECT = "yarrow_counter_basic"
POWER = {"GND", "+5V", "+3V3"}


# ---------------------------------------------------------------- s-expressions
class Str(str):
    """A quoted string of the file."""


def tokenize(text):
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in " \t\r\n":
            i += 1
        elif c in "()":
            yield c
            i += 1
        elif c == '"':
            j = i + 1
            buf = []
            while text[j] != '"':
                if text[j] == "\\":
                    buf.append(text[j:j + 2])
                    j += 2
                else:
                    buf.append(text[j])
                    j += 1
            yield Str("".join(buf))
            i = j + 1
        else:
            j = i
            while j < n and text[j] not in " \t\r\n()":
                j += 1
            yield text[i:j]
            i = j


def parse(text):
    stack, cur = [], []
    for t in tokenize(text):
        if t == "(":
            stack.append(cur)
            cur = []
        elif t == ")":
            done = cur
            cur = stack.pop()
            cur.append(done)
        else:
            cur.append(t)
    return cur


def dump(x):
    if isinstance(x, list):
        return "(" + " ".join(dump(e) for e in x) + ")"
    if isinstance(x, Str):
        return '"' + x + '"'
    return str(x)


def find(node, name):
    return [c for c in node if isinstance(c, list) and c and c[0] == name]


# ---------------------------------------------------------------- libraries
_libs = {}


def lib(name):
    if name not in _libs:
        tree = parse(open(LIBDIR + name + ".kicad_sym", encoding="utf-8").read())[0]
        _libs[name] = {str(s[1]): s for s in find(tree, "symbol")}
    return _libs[name]


def flat_symbol(libname, sym):
    """The symbol with `extends` resolved, renamed Lib:Name for the schematic's lib_symbols."""
    s = lib(libname)[sym]
    ext = find(s, "extends")
    if ext:
        base = copy.deepcopy(flat_symbol_raw(libname, str(ext[0][1])))
        old = str(ext[0][1])
        for sub in find(base, "symbol"):
            sub[1] = Str(str(sub[1]).replace(old, sym, 1))
        props = {str(p[1]): p for p in find(s, "property")}
        base = [e for e in base if not (isinstance(e, list) and e and e[0] == "property")]
        base[1] = Str(sym)
        base[2:2] = [copy.deepcopy(p) for p in find(s, "property")]
        s = base
    s = copy.deepcopy(s)
    s[1] = Str(libname + ":" + sym)
    return s


def flat_symbol_raw(libname, sym):
    s = lib(libname)[sym]
    ext = find(s, "extends")
    if ext:
        return flat_symbol(libname, sym)
    return s


def pins_of(libname, sym):
    """[(unit, number, name, x, y, angle)] of the symbol; unit 0 holds the pins shared by all units."""
    s = lib(libname)[sym]
    ext = find(s, "extends")
    if ext:
        s = lib(libname)[str(ext[0][1])]
        sym = str(ext[0][1])
    out = []
    for sub in find(s, "symbol"):
        parts = str(sub[1]).split("_")
        unit = int(parts[-2])
        for p in find(sub, "pin"):
            at = find(p, "at")[0]
            out.append((unit, str(find(p, "number")[0][1]), str(find(p, "name")[0][1]), float(at[1]), float(at[2]), float(at[3]), str(p[1])))
    return out


# ---------------------------------------------------------------- the design
R = "Resistor_SMD:R_0402_1005Metric"
R12 = "Resistor_SMD:R_1206_3216Metric"
C = "Capacitor_SMD:C_0402_1005Metric"
C08 = "Capacitor_SMD:C_0805_2012Metric"
C12 = "Capacitor_SMD:C_1206_3216Metric"
C1812 = "Capacitor_SMD:C_1812_4532Metric"   # wider pad gap for the 400 V nodes

PARTS = []  # (ref, libid, value, footprint, {pin: net})


def part(ref, libid, value, fp, nets, units=(1,)):
    PARTS.append((ref, libid, value, fp, nets, units))


def two(ref, libid, value, fp, a, b):
    part(ref, libid, value, fp, {"1": a, "2": b})


# power and USB
part("J1", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C", "Connector_USB:USB_C_Receptacle_G-Switch_GT-USB-7010ASV",
     {"A1": "GND", "A12": "GND", "B1": "GND", "B12": "GND", "A4": "VBUS", "A9": "VBUS", "B4": "VBUS", "B9": "VBUS", "A5": "CC1", "B5": "CC2",
      "A6": "USB_DP", "B6": "USB_DP", "A7": "USB_DM", "B7": "USB_DM", "A8": None, "B8": None, "S1": "SHIELD"})
two("F1", "Device:Polyfuse", "500 mA", "Fuse:Fuse_1206_3216Metric", "VBUS", "+5V")
part("U3", "Power_Protection:USBLC6-2SC6", "USBLC6-2SC6", "Package_TO_SOT_SMD:SOT-23-6",
     {"1": "USB_DP", "6": "USB_DP", "3": "USB_DM", "4": "USB_DM", "2": "GND", "5": "VBUS"})
part("U2", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", "Package_TO_SOT_SMD:SOT-23-5", {"1": "+5V", "3": "+5V", "2": "GND", "4": None, "5": "+3V3"})
two("C5", "Device:C", "10uF", C08, "+5V", "GND")
two("C6", "Device:C", "10uF", C08, "+3V3", "GND")
two("R1", "Device:R", "5.1k", R, "CC1", "GND")
two("R2", "Device:R", "5.1k", R, "CC2", "GND")
two("R3", "Device:R", "1M", R, "SHIELD", "GND")
two("C17", "Device:C", "4.7nF", C, "SHIELD", "GND")
two("DNP1", "Device:R", "0R (22R if D- rings)", R, "USB_DM", "USB_DM_MCU")
two("DNP2", "Device:R", "0R (22R if D+ rings)", R, "USB_DP", "USB_DP_MCU")

# MCU
mcu_nets = {"VBAT": "+3V3", "VDD": "+3V3", "VDDA": "+3V3", "VSS": "GND", "VSSA": "GND", "NRST": "NRST", "BOOT0": "BOOT0",
            "PD0": "OSC_IN", "PD1": "OSC_OUT", "PA0": "PULSE", "PA1": "HV_ADC", "PA8": "PWM", "PA11": "USB_DM_MCU", "PA12": "USB_DP_MCU",
            "PA13": "SWDIO", "PA14": "SWCLK", "PB0": "BUZ", "PC13": "LED"}
part("U1", "MCU_ST_STM32F1:STM32F103C8Tx", "CH32F203C8T6", "Package_QFP:LQFP-48_7x7mm_P0.5mm", mcu_nets)
for i in range(9, 14):
    two("C%d" % i, "Device:C", "100nF", C, "+3V3", "GND")
two("C14", "Device:C", "100nF", C, "+3V3", "GND")
two("C16", "Device:C", "100nF", C, "NRST", "GND")
part("Y1", "Device:Crystal_GND24", "8MHz", "Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm", {"1": "OSC_IN", "3": "OSC_OUT", "2": "GND", "4": "GND"})
two("C3", "Device:C", "18pF", C, "OSC_IN", "GND")
two("C4", "Device:C", "18pF", C, "OSC_OUT", "GND")
part("SW1", "Switch:SW_Push", "NRST", "Button_Switch_SMD:SW_SPST_B3S-1000", {"1": "NRST", "2": "GND"})
part("J2", "Connector:Conn_01x04_Pin", "SWD", "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", {"1": "+3V3", "2": "SWDIO", "3": "SWCLK", "4": "GND"})
part("J3", "Connector:Conn_01x02_Pin", "BOOT0", "Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical", {"1": "+3V3", "2": "BOOT0"})
two("R17", "Device:R", "10k", R, "BOOT0", "GND")

# high voltage
two("R4", "Device:R", "10R", "Resistor_SMD:R_0603_1608Metric", "+5V", "5V_BOOST")
two("C7", "Device:C", "22uF", C08, "5V_BOOST", "GND")
two("L1", "Device:L", "1mH", "Inductor_SMD:L_7.3x7.3_H4.5", "5V_BOOST", "SW")
part("U5", "74xGxx:74AHCT1G04", "74AHCT1G04", "Package_TO_SOT_SMD:SOT-23-5", {"2": "PWM", "4": "GATE", "5": "+5V", "3": "GND", "1": None}, units=(1, 2))
two("R20", "Device:R", "100R", R, "GATE", "Q1G")
part("Q1", "Transistor_FET:Q_NMOS_GDS", "600V N-MOSFET", "Package_TO_SOT_SMD:TO-252-3_TabPin2", {"1": "Q1G", "2": "SW", "3": "GND"})
two("D1", "Device:D", "ES1J", "Diode_SMD:D_SMA", "SW", "HV")
two("C2", "Device:C", "10nF 630V", C1812, "HV", "GND")
two("R5", "Device:R", "5.1M", R12, "HV", "DIV1")
two("R6", "Device:R", "5.1M", R12, "DIV1", "DIV2")
two("R7", "Device:R", "5.1M", R12, "DIV2", "DIV3")
two("R8", "Device:R", "5.1M", R12, "DIV3", "HV_ADC")
two("R9", "Device:R", "62k", R, "HV_ADC", "GND")
two("C8", "Device:C", "100nF", C, "HV_ADC", "GND")
two("R10", "Device:R", "4.7M", R12, "HV", "ANODE_R")
two("R11", "Device:R", "4.7M", R12, "ANODE_R", "ANODE")
part("J4", "Connector:Conn_01x01_Pin", "tube clip, anode", "TestPoint:TestPoint_THTPad_D3.0mm_Drill1.5mm", {"1": "ANODE"})
part("J5", "Connector:Conn_01x01_Pin", "tube clip, cathode", "TestPoint:TestPoint_THTPad_D3.0mm_Drill1.5mm", {"1": "GND"})

# pulse input
two("C1", "Device:C", "22pF 1kV", C1812, "ANODE", "PNODE")
two("R12", "Device:R", "220k", R, "+3V3", "PNODE")
two("R13", "Device:R", "330k", R, "PNODE", "GND")
part("D2", "Diode:BAV99", "BAV99", "Package_TO_SOT_SMD:SOT-23", {"1": "GND", "2": "+3V3", "3": "PNODE"})
two("R14", "Device:R", "10k", R, "PNODE", "PIN")
part("U4", "Comparator:LMV331", "TLV3201", "Package_TO_SOT_SMD:SOT-23-5", {"+": "PIN", "-": "VREF1", "~": "PULSE", "V+": "+3V3", "V-": "GND"})
two("R15", "Device:R", "10k", R, "+3V3", "VREF1")
two("R16", "Device:R", "4.3k", R, "VREF1", "GND")
two("C15", "Device:C", "100nF", C, "+3V3", "GND")

# buzzer and LED
part("Q2", "Transistor_FET:2N7002", "2N7002", "Package_TO_SOT_SMD:SOT-23", {"G": "BUZ", "S": "GND", "D": "BUZ_N"})
two("R19", "Device:R", "10k", R, "BUZ", "GND")
part("BZ1", "Device:Buzzer", "piezo", "Buzzer_Beeper:Buzzer_12x9.5RM7.6", {"1": "+5V", "2": "BUZ_N"})
two("R18", "Device:R", "1k", R, "LED", "LED_A")
part("D3", "Device:LED", "green", "LED_SMD:LED_0805_2012Metric", {"1": "GND", "2": "LED_A"})


# ---------------------------------------------------------------- writing
def uid():
    return str(uuid.uuid4())


def fmt(v):
    return ("%.4f" % v).rstrip("0").rstrip(".")


def build():
    root = uid()
    libids, body = {}, []
    pwr = [0]
    x_cursor, y_cursor, row_h = 30.0, 40.0, 0.0
    page_w = 560.0
    n_sym = 0

    def power_symbol(net, x, y):
        pwr[0] += 1
        libids["power:" + net] = ("power", net)
        n = pwr[0]
        body.append(f'(symbol (lib_id "power:{net}") (at {fmt(x)} {fmt(y)} 0) (unit 1) (exclude_from_sim no) (in_bom no) (on_board no) (dnp no) '
                    f'(uuid "{uid()}") (property "Reference" "#PWR{n:02d}" (at {fmt(x)} {fmt(y + 5)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Value" "{net}" (at {fmt(x)} {fmt(y + 3)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Footprint" "" (at {fmt(x)} {fmt(y)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Datasheet" "" (at {fmt(x)} {fmt(y)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(pin "1" (uuid "{uid()}")) (instances (project "{PROJECT}" (path "/{root}" (reference "#PWR{n:02d}") (unit 1)))))')

    def stub(px, py, ang, net, no_connect=False):
        """A wire from the pin outwards and the net label (or power symbol, or no-connect flag) at its end."""
        import math
        ox, oy = -math.cos(math.radians(ang)), math.sin(math.radians(ang))        # outward on the sheet (y points down)
        if no_connect:
            body.append(f'(no_connect (at {fmt(px)} {fmt(py)}) (uuid "{uid()}"))')
            return
        L = 5.08
        ex, ey = px + ox * L, py + oy * L
        body.append(f'(wire (pts (xy {fmt(px)} {fmt(py)}) (xy {fmt(ex)} {fmt(ey)})) (stroke (width 0) (type default)) (uuid "{uid()}"))')
        if net in POWER:
            power_symbol(net, ex, ey)
        else:
            la = {(1, 0): 0, (-1, 0): 180, (0, 1): 270, (0, -1): 90}[(round(ox), round(oy))]
            just = {0: "left bottom", 180: "right bottom", 90: "left bottom", 270: "left bottom"}[la]
            body.append(f'(label "{net}" (at {fmt(ex)} {fmt(ey)} {la}) (effects (font (size 1.27 1.27)) (justify {just})) (uuid "{uid()}"))')

    for ref, libid, value, fp, nets, units in PARTS:
        libname, sym = libid.split(":")
        libids[libid] = (libname, sym)
        pins = pins_of(libname, sym)
        # place by pin extents; flow layout
        xs = [p[3] for p in pins]
        ys = [p[4] for p in pins]
        w = max(xs) - min(xs) + 38.0 if xs else 40.0
        h = max(ys) - min(ys) + 16.0 if ys else 20.0
        if x_cursor + w > page_w:
            x_cursor, y_cursor, row_h = 30.0, y_cursor + row_h + 6.0, 0.0
        ox = round((x_cursor + 19.0 - min(xs)) / 2.54) * 2.54
        oy = round((y_cursor + 8.0 + max(ys)) / 2.54) * 2.54
        x_cursor += w + 6.0
        row_h = max(row_h, h)
        used = set()
        for unit in units:
            body.append(f'(symbol (lib_id "{libid}") (at {fmt(ox)} {fmt(oy)} 0) (unit {unit}) (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp no) '
                        f'(uuid "{uid()}") (property "Reference" "{ref}" (at {fmt(ox)} {fmt(oy - max(ys) - 5)} 0) (effects (font (size 1.27 1.27)))) '
                        f'(property "Value" "{value}" (at {fmt(ox)} {fmt(oy - max(ys) - 3)} 0) (effects (font (size 1.27 1.27)))) '
                        f'(property "Footprint" "{fp}" (at {fmt(ox)} {fmt(oy)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                        f'(property "Datasheet" "" (at {fmt(ox)} {fmt(oy)} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                        + "".join(f'(pin "{p[1]}" (uuid "{uid()}")) ' for p in pins if p[0] in (0, unit))
                        + f'(instances (project "{PROJECT}" (path "/{root}" (reference "{ref}") (unit {unit})))))')
            for (u, num, name, px, py, ang, ptype) in pins:
                if u not in (0, unit):
                    continue
                net = None
                key = None
                for k in (num, name, name.split("/")[0].split("-")[0], re.sub(r"\(.*\)", "", name)):
                    if k in nets:
                        key = k
                        break
                if key is None:
                    # a name with extras like "PA0-WKUP" or "PB1" is matched by its leading port name
                    for k in nets:
                        if re.fullmatch(re.escape(k) + r"([-/(].*)?", name):
                            key = k
                            break
                if key is not None:
                    net = nets[key]
                    used.add(key)
                if net is None:
                    if ptype == "no_connect" or key is not None or True:
                        stub(ox + px, oy - py, ang, "", no_connect=True)
                else:
                    stub(ox + px, oy - py, ang, net)
        missing = [k for k in nets if k not in used]
        if missing:
            print(f"WARNING {ref}: no pin matched {missing}", file=sys.stderr)
        n_sym += 1

    # power flags for the nets that only passive pins drive
    for net, (fx, fy) in (("VBUS", (25.4, 20.32)), ("GND", (60.96, 20.32)), ("+5V", (96.52, 20.32))):
        libids["power:PWR_FLAG"] = ("power", "PWR_FLAG")
        pwr[0] += 1
        n = pwr[0]
        body.append(f'(symbol (lib_id "power:PWR_FLAG") (at {fx} {fy} 0) (unit 1) (exclude_from_sim no) (in_bom no) (on_board no) (dnp no) (uuid "{uid()}") '
                    f'(property "Reference" "#FLG{n:02d}" (at {fx} {fy - 4} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Value" "PWR_FLAG" (at {fx} {fy - 2} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Footprint" "" (at {fx} {fy} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(property "Datasheet" "" (at {fx} {fy} 0) (effects (font (size 1.27 1.27)) (hide yes))) '
                    f'(pin "1" (uuid "{uid()}")) (instances (project "{PROJECT}" (path "/{root}" (reference "#FLG{n:02d}") (unit 1)))))')
        if net in POWER:
            power_symbol(net, fx, fy)
        else:
            body.append(f'(label "{net}" (at {fx} {fy} 0) (effects (font (size 1.27 1.27)) (justify left bottom)) (uuid "{uid()}"))')

    syms = []
    for libid, (libname, sym) in libids.items():
        syms.append(dump(flat_symbol(libname, sym)))
    head = (f'(kicad_sch (version 20250114) (generator "eeschema") (generator_version "9.0") (uuid "{root}") (paper "A2") '
            f'(title_block (title "Yarrow Counter Basic") (date "2026-10-20") (rev "A") (company "Yarrow, GPL-3.0 / CERN-OHL-S") '
            f'(comment 1 "Generated by hardware/basic/tools/make_schematic.py from netlist.md and bom.csv")) (lib_symbols ' + " ".join(syms) + ") ")
    return head + " ".join(body) + ' (sheet_instances (path "/" (page "1"))) (embedded_fonts no))\n'


def write_table(out, fname, kind, names, uri):
    """A project library table, so the editor and ERC find every library the sheet uses."""
    lines = ["(%s" % kind, "  (version 7)"]
    for n in names:
        lines.append('  (lib (name "%s")(type "KiCad")(uri "%s")(options "")(descr ""))' % (n, uri % n))
    lines.append(")")
    with open(os.path.join(out, fname), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, PROJECT + ".kicad_sch"), "w", encoding="utf-8", newline="\n") as f:
        f.write(build())
    with open(os.path.join(out, PROJECT + ".kicad_pro"), "w", encoding="utf-8", newline="\n") as f:
        f.write('{\n  "meta": {"filename": "%s.kicad_pro", "version": 1}\n}\n' % PROJECT)
    symlibs = sorted({p[1].split(":")[0] for p in PARTS} | {"power"})
    fplibs = sorted({p[3].split(":")[0] for p in PARTS})
    write_table(out, "sym-lib-table", "sym_lib_table", symlibs, "${KICAD9_SYMBOL_DIR}/%s.kicad_sym")
    write_table(out, "fp-lib-table", "fp_lib_table", fplibs, "${KICAD9_FOOTPRINT_DIR}/%s.pretty")
    print("written", out, len(PARTS), "parts")


if __name__ == "__main__":
    main()
