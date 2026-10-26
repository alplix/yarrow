// Yarrow GC-03 desk dock: the counter stands on its metal (tube) end like a small desk clock, screen up, leaning back a little.
// Only the lowest part of the case sits in the dock, so the selector knob, the buttons, the power key and the USB-C socket (all
// higher up) stay free and the cable hangs out in the open. Free to print and share (GPL-3.0-or-later).
// Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls, 15 % infill.
// Change the numbers in the first block for another device; measure the metal end with callipers, the clearance is added for you.

// ---- the device (mm)
dev_w = 58;        // width of the case across the short edge
dev_t = 27;        // thickness of the metal end that goes into the dock
knob_z = 38;       // height of the selector knob above the bottom end: the dock must stay lower than this
// ---- the dock
clr = 0.7;         // play on each side: 0.5 is snug, 1.0 is loose
wall = 3.2;
grip = 30;         // how far the device sits in the dock (keep it under knob_z - 4)
tilt = 12;         // backwards lean in degrees
front_h = 15;      // the front wall is lowered to this height with a round scoop
floor_t = 4;       // material under the device
side = 6;          // base margin at the sides and in front of the dock
tail = 16;         // base behind the dock, it balances the lean
$fn = 64;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
bw = pw + 2 * wall + 2 * side;
bd = side + pt + 2 * wall + tail + grip * sin(tilt);
rr = 5;            // outside rounding

module rrect(w, d, h, rad) {
    hull() for (x = [rad, w - rad], y = [rad, d - rad]) translate([x, y, 0]) cylinder(r = rad, h = h);
}

// the dock block stands on its front-bottom edge at (bw/2, side) and leans back by tilt
module tilted(h0 = 0) { translate([bw / 2, side, floor_t]) rotate([-tilt, 0, 0]) children(); }

module body() {
    hull() {
        rrect(bw, bd, 0.6, rr);                                             // the footprint
        tilted() translate([-(pw + 2 * wall) / 2, 0, 0]) rrect(pw + 2 * wall, pt + 2 * wall, grip + 4, 4); // the block on top
    }
}

module pocket() {
    tilted() translate([-pw / 2, wall, 0]) cube([pw, pt, grip + 10]);
}

module scoop() {
    // a round notch in the front wall: the wall is only front_h high in the middle and rises to the sides
    tilted() translate([0, -1, front_h + 22]) rotate([-90, 0, 0]) cylinder(r = 22, h = wall + 2);
}

module trefoil(r) {
    // the radiation symbol: three 60 degree blades, a ring and a centre disc
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

module engrave() {
    // the symbol on the sloping front face, 0.8 mm deep and square to the face
    h = grip + floor_t + 4;
    ang = atan2(side, h);
    zc = 11;
    yf = side * (zc - 0.6) / (h - 0.6);
    translate([bw / 2, yf, zc]) rotate([-ang, 0, 0]) rotate([90, 0, 0]) translate([0, 0, -0.8]) linear_extrude(3) trefoil(4.5);
}

difference() {
    body();
    pocket();
    scoop();
    engrave();
    translate([-1, -1, -50]) cube([bw + 2, bd + 2, 50]);
}
