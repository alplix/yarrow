#include "bat.h"
#include <stdio.h>
#include <stdlib.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* voltage of the model cell at a given charge level (tenths of a percent), inverse of the curve */
static uint32_t mv_at(uint32_t level_x10)
{
    uint32_t lo = 3300, hi = 4200;
    while (hi - lo > 1) {
        uint32_t mid = (lo + hi) / 2;
        if (bat_level_from_mv(mid) < level_x10)
            lo = mid;
        else
            hi = mid;
    }
    return hi;
}

/* charging: the shown level must follow the cell, not the voltage inflated by the charge current */
static void test_charging(void)
{
    bat_t b;
    bat_init(&b, 0);
    uint32_t cell_x10 = 400;                          /* 40 % to start */
    for (int s = 0; s < 120; s++)                    /* two minutes at 1 A: terminal is 0.12 V above the cell */
        bat_tick(&b, mv_at(cell_x10) + 120, true, true);
    CHECK(bat_percent(&b) >= 35 && bat_percent(&b) <= 50, "charging starts at %u %%, cell is at 40 %%", (unsigned)bat_percent(&b));
    for (int s = 0; s < 1200; s++)                   /* 20 more minutes: the cell gains about 7 % */
        bat_tick(&b, mv_at(400 + s * 70 / 1200) + 120, true, true);
    CHECK(bat_percent(&b) >= 40 && bat_percent(&b) <= 60, "after 22 minutes %u %%", (unsigned)bat_percent(&b));
    for (int s = 0; s < 4000; s++)                   /* constant-voltage stage */
        bat_tick(&b, 4200, true, true);
    CHECK(bat_percent(&b) >= 90 && bat_percent(&b) <= 99, "end of the constant-voltage stage %u %%", (unsigned)bat_percent(&b));
    bat_tick(&b, 4200, true, false);
    CHECK(bat_percent(&b) == 100, "done: %u %%", (unsigned)bat_percent(&b));
}

static void test_curve(void)
{
    uint32_t prev = 0;
    for (uint32_t mv = 3200; mv <= 4300; mv += 10) {
        uint32_t l = bat_level_from_mv(mv);
        CHECK(l >= prev, "curve not monotonic at %u mV", (unsigned)mv);
        prev = l;
    }
    CHECK(bat_level_from_mv(3000) == 0 && bat_level_from_mv(4300) == 1000, "curve ends");
}

static uint32_t rnd_state = 12345;
static int noise(int amp)
{
    rnd_state = rnd_state * 1664525u + 1013904223u;
    return (int)((rnd_state >> 16) % (2u * amp + 1u)) - amp;
}

/* discharge at a constant drain, in percent per hour times ten */
static void discharge(bat_t *b, double hours, double pct_per_h, double *level, int *max_est_err_pct_after_2h,
                      double true_total_h)
{
    int seconds = (int)(hours * 3600);
    for (int s = 0; s < seconds; s++) {
        *level -= pct_per_h / 3600.0 * 10.0; /* tenths of a percent */
        if (*level < 0)
            *level = 0;
        uint32_t mv = mv_at((uint32_t)*level) + (uint32_t)(0 + noise(15) + 15 - 15);
        bat_tick(b, mv, false, false);
        if (s > 4 * 3600 && max_est_err_pct_after_2h) {
            double true_left_min = (*level / 10.0) / pct_per_h * 60.0;
            double est = bat_remaining_minutes(b);
            if (true_left_min > 120) {
                int err = (int)(100.0 * (est - true_left_min) / true_left_min);
                if (abs(err) > *max_est_err_pct_after_2h)
                    *max_est_err_pct_after_2h = abs(err);
            }
        }
    }
    (void)true_total_h;
}

static void test_discharge(void)
{
    bat_t b;
    bat_init(&b, 0);
    /* charged and on USB first */
    for (int i = 0; i < 60; i++)
        bat_tick(&b, 4180, true, false);
    CHECK(b.state == BAT_FULL && bat_percent(&b) >= 98, "full on USB: %u%%", (unsigned)bat_percent(&b));

    double level = 1000;
    int err = 0;
    discharge(&b, 20, 100.0 / 30.0, &level, &err, 30); /* a 30 hour battery, 20 hours in */
    printf("discharge: %u%% left, %u min remaining (true %.0f), worst estimate error after 4 h: %d%%, health %u%%, cycles %u\n",
           (unsigned)bat_percent(&b), (unsigned)bat_remaining_minutes(&b), level / 10.0 / (100.0 / 30.0) * 60.0, err,
           (unsigned)bat_health_percent(&b), (unsigned)bat_cycles(&b));
    CHECK(err < 12, "remaining-time estimate error %d%%", err);
    CHECK(bat_percent(&b) >= 30 && bat_percent(&b) <= 40, "percent after 20 h of 30: %u", (unsigned)bat_percent(&b));
    CHECK(bat_health_percent(&b) >= 90, "a healthy cell should read about 100: %u", (unsigned)bat_health_percent(&b));
}

static void test_monotonic_and_cycles(void)
{
    bat_t b;
    bat_stats_t st = {0, 0, 0};
    bat_init(&b, &st);
    uint32_t last = 100;
    double level = 1000;
    for (int s = 0; s < 10 * 3600; s++) {
        level -= 0.02;
        bat_tick(&b, mv_at((uint32_t)level) + (uint32_t)(15 + noise(15)), false, false);
        uint32_t p = bat_percent(&b);
        CHECK(p <= last, "percent rose while discharging (%u -> %u)", (unsigned)last, (unsigned)p);
        last = p;
    }
    /* about 72 percent were used */
    CHECK(bat_cycles(&b) == 0, "cycle count should still be 0: %u", (unsigned)bat_cycles(&b));
    for (int i = 0; i < 20; i++) /* charge */
        bat_tick(&b, 4180, true, true);
    CHECK(b.state == BAT_CHARGING, "charging state");
    level = 1000;
    for (int s = 0; s < 40 * 3600; s++) {
        level -= 0.0174;
        if (level < 50) {
            level = 1000; /* recharge instantly in the model */
            for (int i = 0; i < 600; i++)
                bat_tick(&b, 4190, true, false);
        }
        bat_tick(&b, mv_at((uint32_t)level) + (uint32_t)(15 + noise(15)), false, false);
    }
    printf("after about 2.5 cycles of use: cycles %u\n", (unsigned)bat_cycles(&b));
    CHECK(bat_cycles(&b) >= 2 && bat_cycles(&b) <= 4, "cycles %u", (unsigned)bat_cycles(&b));
}

static void test_wear(void)
{
    /* a cell that once lasted 40 h and now lasts 28 h should read about 70 percent */
    bat_stats_t st = {0, 0, 400};
    bat_t b;
    bat_init(&b, &st);
    for (int i = 0; i < 60; i++)
        bat_tick(&b, 4180, true, false);
    double level = 1000;
    discharge(&b, 12, 100.0 / 28.0, &level, 0, 28);
    printf("worn cell: health %u%% (expected near 70)\n", (unsigned)bat_health_percent(&b));
    CHECK(bat_health_percent(&b) >= 60 && bat_health_percent(&b) <= 80, "health %u", (unsigned)bat_health_percent(&b));
}

int main(void)
{
    test_curve();
    test_charging();
    test_discharge();
    test_monotonic_and_cycles();
    test_wear();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall bat checks passed\n", failures);
    return failures ? 1 : 0;
}
