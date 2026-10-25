/* The calibration must find the sensitivity the simulated tube really has. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cal.h"
#include "meas.h"

static uint64_t rs = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 11); }
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }
static uint32_t poisson(double lam)
{
    if (lam > 500) { double g = sqrt(-2 * log(uni())) * cos(6.283185307 * uni()); double v = lam + sqrt(lam) * g; return v < 0 ? 0 : (uint32_t)(v + 0.5); }
    double L = exp(-lam), p = 1.0; uint32_t k = 0;
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    /* a tube with a true sensitivity of 70 cpm per uSv/h (the stored value is 80), placed at 0.50, 5 and 80 uSv/h */
    const double true_sens = 70.0;
    const uint32_t dead_us = 90;
    double refs_usvh[3] = {0.50, 5.0, 80.0};
    int secs[3] = {3600, 600, 120};

    for (int k = 0; k < 3; k++) {
        double cpm_true = true_sens * refs_usvh[k];
        double cps_true = cpm_true / 60.0;
        /* what the tube delivers: true rate reduced by dead time (non-paralysable) */
        double cps_obs = cps_true / (1.0 + cps_true * dead_us * 1e-6);
        cal_acc_t c;
        cal_reset(&c);
        for (int s = 0; s < secs[k]; s++)
            cal_tick(&c, poisson(cps_obs), dead_us);
        uint32_t sens = cal_sensitivity_x100(&c, (uint32_t)(refs_usvh[k] * 1000.0 + 0.5));
        double err = 100.0 * (sens / 100.0 - true_sens) / true_sens;
        double unc = cal_uncertainty_x10(&c) / 10.0;
        printf("at %.2f uSv/h, %d s: %u counts, sensitivity %.2f (true %.1f), error %+.2f %%, stated uncertainty %.2f %%\n",
               refs_usvh[k], secs[k], c.counts, sens / 100.0, true_sens, err, unc);
        CHECK(fabs(err) < 3.0 * unc + 0.6, "error %.2f %% is outside 3 sigma (%.2f %%)", err, unc);
        CHECK(!c.saturated, "tube reported saturated");
    }

    cal_acc_t z;
    cal_reset(&z);
    CHECK(cal_sensitivity_x100(&z, 100) == 0, "nothing counted gives zero");
    CHECK(cal_uncertainty_x10(&z) == 1000, "empty uncertainty");
    cal_tick(&z, 4, 90);
    CHECK(cal_sensitivity_x100(&z, 0) == 0, "zero reference gives zero");

    /* dead time by the two-source method: a tube with a true 90 us (non-paralysable), two sources */
    for (int k = 0; k < 3; k++) {
        double ta[3] = {1500, 400, 3000}, tb[3] = {1300, 350, 2500}, bgc[3] = {1.5, 1.0, 2.0};
        double tau = 90e-6;
        int t = k == 1 ? 1800 : 200;
        dt_meas_t m;
        double lam[4] = {ta[k] + bgc[k], ta[k] + tb[k] + bgc[k], tb[k] + bgc[k], bgc[k]};
        for (int i = 0; i < 4; i++) {
            double obs = lam[i] / (1.0 + lam[i] * tau);
            m.seconds[i] = (uint32_t)t;
            m.counts[i] = poisson(obs * t);
        }
        uint32_t unc;
        uint32_t got = cal_deadtime_us(&m, &unc);
        double lim = 3.0 * unc + 3.0;
        printf("dead time, sources %.0f/%.0f cps: %u +- %u us\n", ta[k], tb[k], got, unc);
        if (k == 1)
            CHECK(got == 0 || fabs((double)got - 90.0) <= lim, "weak sources: no answer or an honest one (%u +- %u)", got, unc);
        else
            CHECK(got != 0 && fabs((double)got - 90.0) <= lim, "dead time %u +- %u us, true 90", got, unc);
    }
    {
        dt_meas_t m = {{1000, 2000, 1000, 10}, {100, 100, 100, 100}};
        uint32_t unc;
        CHECK(cal_deadtime_us(&m, &unc) == 0, "no dead-time loss at all: no answer");
    }

    printf(failures ? "\n%d check(s) FAILED\n" : "\nall calibration checks passed\n", failures);
    return failures ? 1 : 0;
}
