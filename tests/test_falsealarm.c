/* Start-up must not raise an alarm out of a handful of counts: boot many times at natural background and count alarms. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"
#include "meas.h"

static uint64_t rs = 0x1234567ull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 11); }
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }
static uint32_t poisson(double lam)
{
    double L = exp(-lam), p = 1.0; uint32_t k = 0;
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int boots_with_alarm(double cps, int boots, int seconds)
{
    int alarms = 0;
    for (int b = 0; b < boots; b++) {
        meas_t m;
        meas_cfg_t mc = {8000, 90};
        meas_init(&m, &mc);
        alarm_t a;
        alarm_cfg_t ac = {{500, 10000, 100000}};
        alarm_init(&a, &ac);
        bool raised = false;
        for (int s = 0; s < seconds; s++) {
            meas_tick(&m, poisson(cps));
            alarm_tick(&a, 1000000u + (uint32_t)s, meas_reliable(&m) ? meas_rate_nsvh(&m) : 0, meas_dose_nsv(&m));
            if (a.level > 0)
                raised = true;
        }
        alarms += raised;
    }
    return alarms;
}

int main(void)
{
    /* natural background (0.12 uSv/h is 9.6 cpm) and a 3 times higher but still harmless 0.35 uSv/h (28 cpm) */
    int a1 = boots_with_alarm(0.16, 3000, 900);
    int a2 = boots_with_alarm(0.47, 3000, 900);
    printf("false alarms: %d of 3000 boots at 0.12 uSv/h, %d of 3000 at 0.35 uSv/h\n", a1, a2);
    CHECK(a1 == 0, "alarms at natural background: %d", a1);
    CHECK(a2 <= 15, "alarms at 0.35 uSv/h: %d", a2); /* 70 percent of the first level: a rare fluke may touch it, never the background */

    /* a real source must still raise the alarm quickly: 5 uSv/h is 400 cpm */
    meas_t m;
    meas_cfg_t mc = {8000, 90};
    meas_init(&m, &mc);
    alarm_t a;
    alarm_cfg_t ac = {{500, 10000, 100000}};
    alarm_init(&a, &ac);
    int first = -1;
    for (int s = 0; s < 120 && first < 0; s++) {
        meas_tick(&m, poisson(400.0 / 60.0));
        alarm_tick(&a, 1000000u + (uint32_t)s, meas_reliable(&m) ? meas_rate_nsvh(&m) : 0, meas_dose_nsv(&m));
        if (a.level > 0)
            first = s;
    }
    printf("a 5 uSv/h source raised the alarm after %d s\n", first);
    CHECK(first >= 0 && first <= 20, "alarm too late or never: %d", first);

    printf(failures ? "\n%d check(s) FAILED\n" : "\nall false alarm checks passed\n", failures);
    return failures ? 1 : 0;
}
