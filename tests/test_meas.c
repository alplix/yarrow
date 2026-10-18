#include "meas.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t rng = 0x1234567899ABCDEFull;
static uint32_t rnd(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 11);
}
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }
static uint32_t poisson(double lam)
{
    if (lam > 500) { /* normal approximation, fine for the test */
        double g = sqrt(-2 * log(uni())) * cos(6.283185307 * uni());
        double v = lam + sqrt(lam) * g;
        return v < 0 ? 0 : (uint32_t)(v + 0.5);
    }
    double L = exp(-lam), p = 1.0;
    uint32_t k = 0;
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static const meas_cfg_t cfg = {8000, 0};

static void run(meas_t *m, double cps, int seconds)
{
    for (int i = 0; i < seconds; i++)
        meas_tick(m, poisson(cps));
}

static void test_steady(double cpm, double tol_pct)
{
    meas_t m;
    meas_init(&m, &cfg);
    run(&m, cpm / 60.0, 900);
    /* average the estimate over a further stretch to check bias, and watch the spread */
    double sum = 0, sum2 = 0;
    int n = 60000; /* estimates are strongly correlated, so a long run is needed to see the bias */
    for (int i = 0; i < n; i++) {
        meas_tick(&m, poisson(cpm / 60.0));
        double v = meas_cpm(&m);
        sum += v;
        sum2 += v * v;
    }
    double mean = sum / n, sd = sqrt(sum2 / n - mean * mean);
    printf("steady %7.1f CPM: estimate %.1f, spread %.1f%%, window %us, %.3f uSv/h\n", cpm, mean, 100 * sd / mean,
           (unsigned)m.window_s, meas_rate_nsvh(&m) / 1000.0);
    CHECK(fabs(mean - cpm) / cpm * 100 < tol_pct, "bias too large at %.1f CPM (%.1f)", cpm, mean);
}

static void test_step(void)
{
    /* 10 CPM, then a jump to 1000 CPM: the estimate must follow within seconds. */
    meas_t m;
    meas_init(&m, &cfg);
    run(&m, 10 / 60.0, 600);
    unsigned long seen = 0;
    int reach = -1;
    for (int t = 1; t <= 60; t++) {
        meas_tick(&m, poisson(1000 / 60.0));
        seen = meas_cpm(&m);
        if (reach < 0 && seen > 700)
            reach = t;
    }
    printf("step 10 -> 1000 CPM: estimate above 700 CPM after %d s, window now %u s, %lu CPM\n", reach,
           (unsigned)m.window_s, seen);
    CHECK(reach > 0 && reach <= 8, "step response too slow: %d s", reach);
    CHECK(seen > 850 && seen < 1150, "step result off: %lu", seen);

    /* and back down */
    reach = -1;
    for (int t = 1; t <= 120; t++) {
        meas_tick(&m, poisson(10 / 60.0));
        if (reach < 0 && meas_cpm(&m) < 100)
            reach = t;
    }
    printf("step 1000 -> 10 CPM: estimate below 100 CPM after %d s\n", reach);
    CHECK(reach > 0 && reach <= 30, "step down too slow: %d s", reach);
}

static void test_dead_time(void)
{
    bool sat = false;
    /* 100 us dead time: 5000 measured per second really was 5000 / (1 - 0.5) = 10000 */
    uint32_t c = meas_correct_cps_x100(5000, 100, &sat);
    CHECK(c == 1000000 && !sat, "dead time 5000 cps: %u", (unsigned)c);
    c = meas_correct_cps_x100(100, 100, &sat);
    CHECK(c > 10000 && c < 10200, "dead time 100 cps: %u", (unsigned)c);
    c = meas_correct_cps_x100(9500, 100, &sat);
    CHECK(sat, "should report saturation at 95 percent dead time");
    (void)c;
    c = meas_correct_cps_x100(0, 100, &sat);
    CHECK(c == 0, "zero pulses");
}

static void test_dose(void)
{
    /* 1 uSv/h = 80 CPM: one hour of it adds 1000 nSv */
    meas_t m;
    meas_init(&m, &cfg);
    for (int i = 0; i < 3600; i++)
        meas_tick(&m, (i % 3) == 0 ? 2 : 1); /* 80 CPM = 4800 counts per hour; 1200*2 + 2400*1 = 4800 */
    uint64_t d = meas_dose_nsv(&m);
    printf("dose after 1 h at 1 uSv/h: %llu nSv\n", (unsigned long long)d);
    CHECK(d >= 990 && d <= 1010, "dose %llu nSv", (unsigned long long)d);
    CHECK(meas_rate_nsvh(&m) > 900 && meas_rate_nsvh(&m) < 1100, "rate %u", (unsigned)meas_rate_nsvh(&m));
}

static void test_extremes(void)
{
    meas_t m;
    meas_cfg_t hot = {8000, 100};
    meas_init(&m, &hot);
    for (int i = 0; i < 400; i++)
        meas_tick(&m, 4000000000u); /* absurd rate must not overflow or crash */
    CHECK(m.saturated, "saturation not flagged");
    meas_init(&m, &cfg);
    for (int i = 0; i < 400; i++)
        meas_tick(&m, 0);
    CHECK(meas_cpm(&m) == 0 && meas_rate_nsvh(&m) == 0, "zero input gives non-zero output");
}

int main(void)
{
    test_steady(10, 6);
    test_steady(25, 6);
    test_steady(100, 5);
    test_steady(1000, 3);
    test_steady(30000, 3);
    test_step();
    test_dead_time();
    test_dose();
    test_extremes();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall meas checks passed\n", failures);
    return failures ? 1 : 0;
}
