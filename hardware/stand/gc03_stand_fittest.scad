// Fit test for gc03_stand.scad: just the opening of the dock as a 10 mm ring, about 6 g. Print it first, push the metal end of the
// GC-03 into it; it should go in with a little play and not rock. Keep dev_w, dev_t and clr equal to gc03_stand.scad.
dev_w = 58;
dev_t = 27;
clr = 0.7;
wall = 3.2;
difference() {
    translate([-(dev_w + 2 * clr) / 2 - wall, 0, 0]) cube([dev_w + 2 * clr + 2 * wall, dev_t + 2 * clr + 2 * wall, 10]);
    translate([-(dev_w + 2 * clr) / 2, wall, -1]) cube([dev_w + 2 * clr, dev_t + 2 * clr, 12]);
}
