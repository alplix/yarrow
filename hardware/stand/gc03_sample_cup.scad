// Sample cup that drops into the Yarrow GC-03 dock: it turns the dock into a fixed place for the thing you measure (a coin, a
// mineral, uranium glass, an old watch hand, a soil sample), so that the "sample test" of the counter always sees the sample in
// the same place. The dock leans back by 10 degrees; the cup is cut so that its top is level when it sits in the dock, and the
// round recess is vertical, so a flat sample lies in it without sliding.
// Free to print and share (GPL-3.0-or-later). Print it as it is (flat floor down), 0.2 mm layers, any colour, no supports.
// Change the first block to the pocket of your dock (gc03_stand.scad: pw = dev_w + 2 * clr, pt = dev_t + 2 * clr, grip, tilt).

pocket_w = 59.4;   // inner width of the pocket of the dock
pocket_t = 28.4;   // inner depth of the pocket
tilt = 10;         // the lean of the dock
play = 0.35;       // play on each side, so that it goes in and comes out easily (turn the dock upside down and it drops out)
front_h = 22;      // height of the front edge of the cup along the pocket
recess_d = 22;     // diameter of the round recess (a coin of up to 20 mm); the cup is only 27 mm deep, so it cannot be much bigger
recess_depth = 12; // its depth, measured straight down
$fn = 96;

cw = pocket_w - 2 * play;
cd = pocket_t - 2 * play;
top_slope = tan(tilt);   // height gained per mm towards the back

difference() {
    // the block, cut by the level plane
    intersection() {
        translate([-cw / 2, 0, 0]) cube([cw, cd, front_h + cd * top_slope + 1]);
        // everything below the level plane: w = front_h + v * tan(tilt)
        translate([0, 0, 0]) rotate([tilt, 0, 0]) translate([-cw, -cd * 2, -50]) cube([cw * 2, cd * 4, 50 + front_h * cos(tilt) + 0.001]);
    }
    // the round recess, vertical in the world: axis tilted forward by the lean
    translate([0, cd / 2, front_h + cd / 2 * top_slope])
        rotate([tilt, 0, 0]) translate([0, 0, -recess_depth]) cylinder(d = recess_d, h = recess_depth + 10);
}
