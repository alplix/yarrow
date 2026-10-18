"""Draws the larger decoration sprites from simple geometry and rewrites their SPR(...) lines in src/ui.c.
Usage: python tools/gensprites.py   (then python tools/spritesheet.py out.png to look at them)"""
import math
import re


def grid(w, h):
    return [["."] * w for _ in range(h)]


def put(g, x, y, c):
    if 0 <= y < len(g) and 0 <= x < len(g[0]):
        g[y][x] = c


def rows(g):
    return ["".join(r) for r in g]


def poly_inside(px, py, pts):
    inside = False
    j = len(pts) - 1
    for i in range(len(pts)):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > py) != (yj > py) and px < (xj - xi) * (py - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def trefoil():
    n = 13
    c = 6
    g = grid(n, n)
    for y in range(n):
        for x in range(n):
            dx, dy = x - c, c - y
            r = math.hypot(dx, dy)
            a = math.degrees(math.atan2(dy, dx)) % 360
            if r <= 1.5:
                g[y][x] = "y"
            elif 2.8 <= r <= 6.7:
                for centre in (90, 210, 330):
                    if abs((a - centre + 180) % 360 - 180) <= 31:
                        g[y][x] = "y"
    return rows(g)


def atom():
    n = 19
    c = 9
    g = grid(n, n)
    for th in (40, 140):
        t_rad = math.radians(th)
        for k in range(4000):
            t = k * math.pi / 2000
            ex, ey = 8.6 * math.cos(t), 3.0 * math.sin(t)
            x = c + ex * math.cos(t_rad) - ey * math.sin(t_rad)
            y = c - (ex * math.sin(t_rad) + ey * math.cos(t_rad))
            put(g, round(x), round(y), "a")
    for y in range(n):
        for x in range(n):
            if math.hypot(x - c, y - c) <= 2.0:
                g[y][x] = "h"
    for (x, y) in ((15, 3), (3, 15), (15, 15)):
        for dx in (0, 1):
            for dy in (0, 1):
                put(g, x + dx - 1, y + dy - 1, "w")
    return rows(g)


def tower():
    w, h = 15, 16
    g = grid(w, h)
    half = {3: 3.5, 4: 3.0, 5: 2.5, 6: 2.5, 7: 2.5, 8: 3.0, 9: 3.5, 10: 4.0, 11: 4.5, 12: 5.0, 13: 5.5, 14: 6.0, 15: 6.5}
    for y, hw in half.items():
        x0, x1 = round(7 - hw), round(7 + hw)
        for x in range(x0, min(x1, 14) + 1):
            put(g, x, y, "d" if x == x1 else "i")
    for x in range(round(7 - 3.6), round(7 + 3.6) + 1):
        put(g, x, 3, "d")
    for (x, y) in ((4, 1), (5, 0), (6, 0), (5, 1), (6, 1), (7, 1), (9, 0), (10, 0), (11, 0), (9, 1), (10, 1), (8, 2), (9, 2)):
        put(g, x, y, "d")
    return rows(g)


def mushroom():
    w, h = 15, 15
    g = grid(w, h)
    for y in range(h):
        for x in range(w):
            if ((x - 7) / 7.2) ** 2 + ((y - 4) / 4.2) ** 2 <= 1 and y <= 6:
                g[y][x] = "h"
    for y in range(7, 11):  # stem
        for x in range(6, 9):
            put(g, x, y, "d")
    for y in range(11, 15):  # foot
        spread = y - 10
        for x in range(7 - 1 - spread, 7 + 2 + spread):
            put(g, x, y, "d")
    for (x, y) in ((4, 2), (5, 1), (6, 1), (3, 3)):  # highlight
        put(g, x, y, "w")
    for x in range(1, 14):  # shaded underside of the cap
        if g[6][x] == "h":
            g[6][x] = "d"
    return rows(g)


def skull():
    w, h = 11, 12
    g = grid(w, h)
    for y in range(h):
        for x in range(w):
            if ((x - 5) / 5.2) ** 2 + ((y - 4.4) / 4.6) ** 2 <= 1:
                g[y][x] = "i"
    for y in range(8, 12):
        for x in range(2, 9):
            g[y][x] = "i"
    for (cx, cy) in ((3, 5), (7, 5)):
        for y in range(h):
            for x in range(w):
                if math.hypot(x - cx, y - cy) <= 1.7:
                    g[y][x] = "."
    for x in (4, 5, 6):
        put(g, x, 7, ".")
    for x in (3, 5, 7):  # gaps between the teeth
        put(g, x, 10, ".")
        put(g, x, 11, ".")
    return rows(g)


def tube():
    w, h = 17, 7
    g = grid(w, h)
    for x in range(2, 15):
        put(g, x, 0, "y")
        put(g, x, 6, "y")
    for y in range(1, 6):
        put(g, 1, y, "y")
        put(g, 15, y, "y")
    for y in range(2, 5):
        put(g, 0, y, "y")
        put(g, 16, y, "y")
    for x in range(3, 13):
        put(g, x, 3, "y")
    return rows(g)


def flask():
    w, h = 11, 13
    g = grid(w, h)
    for y in range(h):
        if y < 4:
            xl, xr = 3, 7
        else:
            grow = (y - 4) / 8.0
            xl, xr = round(3 - 3 * grow), round(7 + 3 * grow)
        for x in range(xl, xr + 1):
            edge = x in (xl, xr) or y == h - 1
            liquid = y >= 8
            if edge or liquid:
                g[y][x] = "y"
    for x in range(2, 9):
        put(g, x, 0, "y")
    put(g, 4, 9, ".")
    put(g, 6, 11, ".")
    return rows(g)


def signal():
    w, h = 19, 11
    g = grid(w, h)
    cx, cy = 9, 10
    put(g, cx, cy, "y")
    put(g, cx - 1, cy, "y")
    put(g, cx + 1, cy, "y")
    put(g, cx, cy - 1, "y")
    for y in range(h):
        for x in range(w):
            dx, dy = x - cx, cy - y
            r = math.hypot(dx, dy)
            ang = math.degrees(math.atan2(dy, dx))
            if 38 <= ang <= 142:
                for rr in (4, 6.5, 9):
                    if abs(r - rr) <= 0.5:
                        g[y][x] = "y"
    return rows(g)


def drop():
    w, h = 9, 13
    g = grid(w, h)
    for y in range(h):
        for x in range(w):
            if y < 5:
                ok = abs(x - 4) <= y * 0.8
            else:
                ok = math.hypot(x - 4, y - 8.2) <= 4.3
            if ok:
                g[y][x] = "y"
    put(g, 2, 8, ".")
    put(g, 2, 9, ".")
    return rows(g)


def radio():
    w, h = 11, 16
    g = grid(w, h)
    for y in range(0, 4):
        put(g, 8, y, "y")
    put(g, 7, 0, "y")
    for y in range(4, 16):
        for x in range(1, 10):
            if x in (1, 9) or y in (4, 15):
                g[y][x] = "y"
    for x in range(3, 8):
        put(g, x, 6, "y")
    for y in (9, 11, 13):
        for x in (3, 5, 7):
            put(g, x, y, "y")
    return rows(g)


def plug():
    w, h = 9, 14
    g = grid(w, h)
    for y in range(0, 4):
        for x in (2, 3, 5, 6):
            put(g, x, y, "y")
    for y in range(4, 9):
        for x in range(9):
            if not (y in (4, 8) and x in (0, 8)):
                g[y][x] = "y"
    for y in range(9, 11):
        for x in range(2, 7):
            put(g, x, y, "y")
    for y in range(11, 14):
        for x in (3, 4):
            put(g, x, y, "y")
    return rows(g)


def bolt():
    w, h = 10, 15
    g = grid(w, h)
    pts = [(4.5, 0), (8, 0), (6, 5.5), (9, 5.5), (2, 15), (3.4, 8.5), (0.6, 8.5)]
    for y in range(h):
        for x in range(w):
            if poly_inside(x + 0.5, y + 0.5, pts):
                g[y][x] = "y"
    return rows(g)


def flower():
    w, h = 15, 18
    g = grid(w, h)
    centres = ((3, 3), (7, 2), (11, 3), (5, 6), (9, 6))
    for (cx, cy) in centres:
        for (dx, dy) in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            put(g, cx + dx, cy + dy, "p")
    for (cx, cy) in centres:
        put(g, cx, cy, "w")
    for y in range(8, 18):
        put(g, 7, y, "l")
    for (x, y) in ((6, 9), (5, 8), (8, 10), (9, 9), (6, 12), (5, 11), (4, 11), (8, 13), (9, 12), (10, 12),
                   (6, 15), (5, 14), (8, 16), (9, 15)):
        put(g, x, y, "l")
    return rows(g)


NEW = {"spr_trefoil": trefoil, "spr_atom": atom, "spr_tower": tower, "spr_mushroom": mushroom, "spr_skull": skull,
       "spr_tube": tube, "spr_flask": flask, "spr_signal": signal, "spr_drop": drop,
       "spr_plug": plug, "spr_flower": flower}

if __name__ == "__main__":
    src = open("src/ui.c", encoding="utf-8").read()
    for name, fn in NEW.items():
        r = fn()
        body = ", ".join('"%s"' % x for x in r)
        pat = re.compile(r"SPR\(%s,[^;]*\);" % name, re.S)
        assert pat.search(src), name
        src = pat.sub(lambda m: "SPR(%s, %s);" % (name, body), src, count=1)
    open("src/ui.c", "w", encoding="utf-8", newline="\n").write(src)
