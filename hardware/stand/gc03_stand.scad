// Yarrow GC-03 desk dock, "Sunrise" version: the counter stands on its metal (tube) end, screen up, leaning back against an arched
// plate with a sunburst around the device and the radiation symbol in the crown. Only the lowest part of the case sits in the
// dock, so the selector knob, the buttons, the power key and the USB-C socket (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls,
// 15 % infill (more in the base is better: weight keeps it from tipping).
//
// Colours without a purge tower mess: three bands, two filament changes:
//   black   from the bed to z = 8.0 mm     (the two-tier base)
//   white   from 8.0 mm to z = 146.0 mm    (dock and plate)
//   yellow  above 146.0 mm                 (the crown with the symbol)
// Change the numbers in the first block for another device; measure the metal end with callipers, the clearance is added for you.

// ---- the device (mm)
dev_w = 58;        // width of the case across the short edge
dev_t = 27;        // thickness of the metal end that goes into the dock
// ---- the dock
clr = 0.7;         // play on each side: 0.5 is snug, 1.0 is loose
wall = 3.2;
grip = 30;         // how far the device sits in the dock (keep it well under the height of the selector knob, about 38 mm)
tilt = 10;         // backwards lean in degrees
front_h = 15;      // the front wall is lowered to this height with a round scoop
// ---- the plate and the base
plate_w = 82;
plate_h = 170;     // height along the lean (the device is about 135 mm long)
plate_t = 7;
rim = 5;           // solid frame around the sunburst
base_h = 8;        // two tiers, black
side = 4;          // base margin at the sides
front = 9;         // base in front of the dock
tail = 22;         // base behind the plate: balances the lean
$fn = 64;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
bt = wall + pt + plate_t;                  // dock depth including the plate
bw = plate_w + 2 * side;
bd = front + bt * cos(tilt) + tail;
rr = 6;

module rrect(w, d, h, rad) {
    hull() for (x = [rad, w - rad], y = [rad, d - rad]) translate([x, y, 0]) cylinder(r = rad, h = h);
}

// the dock stands on its front-bottom edge at (bw/2, front, base_h) and leans back by tilt; local axes: u across, v depth, w up the lean
module local() { translate([bw / 2, front, base_h]) rotate([-tilt, 0, 0]) children(); }

module arch2d(w, h) { hull() { translate([-w / 2, 0]) square([w, 0.01]); translate([0, h - w / 2]) circle(d = w); } }

module slab() {
    hull() {
        rrect(bw, bd, 3, rr);
        translate([3, 3, 3]) rrect(bw - 6, bd - 6, base_h - 3, rr - 1);
    }
}

module block() {
    // the dock block (from below the base up to grip + 4) and the arched plate behind the pocket
    translate([-(pw + 2 * wall) / 2, 0, -16]) rrect(pw + 2 * wall, bt, grip + 20, 4);
    translate([0, bt, -16]) rotate([90, 0, 0]) linear_extrude(plate_t) arch2d(plate_w, plate_h + 16);
}

module pocket() { translate([-pw / 2, wall, 0]) cube([pw, pt, grip + 10]); }

module scoop() { translate([0, -1, front_h + 22]) rotate([-90, 0, 0]) cylinder(r = 22, h = wall + 2); }

module trefoil(r) {
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

// engraved 0.8 mm into the front face of the plate (the face the device leans on)
module sunburst() {
    cy = 105;                                   // the rays start behind the device
    intersection() {
        offset(delta = -rim) arch2d(plate_w, plate_h);
        difference() {
            union() for (a = [0 : 7.5 : 359]) translate([0, cy]) rotate(a) translate([14, -0.6]) square([200, 1.2]);
            translate([0, cy]) circle(14);
        }
    }
    difference() { offset(delta = -rim) arch2d(plate_w, plate_h); offset(delta = -rim - 1.4) arch2d(plate_w, plate_h); }
    // the symbol in the crown, in a ring
    translate([0, plate_h - 24]) {
        difference() { circle(10.5); circle(9.3); }
        trefoil(6.4);
    }
}

module engrave_plate() { translate([0, wall + pt + 0.8, 0]) rotate([90, 0, 0]) linear_extrude(2.4) sunburst(); }

// the small symbol on the front of the dock block
module engrave_front() { translate([0, 0.8, 9]) rotate([90, 0, 0]) linear_extrude(2) trefoil(4.5); }

difference() {
    union() {
        slab();
        local() block();
    }
    local() { pocket(); scoop(); engrave_plate(); engrave_front(); }
    translate([-1, -1, -50]) cube([bw + 2, bd + 2, 50]);
}
