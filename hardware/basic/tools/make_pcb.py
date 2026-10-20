#!/usr/bin/env python3
"""Places the parts of the Yarrow Counter Basic board on a PCB from the schematic's net list.

Run inside WSL where KiCad 9 (python module pcbnew) is installed, after make_schematic.py and the net list export:

    python3 make_pcb.py ../kicad

It writes yarrow_counter_basic.kicad_pcb (outline, mounting holes, placement, nets, bottom ground pour, silkscreen), the net
classes of the project (.kicad_pro) and the custom design rules (.kicad_dru: 2.5 mm from every 400 V net to everything else).
No tracks yet: routing is the next step.
"""
import json
import os
import sys
import xml.etree.ElementTree as ET

import pcbnew

FPDIR = "/usr/share/kicad/footprints/"
PROJECT = "yarrow_counter_basic"
OX, OY = 100.0, 80.0           # board origin on the KiCad sheet, mm
W, H = 140.0, 50.0             # board size, mm
TUBE_LEN = 108.0               # clip to clip, SBM-20
mm = pcbnew.FromMM


def V(x, y):
    return pcbnew.VECTOR2I(mm(OX + x), mm(OY + y))


# ---------------------------------------------------------------- the net list
def read_netlist(path):
    root = ET.parse(path).getroot()
    comps = {}
    for c in root.iter("comp"):
        fp = c.find("footprint")
        comps[c.get("ref")] = (c.findtext("value") or "", fp.text if fp is not None else "")
    pad_net = {}
    for net in root.iter("net"):
        name = net.get("name").lstrip("/")
        if name.startswith("unconnected-"):
            continue
        for n in net.iter("node"):
            pad_net[(n.get("ref"), n.get("pin"))] = name
    return comps, pad_net


def load_fp(libid, ref, value):
    lib, name = libid.split(":")
    fp = pcbnew.FootprintLoad(FPDIR + lib + ".pretty", name)
    if fp is None:
        raise SystemExit("footprint not found: " + libid)
    fp.SetReference(ref)
    fp.SetValue(value)
    return fp


# ---------------------------------------------------------------- placement
# fixed positions: ref -> (x, y, rotation); everything else is packed into its region
FIXED = {
    "J5": (14.0, 8.0, 0), "J4": (14.0 + TUBE_LEN, 8.0, 0),            # tube clips, cathode left, anode right
    "J1": (6.0, 33.0, 90),                                           # USB-C on the left edge
    "U1": (58.0, 34.0, 0),
    # decoupling at the corners of the MCU, crystal and reset on its left, boot jumper above
    "C9": (51.0, 27.0, 0), "C10": (65.0, 27.0, 0), "C11": (65.0, 41.0, 0), "C12": (51.0, 41.0, 0), "C13": (58.0, 27.0, 0),
    "C14": (51.0, 34.0, 90), "Y1": (44.0, 31.0, 0), "C3": (44.0, 27.0, 0), "C4": (44.0, 35.0, 0), "C16": (48.0, 37.5, 90),
    "SW1": (45.0, 43.0, 0), "J2": (72.0, 33.0, 0), "J3": (62.0, 21.0, 90), "R17": (66.0, 22.0, 0),
    "R10": (116.0, 17.5, 90), "R11": (121.0, 17.5, 90),              # anode resistors under the anode clip
    "C1": (103.0, 21.0, 90),                                         # pulse coupling capacitor at the border of the HV area
    "R5": (136.0, 21.0, 90), "R6": (136.0, 27.5, 90), "R7": (136.0, 34.0, 90), "R8": (136.0, 40.5, 90),
    "R9": (136.0, 46.5, 90), "C8": (131.0, 46.5, 0),
    "Q1": (122.0, 38.0, 0),
}
REGIONS = {
    "usb": ((11.0, 18.0, 25.0, 48.0), ["F1", "U3", "R1", "R2", "R3", "C17", "DNP1", "DNP2"]),
    "power": ((26.0, 18.0, 41.0, 48.0), ["U2", "C5", "C6"]),
    "io": ((76.0, 18.0, 93.0, 48.0), ["U4", "R12", "R13", "R14", "R15", "R16", "C15", "D2", "BZ1", "Q2", "R19", "R18", "D3"]),
    "hv": ((94.0, 24.0, 134.0, 48.0), ["L1", "D1", "C2", "U5", "R20", "R4", "C7"]),
}
HOLES = [(3.0, 3.0), (W - 3.0, 3.0), (3.0, H - 3.0)]      # the fourth corner is taken by the divider


