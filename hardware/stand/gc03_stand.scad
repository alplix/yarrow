// Yarrow GC-03 desk dock, "Bench instrument" version: the counter stands in the top of a small instrument cabinet, screen up,
// leaning back a little, like the probe of a laboratory meter. The front of the cabinet carries a nameplate, a vent grille,
// a dial and four screw heads (all engraved 0.8 mm). Only the lowest part of the case sits in the dock, so the selector knob,
// the buttons, the power key and the USB-C socket (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls,
// 15 % infill.
//
// Colours without a purge tower mess: three bands, two filament changes:
//   black   from the bed to z = 8.0 mm     (the two-tier base)
//   white   from 8.0 mm to z = 56.0 mm     (the cabinet)
//   yellow  above 56.0 mm                  (the top trim)
// Change the numbers in the first block for another device; measure the metal end with callipers, the clearance is added for you.

// ---- the device (mm)
dev_w = 58;        // width of the case across the short edge
dev_t = 27;        // thickness of the metal end that goes into the dock
// ---- the dock
clr = 0.7;         // play on each side: 0.5 is snug, 1.0 is loose
grip = 34;         // how far the device sits in the cabinet (keep it well under the height of the selector knob, about 38 mm)
tilt = 10;         // backwards lean in degrees
// ---- the cabinet
cab_w = 98;
cab_d = 68;
cab_h = 52;        // above the base
base_h = 8;        // two tiers, black
margin = 3;        // base sticks out this much around the cabinet
front_wall = 15;   // material in front of the pocket
$fn = 64;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
bw = cab_w + 2 * margin;
bd = cab_d + 2 * margin;
rr = 7;

module rrect(w, d, h, rad) {
    hull() for (x = [rad, w - rad], y = [rad, d - rad]) translate([x, y, 0]) cylinder(r = rad, h = h);
}

module slab() {
    hull() {
        rrect(bw, bd, 3, rr);
        translate([3, 3, 3]) rrect(bw - 6, bd - 6, base_h - 3, rr - 1);
    }
}

module cabinet() {
    // rounded corners, a 3 mm chamfer round the top edge
    translate([margin, margin, base_h - 0.01]) hull() {
        rrect(cab_w, cab_d, cab_h - 3, rr - 2);
        translate([3, 3, 0]) rrect(cab_w - 6, cab_d - 6, cab_h, rr - 4);
    }
}

// the pocket stands on its front-bottom edge and leans back
module pocket() {
    translate([bw / 2, margin + front_wall, base_h + cab_h - grip]) rotate([-tilt, 0, 0]) translate([-pw / 2, 0, 0]) cube([pw, pt, grip + 90]);
}

module trefoil(r) {
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

module screwhead(x, y) {
    translate([x, y]) {
        difference() { circle(3.2); circle(2.2); }
        rotate(35) square([4.4, 0.9], center = true);
    }
}

// the front of the cabinet: x across from the centre, y up from the foot of the cabinet
module front_art() {
    // double frame: the groove between two outlines
    difference() {
        translate([-(cab_w - 14) / 2, 4]) square([cab_w - 14, cab_h - 8]);
        translate([-(cab_w - 14) / 2 + 1.4, 5.4]) square([cab_w - 14 - 2.8, cab_h - 10.8]);
    }
    for (x = [-38, 38], y = [10, cab_h - 10]) screwhead(x, y);
    // vent grille: six slots
    for (i = [0 : 5]) translate([-38, 13 + i * 5.2]) square([22, 2]);
    // nameplate with the symbol and the name
    translate([-11, 12]) difference() { square([22, cab_h - 24]); translate([1.2, 1.2]) square([19.6, cab_h - 26.4]); }
    translate([0, cab_h / 2 + 5]) trefoil(5.2);
    translate([0, cab_h / 2 - 8]) text("YARROW", size = 3, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
    // dial: rings, a pointer and ticks
    translate([27, cab_h / 2]) {
        difference() { circle(9); circle(7.8); }
        difference() { circle(4.2); circle(3.2); }
        rotate(55) translate([0, -0.5]) square([7.5, 1]);
        for (a = [-120 : 30 : 120]) rotate(90 + a) translate([10.4, -0.4]) square([2, 0.8]);
    }
}

module engrave_front() { translate([bw / 2, margin + 0.8, base_h]) rotate([90, 0, 0]) linear_extrude(2) front_art(); }

difference() {
    union() { slab(); cabinet(); }
    pocket();
    engrave_front();
    translate([-1, -1, -50]) cube([bw + 2, bd + 2, 50]);
}
