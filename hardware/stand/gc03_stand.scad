// Yarrow GC-03 desk dock, "Fan" version: the counter stands on its metal (tube) end, screen up, leaning back against a fan-shaped
// plate that opens up from the dock like a folding fan, with ribs around the device and the radiation symbol at the top. Only the lowest part of the case sits in the
// dock, so the selector knob, the buttons, the power key and the USB-C socket (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls,
// 15 % infill (more in the base is better: weight keeps it from tipping).
//
// Colours without a purge tower mess: three bands, two filament changes:
//   black   from the bed to z = 8.0 mm     (the two-tier base)
//   white   from 8.0 mm to z = 147.0 mm    (dock and plate)
//   yellow  above 147.0 mm                 (the top edge of the fan with the symbol)
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
fan_apex = 40;     // the ribs meet this far below the bottom of the dock
fan_r = 208;       // radius from that point (plate top is at fan_r - fan_apex above the dock floor)
fan_a = 24;        // half the opening angle of the fan, degrees
plate_w = 82;      // width of the base and the dock block follow from this
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

// a sector opening upwards from (0, -fan_apex), with rounded corners
module fan2d() {
    offset(r = 4) offset(delta = -4)
        intersection() {
            translate([0, -fan_apex]) circle(fan_r);
            polygon(concat([[0, -fan_apex]], [for (t = [-fan_a : 2 : fan_a]) [(fan_r + 10) * sin(t), -fan_apex + (fan_r + 10) * cos(t)]]));
        }
}

module slab() {
    hull() {
        rrect(bw, bd, 3, rr);
        translate([3, 3, 3]) rrect(bw - 6, bd - 6, base_h - 3, rr - 1);
    }
}

module block() {
    // the dock block (from below the base up to grip + 4) and the arched plate behind the pocket
    translate([-(pw + 2 * wall) / 2, 0, -16]) rrect(pw + 2 * wall, bt, grip + 20, 4);
    translate([0, bt, 0]) rotate([90, 0, 0]) linear_extrude(plate_t) fan2d();
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

// engraved 0.8 mm into the front face of the plate (the face the device leans on): ribs from the fan's centre, two arcs, a frame
module sunburst() {
    med = fan_r - fan_apex - 16;                  // the symbol sits near the top edge
    field = rim;
    intersection() {
        offset(delta = -field) fan2d();
        difference() {
            union() {
                // ribs every 3 degrees, from above the dock
                for (a = [-fan_a : 3 : fan_a]) translate([0, -fan_apex]) rotate(90 - a) translate([95, -0.6]) square([200, 1.2]);
                // two arcs across the ribs
                for (r = [130, 170]) translate([0, -fan_apex]) difference() { circle(r + 0.6); circle(r - 0.6); }
            }
            translate([0, med]) circle(14);
        }
    }
    difference() { offset(delta = -field) fan2d(); offset(delta = -field - 1.4) fan2d(); }
    translate([0, med]) {
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
