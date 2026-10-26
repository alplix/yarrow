// Yarrow GC-03 desk dock, "Atom" version: the counter stands on its metal (tube) end, screen up, leaning back against a flat
// open plate that draws an atom behind it: three electron orbits and the nucleus. The orbits reach out around the device, so it
// looks like the nucleus of the atom. Only the lowest part of the case sits in the dock, so the selector knob, the buttons, the
// power key and the USB-C socket (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls, 15 % infill.
//
// Colours without a purge tower mess: two bands, one filament change:
//   black   from the bed to z = 8.0 mm     (the base)
//   white   above 8.0 mm                   (the dock and the atom)
// Change the numbers in the first block for another device; measure the metal end with callipers, the clearance is added for you.

// ---- the device (mm)
dev_w = 58;        // width of the case across the short edge
dev_t = 27;        // thickness of the metal end that goes into the dock
// ---- the dock
clr = 0.7;         // play on each side: 0.5 is snug, 1.0 is loose
wall = 3.2;
grip = 32;         // how far the device sits in the dock (keep it under the height of the selector knob, about 38 mm)
tilt = 10;         // backwards lean in degrees
// ---- the atom
orbit_a = 62;      // half the long axis of an orbit
orbit_b = 24;      // half the short axis
orbit_t = 4.5;     // width of the orbit strip
centre = 76;       // height of the nucleus along the lean (the device is about 135 mm long)
plate_t = 6;       // thickness of the plate
nucleus_r = 7;
// ---- the base
base_h = 8;
side = 8;          // base margin at the sides
front = 9;         // base in front of the dock
tail = 24;         // base behind the dock: balances the lean
$fn = 72;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
bt = wall + pt + plate_t;
bw = pw + 2 * wall + 2 * side;
bd = front + bt * cos(tilt) + tail;
rr = 6;

module rrect(w, d, h, rad) {
    hull() for (x = [rad, w - rad], y = [rad, d - rad]) translate([x, y, 0]) cylinder(r = rad, h = h);
}

module slab() {
    hull() {
        rrect(bw, bd, 3, rr);
        translate([3, 3, 3]) rrect(bw - 6, bd - 6, base_h - 3, rr - 1);
    }
}

// local axes: u across, v depth, w up the lean, origin at the front-bottom edge of the dock
module local() { translate([bw / 2, front, base_h]) rotate([-tilt, 0, 0]) children(); }

module ring2d() {
    difference() {
        scale([orbit_a, orbit_b]) circle(1);
        scale([orbit_a - orbit_t, orbit_b - orbit_t]) circle(1);
    }
}

module atom2d() {
    translate([0, centre]) {
        for (a = [0, 60, 120]) rotate(a) ring2d();
        circle(nucleus_r);
    }
    translate([-4.5, -16]) square([9, centre + 16]);        // the stem from the nucleus down into the dock
}

module block() {
    translate([-(pw + 2 * wall) / 2, 0, -16]) rrect(pw + 2 * wall, bt, grip + 16, 4);
    translate([0, bt, 0]) rotate([90, 0, 0]) linear_extrude(plate_t) atom2d();
}

module pocket() { translate([-pw / 2, wall, 0]) cube([pw, pt, grip + 10]); }

module trefoil(r) {
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

// the symbol on the front of the dock, engraved 0.8 mm
module engrave_front() { translate([0, 0.8, 14]) rotate([90, 0, 0]) linear_extrude(2) trefoil(6); }

difference() {
    union() { slab(); local() block(); }
    local() { pocket(); engrave_front(); }
    translate([-100, -100, -50]) cube([300, 300, 50]);
}
