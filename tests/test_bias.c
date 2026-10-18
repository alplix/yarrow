/* The rate estimate must not lean to one side. Choosing the averaging window from the very pulses that are then
 * averaged makes the estimate too high at some rates, so this measures the bias directly. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "meas.h"

static uint64_t rs = 0xABCDEF12345ull;
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
    /* true rates in counts per second; dead time 90 us is negligible here, so the true rate is what the tube sees */
    const double rates[] = {0.05, 0.10, 0.17, 0.25, 0.40, 0.70, 1.0, 1.6, 2.5, 4.0, 8.0, 20.0};
    for (unsigned r = 0; r < sizeof rates / sizeof *rates; r++) {
        double sum_ratio = 0, sum_sq = 0;
        long n = 0;
        for (int run = 0; run < 40; run++) {
            meas_t m;
            meas_cfg_t mc = {8000, 90};
            meas_init(&m, &mc);
            for (int s = 0; s < 3000; s++) {
                meas_tick(&m, poisson(rates[r]));
                if (s >= 600 && s % 13 == 0) { /* sample the estimate now and then */
                    double est = m.rate_cps_x100 / 100.0;
                    double ratio = est / rates[r];
                    sum_ratio += ratio;
                    sum_sq += ratio * ratio;
                    n++;
                }
            }
        }
        double mean = sum_ratio / n;
        double sd = sqrt(sum_sq / n - mean * mean);
        printf("true %5.2f cps: mean estimate / true = %.4f (spread %.3f, %ld samples)\n", rates[r], mean, sd, n);
        CHECK(fabs(mean - 1.0) < 0.02, "bias %.3f at %.2f cps", mean - 1.0, rates[r]);
    }
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall bias checks passed\n", failures);
    return failures ? 1 : 0;
}
