#!/usr/bin/env python3
"""Turns the simulator's PPM files into one contact sheet per theme: python tools/montage.py build/sim"""
import os
import sys
from PIL import Image

folder = sys.argv[1]
themes = ["dark", "light", "paper", "amber", "sakura", "night", "terminal", "ocean", "forest", "contrast", "lavender",
          "mint", "sunset", "charcoal", "sand", "ice", "cherry", "violet", "candy", "mono", "korea"]
order = ["boot", "main", "trend", "efield", "hfield", "battery", "history", "alarm", "mode", "menu", "guide_list",
         "guide_page0", "guide_page5", "settings", "events", "budget", "sample", "log", "storage", "thresh", "tube",
         "selftest", "usb", "scan", "themes", "about", "menu_end", "charge"]
cols = int(sys.argv[2]) if len(sys.argv) > 2 else 6
scale = float(sys.argv[3]) if len(sys.argv) > 3 else 0.6
only = sys.argv[4].split(",") if len(sys.argv) > 4 else order

for t in themes:
    shots = [n for n in order if n in only and os.path.exists(os.path.join(folder, f"{t}_{n}.ppm"))]
    if not shots:
        continue
    w, h = int(240 * scale), int(320 * scale)
    rows = (len(shots) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (w + 8) + 8, rows * (h + 8) + 8), (60, 60, 60))
    for i, n in enumerate(shots):
        im = Image.open(os.path.join(folder, f"{t}_{n}.ppm")).convert("RGB")
        im = im.resize((w, h), Image.LANCZOS)
        sheet.paste(im, (8 + (i % cols) * (w + 8), 8 + (i // cols) * (h + 8)))
    sheet.save(os.path.join(folder, f"sheet_{t}.png"))
    print("sheet", t, len(shots), "screens")


# one sheet that shows the same screens in every theme
for shot in ("main", "alarm", "themes", "about", "charge"):
    have = [t for t in themes if os.path.exists(os.path.join(folder, f"{t}_{shot}.ppm"))]
    if not have:
        continue
    w, h = int(240 * 0.5), int(320 * 0.5)
    ncols = 5
    nrows = (len(have) + ncols - 1) // ncols
    sheet = Image.new("RGB", (ncols * (w + 6) + 6, nrows * (h + 6) + 6), (60, 60, 60))
    for i, t in enumerate(have):
        im = Image.open(os.path.join(folder, f"{t}_{shot}.ppm")).convert("RGB").resize((w, h), Image.LANCZOS)
        sheet.paste(im, (6 + (i % ncols) * (w + 6), 6 + (i // ncols) * (h + 6)))
    sheet.save(os.path.join(folder, f"gallery_{shot}.png"))
    print("gallery", shot, len(have))
