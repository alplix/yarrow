// Yarrow GC-03 desk dock, "Cooling tower" version: the counter stands in the top of a small cooling tower, screen up, leaning back a
// little, the way the steam leaves a real one. The tower is a true hyperboloid, and the lattice engraved on it is made of its
// straight lines (a hyperboloid is ruled: that is how real cooling towers are built from straight rods). The radiation symbol is on
// the rim. Only the lowest part of the case sits in the dock, so the selector knob, the buttons, the power key and the USB-C socket
// (all higher up) stay free.
// Free to print and share (GPL-3.0-or-later). Print as it is, base down, no supports, 0.2 mm layers, PETG or PLA, 3 walls, 15 % infill.
//
// Colours without a purge tower mess: three bands, two filament changes:
//   black   from the bed to z = 8.0 mm     (the two-tier base)
//   white   from 8.0 mm to z = 56.0 mm     (the tower)
//   yellow  above 56.0 mm                  (the rim)
// Change the numbers in the first block for another device; measure the metal end with callipers, the clearance is added for you.

// ---- the device (mm)
dev_w = 58;        // width of the case across the short edge
dev_t = 27;        // thickness of the metal end that goes into the dock
// ---- the dock
clr = 0.7;         // play on each side: 0.5 is snug, 1.0 is loose
grip = 34;         // how far the device sits in the tower (keep it well under the height of the selector knob, about 38 mm)
tilt = 10;         // backwards lean in degrees
// ---- the tower
tower_h = 52;      // above the base
waist_r = 38;      // radius at the narrowest point
waist_z = 34;      // height of the waist above the base
top_r = 42;        // radius at the top
base_h = 8;        // two tiers, black
rim_h = 4;         // the yellow rim
ribs = 24;         // straight lines of each direction in the lattice
$fn = 96;

pw = dev_w + 2 * clr;
pt = dev_t + 2 * clr;
k = sqrt((top_r * top_r - waist_r * waist_r) / pow(tower_h - waist_z, 2));   // slope of the straight lines
function rad(z) = sqrt(waist_r * waist_r + k * k * pow(z - waist_z, 2));
base_r = rad(0) + 5;

// the point at height z on the straight line that crosses the waist at angle phi (dir = -1 or 1 for the two directions)
function pt3(phi, dir, z) = [waist_r * cos(phi) - dir * k * (z - waist_z) * sin(phi),
                             waist_r * sin(phi) + dir * k * (z - waist_z) * cos(phi),
                             base_h + z];

module tower() {
    translate([0, 0, base_h - 0.01])
        rotate_extrude() polygon(concat([[0, 0]], [for (z = [0 : 2 : tower_h]) [rad(z), z]], [[0, tower_h]]));
}

module slab() {
    hull() {
        cylinder(r = base_r, h = 3);
        translate([0, 0, 3]) cylinder(r = base_r - 3, h = base_h - 3);
    }
}

// the pocket leans back around the middle of its depth, so that its corners stay inside the tower wall
module pocket() {
    translate([0, -grip * sin(tilt) / 2, base_h + tower_h - grip]) rotate([-tilt, 0, 0]) translate([-pw / 2, -pt / 2, 0]) cube([pw, pt, grip + 60]);
}

module trefoil(r) {
    for (a = [90, 210, 330]) rotate(a - 30)
        intersection() {
            difference() { circle(r); circle(r * 0.28); }
            polygon([[0, 0], [r * 2, 0], [r * 2 * cos(60), r * 2 * sin(60)]]);
        }
    circle(r * 0.16);
}

// a thin groove along one straight line, from z0 to z1
module rod(phi, dir, z0, z1) {
    hull() {
        translate(pt3(phi, dir, z0)) sphere(r = 0.7, $fn = 8);
        translate(pt3(phi, dir, z1)) sphere(r = 0.7, $fn = 8);
    }
}

module lattice() {
    for (i = [0 : ribs - 1], dir = [-1, 1]) rod(i * 360 / ribs, dir, 4, tower_h - rim_h - 1);
}

// engraved 0.8 mm into the top of the rim, in front of the pocket
module rim_symbol() {
    translate([0, -30, base_h + tower_h - 0.8]) linear_extrude(2) trefoil(5.5);
}

difference() {
    union() { slab(); tower(); }
    pocket();
    lattice();
    rim_symbol();
    translate([-100, -100, -50]) cube([200, 200, 50]);
}
