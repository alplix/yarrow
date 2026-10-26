// Yarrow GC-03 desk dock, "Atom" version, two parts that press together without glue or screws:
//   dock  the base and the cradle for the counter (print it in black)
//   atom  a flat open plate with three electron orbits and the nucleus (print it lying flat, in yellow, no supports)
// The counter stands on its metal (tube) end, screen up, leaning back; the atom stands behind it, the orbits reaching out around
// the device. The atom has a stem with small barbs on its edges; it is pushed into a slot in the back of the dock and holds by
// friction. Only the lowest part of the case sits in the dock, so the selector knob, the buttons, the power key and the USB-C
// socket (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Make the STLs with   openscad -D 'part="dock"' ...   and   -D 'part="atom"'.
// Print the dock base down and the atom flat on the bed, 0.2 mm layers, PETG or PLA, 3 walls, 15 % infill, no supports for either.
// Each part has one colour, so there is no filament change at all.
// Measure the metal end of your counter with callipers and change the numbers in the first block; the clearance is added for you.

part = "both";     // "dock", "atom" or "both" (assembled, for looking at)

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
centre = 92;       // height of the nucleus along the lean (the lowest orbit point is centre - 55, it must stay above the dock)
plate_t = 6;       // thickness of the plate
nucleus_r = 7;
stem_w = 9;        // width of the stem
stem_bottom = 6;   // where the stem ends inside the dock (the dock top is at grip)
barb = 0.25;       // height of the barbs on the stem edges: the press fit
// ---- the joint
skin = 2.4;        // dock material between the pocket and the slot
fit = 0.15;        // play between stem and slot (barbs cut into this)
slot_v = plate_t + 0.3;
slot_u = stem_w + 2 * fit;
// ---- the base
base_h = 8;
side = 8;          // base margin at the sides
front = 9;         // base in front of the dock
tail = 22;         // base behind the dock: balances the lean
$fn = 72;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
bt = wall + pt + skin + slot_v + 3;
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
    // the stem with barbs: triangles on both edges, every 4 mm
    translate([-stem_w / 2, stem_bottom]) square([stem_w, centre - stem_bottom]);
    for (s = [-1, 1], z = [stem_bottom + 2 : 4 : grip - 2])
        polygon([[s * stem_w / 2, z - 1.6], [s * (stem_w / 2 + barb), z], [s * stem_w / 2, z + 1.6]]);
}

module atom() { linear_extrude(plate_t) atom2d(); }

module dock() {
    difference() {
        union() {
            slab();
            local() translate([-(pw + 2 * wall) / 2, 0, -16]) rrect(pw + 2 * wall, bt, grip + 16, 4);
        }
        local() {
            translate([-pw / 2, wall, 0]) cube([pw, pt, grip + 10]);                                  // the pocket
            translate([-slot_u / 2, wall + pt + skin, stem_bottom - 0.5]) cube([slot_u, slot_v, grip + 10]); // the slot
            translate([0, 0.8, 14]) rotate([90, 0, 0]) linear_extrude(2) trefoil(6);                  // the symbol on the front
        }
        translate([-100, -100, -50]) cube([400, 400, 50]);
    }
}

module trefoil(r) {
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

if (part == "dock") dock();
else if (part == "atom") atom();
else {
    dock();
    local() translate([0, wall + pt + skin + 0.15 + plate_t, 0]) rotate([90, 0, 0]) atom();
}
