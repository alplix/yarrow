"""Renders every SPR(...) sprite of src/ui.c to one enlarged PNG for review: python tools/spritesheet.py out.png"""
import re, sys
from PIL import Image, ImageDraw

src = open("src/ui.c", encoding="utf-8").read()
cols = {"y": (250, 204, 21), "w": (255, 255, 255), "p": (244, 114, 182), "l": (74, 222, 128), "a": (96, 165, 250),
        "b": (248, 113, 113), "h": (251, 146, 60), "d": (120, 90, 60), "i": (200, 210, 225)}
items = []
for m in re.finditer(r'SPR\((\w+),\s*((?:"[^"]*"\s*,?\s*)+)\)\s*;', src):
    rows = re.findall(r'"([^"]*)"', m.group(2))
    items.append((m.group(1), rows))
S = 8
W = 1100
x = y = 10
rowh = 0
img = Image.new("RGB", (W, 900), (10, 14, 20))
d = ImageDraw.Draw(img)
for name, rows in items:
    w = max(len(r) for r in rows) * S
    h = len(rows) * S
    if x + w + 10 > W:
        x = 10
        y += rowh + 26
        rowh = 0
    for ry, r in enumerate(rows):
        for rx, c in enumerate(r):
            if c != ".":
                d.rectangle([x + rx * S, y + ry * S, x + rx * S + S - 2, y + ry * S + S - 2], fill=cols.get(c, (255, 0, 255)))
    d.text((x, y + h + 2), name.replace("spr_", ""), fill=(180, 190, 200))
    x += max(w, 70) + 14
    rowh = max(rowh, h)
img = img.crop((0, 0, W, y + rowh + 30))
img.save(sys.argv[1])
print(len(items), "sprites")