def box_mm(fp):
    bb = fp.GetBoundingBox(False)
    return pcbnew.ToMM(bb.GetWidth()), pcbnew.ToMM(bb.GetHeight())


def put(fp, cx, cy, rot=0):
    """Puts the middle of the footprint's box at (cx, cy)."""
    fp.SetPosition(V(0, 0))
    fp.SetOrientationDegrees(rot)
    bb = fp.GetBoundingBox(False)
    c, p = bb.GetCenter(), fp.GetPosition()
    fp.SetPosition(pcbnew.VECTOR2I(mm(OX + cx) - (c.x - p.x), mm(OY + cy) - (c.y - p.y)))


def pack(fps, refs, region, gap=1.0):
    x0, y0, x1, y1 = region
    x, y, rowh = x0, y0, 0.0
    for ref in sorted(refs, key=lambda r: -(box_mm(fps[r])[0] * box_mm(fps[r])[1])):
        w, h = box_mm(fps[ref])
        if x + w > x1:
            x, y, rowh = x0, y + rowh + gap, 0.0
        put(fps[ref], x + w / 2, y + h / 2)
        x += w + gap
        rowh = max(rowh, h)
        if y + h > y1 + 0.01:
            print(f"WARNING: region is full at {ref}", file=sys.stderr)


# ---------------------------------------------------------------- board
def line(b, x1, y1, x2, y2, layer, width=0.15):
    s = pcbnew.PCB_SHAPE(b)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(V(x1, y1))
    s.SetEnd(V(x2, y2))
    s.SetLayer(layer)
    s.SetWidth(mm(width))
    b.Add(s)


def rect(b, x1, y1, x2, y2, layer, width=0.15):
    for a, c in (((x1, y1), (x2, y1)), ((x2, y1), (x2, y2)), ((x2, y2), (x1, y2)), ((x1, y2), (x1, y1))):
        line(b, a[0], a[1], c[0], c[1], layer, width)


