#include "ylc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE 1008

static uint64_t rng_state = 88172645463325252ull;
static uint32_t rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint32_t)(rng_state >> 11);
}
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }

static uint32_t poisson(double lam)
{
    double L = exp(-lam), p = 1.0;
    uint32_t k = 0;
    do {
        k++;
        p *= uni();
    } while (p > L);
    return k - 1;
}

static double poisson_entropy(double lam)
{
    double h = 0, lf = 0;
    for (int k = 0; k < (int)(lam + 12 * sqrt(lam + 1) + 40); k++) {
        if (k > 1)
            lf += log((double)k);
        double p = exp(-lam + k * log(lam) - lf);
        if (p > 0)
            h -= p * log2(p);
    }
    return h;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void test_compression(double lam, int n)
{
    uint32_t *data = malloc(sizeof(uint32_t) * (size_t)n);
    for (int i = 0; i < n; i++)
        data[i] = poisson(lam);

    uint8_t page[PAGE];
    uint32_t mean = (uint32_t)(lam * 256);
    int pos = 0;
    double total_bytes = 0;
    int bad = 0;
    while (pos < n) {
        ylc_enc_t e;
        ylc_enc_init(&e, page, PAGE, mean);
        int start = pos;
        while (pos < n && ylc_enc_put(&e, data[pos]))
            pos++;
        uint32_t next_mean = ylc_enc_mean(&e);
        uint32_t len = ylc_enc_finish(&e);
        total_bytes += len;

        ylc_dec_t d;
        ylc_dec_init(&d, page, len, mean);
        for (int i = start; i < pos; i++) {
            uint32_t x;
            if (!ylc_dec_get(&d, &x) || x != data[i]) {
                bad++;
                break;
            }
        }
        uint32_t extra;
        if (ylc_dec_get(&d, &extra) || !d.ended) /* must stop exactly at the end, by the end flag */
            bad++;
        mean = next_mean;
    }
    double bits = total_bytes * 8 / n, h = poisson_entropy(lam);
    printf("lambda %7.2f  entropy %6.3f  coded %6.3f bits/sample  (%+5.1f%%)\n", lam, h, bits, 100 * (bits / h - 1));
    CHECK(bad == 0, "round trip mismatch at lambda %.2f", lam);
    CHECK(bits < h * 1.12, "lambda %.2f: %.3f bits/sample is more than 12 percent above entropy %.3f", lam, bits, h);
    free(data);
}

static void test_truncation(double lam)
{
    /* Power loss: the page was never finished, only the final bytes reached flash. */
    enum { N = 6000 };
    static uint32_t data[N];
    uint8_t page[PAGE];
    ylc_enc_t e;
    ylc_enc_init(&e, page, PAGE, (uint32_t)(lam * 256));
    int n = 0;
    while (n < N && (data[n] = poisson(lam), ylc_enc_put(&e, data[n])))
        n++;
    ylc_dec_t d;
    ylc_dec_init(&d, page, e.len, (uint32_t)(lam * 256));
    int got = 0;
    uint32_t x;
    while (ylc_dec_get(&d, &x)) {
        if (got >= n || x != data[got]) {
            CHECK(0, "truncated decode differs at sample %d", got);
            break;
        }
        got++;
    }
    printf("lambda %7.2f  unfinished page: %d of %d samples recovered (%u bytes)\n", lam, got, n, (unsigned)e.len);
    /* Only the bytes still pending in the coder may be lost: about 9 bytes worth of samples. */
    double bytes_per_sample = (double)e.len / n;
    CHECK((n - got) * bytes_per_sample <= 9.0, "lost %d of %d samples, more than 9 bytes worth", n - got, n);
}

static void test_cut_streams(void)
{
    /* A finished stream cut at any length must decode a correct prefix. */
    enum { N = 3000 };
    static uint32_t data[N];
    uint8_t page[PAGE];
    ylc_enc_t e;
    ylc_enc_init(&e, page, PAGE, 0);
    int n = 0;
    while (n < N && (data[n] = poisson(2.5), ylc_enc_put(&e, data[n])))
        n++;
    uint32_t len = ylc_enc_finish(&e);
    {
        ylc_dec_t d;
        ylc_dec_init(&d, page, len, 0);
        uint32_t x;
        int got = 0;
        while (ylc_dec_get(&d, &x))
            CHECK(got < n && x == data[got++], "full stream sample %d wrong", got - 1);
        CHECK(got == n && d.ended, "full stream: %d of %d samples, ended=%d", got, n, d.ended);
    }
    for (uint32_t cut = 0; cut <= len; cut += 7) {
        ylc_dec_t d;
        ylc_dec_init(&d, page, cut, 0);
        uint32_t x;
        int got = 0;
        while (ylc_dec_get(&d, &x)) {
            if (got >= n || x != data[got]) {
                CHECK(0, "cut at %u: sample %d wrong", (unsigned)cut, got);
                break;
            }
            got++;
        }
    }
}

static void test_extremes(void)
{
    static const uint32_t pattern[] = {0, 0, 0, 0xFFFFFFFFu, 0, 1, 0xFFFFFFFFu, 0xFFFFFFFFu, 5, 4000000000u, 7, 0,
                                       123456789, 123456790, 0, 0xFFFFFFFEu, 2, 2, 2, 2};
    uint8_t buf[512];
    ylc_enc_t e;
    ylc_enc_init(&e, buf, sizeof buf, 0);
    int n = (int)(sizeof pattern / sizeof *pattern);
    for (int i = 0; i < n; i++)
        CHECK(ylc_enc_put(&e, pattern[i]), "extreme sample %d rejected", i);
    uint32_t len = ylc_enc_finish(&e);
    ylc_dec_t d;
    ylc_dec_init(&d, buf, len, 0);
    for (int i = 0; i < n; i++) {
        uint32_t x = 0;
        CHECK(ylc_dec_get(&d, &x) && x == pattern[i], "extreme sample %d: got %u want %u", i, (unsigned)x, (unsigned)pattern[i]);
    }
}

static void test_level_changes(void)
{
    /* A source that changes level: ramp, step and burst. */
    enum { N = 20000 };
    static uint32_t data[N];
    for (int i = 0; i < N; i++) {
        double lam = i < 5000 ? 3 : i < 8000 ? 3 + (i - 5000) * 0.2 : i < 9000 ? 2000 : 12;
        data[i] = poisson(lam);
    }
    uint8_t page[PAGE];
    uint32_t mean = 0;
    int pos = 0;
    while (pos < N) {
        ylc_enc_t e;
        ylc_enc_init(&e, page, PAGE, mean);
        int start = pos;
        while (pos < N && ylc_enc_put(&e, data[pos]))
            pos++;
        uint32_t next = ylc_enc_mean(&e);
        uint32_t len = ylc_enc_finish(&e);
        ylc_dec_t d;
        ylc_dec_init(&d, page, len, mean);
        for (int i = start; i < pos; i++) {
            uint32_t x;
            if (!ylc_dec_get(&d, &x) || x != data[i]) {
                CHECK(0, "level change: mismatch at %d", i);
                return;
            }
        }
        mean = next;
    }
}

static void test_garbage(void)
{
    /* Damaged flash must never crash or loop forever. */
    uint8_t buf[PAGE];
    for (int round = 0; round < 300; round++) {
        uint32_t len = rnd() % PAGE;
        for (uint32_t i = 0; i < len; i++)
            buf[i] = (uint8_t)rnd();
        ylc_dec_t d;
        ylc_dec_init(&d, buf, len, rnd() % 100000);
        uint32_t x;
        int count = 0;
        while (count < 100000 && ylc_dec_get(&d, &x))
            count++;
    }
}

int main(void)
{
    static const double lams[] = {0.02, 0.05, 0.17, 0.5, 1.7, 5, 10, 50, 100, 600};
    for (unsigned i = 0; i < sizeof lams / sizeof *lams; i++)
        test_compression(lams[i], lams[i] > 100 ? 40000 : 120000);
    test_truncation(0.17);
    test_truncation(10);
    test_truncation(600);
    test_cut_streams();
    test_extremes();
    test_level_changes();
    test_garbage();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall ylc checks passed\n", failures);
    return failures ? 1 : 0;
}