def text(b, s, x, y, size=1.2, layer=None):
    t = pcbnew.PCB_TEXT(b)
    t.SetText(s)
    t.SetLayer(layer if layer is not None else pcbnew.F_SilkS)
    t.SetPosition(V(x, y))
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    t.SetTextThickness(mm(size / 6))
    b.Add(t)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    comps, pad_net = read_netlist(os.path.join(out, PROJECT + ".net.xml"))
    b = pcbnew.BOARD()
    nets = {}

    def net(name):
        if name not in nets:
            n = pcbnew.NETINFO_ITEM(b, name)
            b.Add(n)
            nets[name] = n
        return nets[name]

    fps = {}
    for ref, (value, libid) in comps.items():
        fp = load_fp(libid, ref, value)
        for pad in fp.Pads():
            nm = pad_net.get((ref, pad.GetNumber()))
            if nm:
                pad.SetNet(net(nm))
        fp.Value().SetVisible(False)
        if ref in ("J4", "J5"):
            fp.Reference().SetVisible(False)             # the tube outline names them
        fp.Reference().SetTextSize(pcbnew.VECTOR2I(mm(0.8), mm(0.8)))
        fp.Reference().SetTextThickness(mm(0.12))
        fps[ref] = fp
        b.Add(fp)
    for (ref, pin), nm in pad_net.items():
        if ref in fps and not any(p.GetNumber() == pin for p in fps[ref].Pads()):
            print(f"WARNING {ref}: footprint has no pad {pin} (net {nm})", file=sys.stderr)

    # placement
    for ref, (x, y, rot) in FIXED.items():
        put(fps[ref], x, y, rot)
    for name, (region, refs) in REGIONS.items():
        pack(fps, refs, region)
    placed = set(FIXED) | {r for _, (_, refs) in REGIONS.items() for r in refs}
    left = sorted(set(fps) - placed)
    if left:
        print("WARNING: not placed:", left, file=sys.stderr)

    # outline and holes
    rect(b, 0, 0, W, H, pcbnew.Edge_Cuts, 0.1)
    for (x, y) in HOLES:
        h = pcbnew.FootprintLoad(FPDIR + "MountingHole.pretty", "MountingHole_2.7mm_M2.5")
        h.SetReference("H%d" % (HOLES.index((x, y)) + 1))
        h.SetValue("M2.5")
        h.SetPosition(V(x, y))
        b.Add(h)

    # silkscreen: the tube, the high-voltage area, a warning
    rect(b, 14.0, 2.5, 14.0 + TUBE_LEN, 13.5, pcbnew.F_SilkS)
    text(b, "SBM-20 / STS-5 tube", 14.0 + TUBE_LEN / 2, 8.0, 1.5)
    rect(b, 93.0, 16.0, W - 1.0, H - 1.0, pcbnew.F_SilkS, 0.3)
    text(b, "HIGH VOLTAGE 400 V", 115.0, H - 3.0, 1.5)
    text(b, "Yarrow Counter Basic rev A", 64.0, 15.2, 1.2)
    text(b, "anode", 14.0 + TUBE_LEN, 12.0, 0.8)
    text(b, "cathode", 14.0, 12.0, 0.8)

    # ground pour on the bottom layer
    z = pcbnew.ZONE(b)
    z.SetLayer(pcbnew.B_Cu)
    z.SetNet(net("GND"))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    poly = pcbnew.SHAPE_POLY_SET()
    chain = pcbnew.SHAPE_LINE_CHAIN()
    for (x, y) in ((0.3, 0.3), (W - 0.3, 0.3), (W - 0.3, H - 0.3), (0.3, H - 0.3)):
        chain.Append(V(x, y))
    chain.SetClosed(True)
    poly.AddOutline(chain)
    z.SetOutline(poly)
    b.Add(z)

    # design rules
    ds = b.GetDesignSettings()
    ds.m_MinClearance = mm(0.2)
    ds.m_TrackMinWidth = mm(0.2)
    ds.m_CopperEdgeClearance = mm(0.5)
    # the pour is filled by kicad-cli (drc --refill-zones --save-board): ZONE_FILLER crashes without the GUI
    path = os.path.join(out, PROJECT + ".kicad_pcb")
    pcbnew.SaveBoard(path, b)

    # project: net classes and custom rules
    pro_path = os.path.join(out, PROJECT + ".kicad_pro")
    pro = json.load(open(pro_path, encoding="utf-8"))

    def cls(name, clearance, width, priority):
        return {"name": name, "clearance": clearance, "track_width": width, "via_diameter": 0.8, "via_drill": 0.4, "wire_width": 6, "bus_width": 12,
                "line_style": 0, "microvia_diameter": 0.3, "microvia_drill": 0.1, "pcb_color": "rgba(0, 0, 0, 0.000)",
                "schematic_color": "rgba(0, 0, 0, 0.000)", "diff_pair_gap": 0.25, "diff_pair_via_gap": 0.25, "diff_pair_width": 0.2, "priority": priority}

    pro["net_settings"] = {
        "classes": [cls("Default", 0.2, 0.25, 2147483647), cls("HV", 0.5, 0.3, 0), cls("HVDIV", 0.3, 0.25, 1)],
        "meta": {"version": 4},
        "netclass_assignments": None,
        "netclass_patterns": [{"netclass": "HV", "pattern": p} for p in ("HV", "SW", "ANODE", "ANODE_R")] +
                             [{"netclass": "HVDIV", "pattern": p} for p in ("DIV1", "DIV2", "DIV3")],
    }
    json.dump(pro, open(pro_path, "w", encoding="utf-8", newline="\n"), indent=2)
    with open(os.path.join(out, PROJECT + ".kicad_dru"), "w", encoding="utf-8", newline="\n") as f:
        f.write("""(version 1)
# 400 V nets keep 2.5 mm from everything that is not itself high voltage (IPC-2221, 301 to 500 V, uncoated, external)
(rule "HV to low voltage"
  (condition "A.NetClass == 'HV' && B.NetClass != 'HV' && B.NetClass != 'HVDIV'")
  (constraint clearance (min 2.5mm)))
# the divider carries up to 300 V; between neighbours of the chain 100 V is all that is across a pad gap
(rule "HV divider to low voltage"
  (condition "A.NetClass == 'HVDIV' && B.NetClass != 'HV' && B.NetClass != 'HVDIV'")
  (constraint clearance (min 1.5mm)))
# the leads of the MOSFET are as far apart as its package makes them (600 V part, 1.1 mm); coat the board there
(rule "Q1 package"
  (condition "A.memberOfFootprint('Q1') && B.memberOfFootprint('Q1')")
  (constraint clearance (min 0.9mm)))
""")
    print("written", path, len(fps), "footprints")


if __name__ == "__main__":
    main()
