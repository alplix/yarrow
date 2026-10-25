/* Tests of the Radioactive@Home source against a fake HID device: the report layout, the identity, the refusal of other devices
 * with the shared USB id, and bins cut from the host clock that match the true count of the pulses. */
#include "yarrow_src.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

#define BASE 1792400400.0            /* a multiple of 600 */
#define N_PULSES 6000

static double wall;
static double pulse_t[N_PULSES];     /* seconds of the pulses, increasing */
static int n_pulses;
static double dev_start, timer0_ms = 5000;
static int fake_mode;                /* 0 normal, 1 timer does not run in milliseconds */
static unsigned calls;

static double fake_now(void) { wall += 0.001; return wall; }
static double sleep_scale = 1.0;      /* how long a wait really takes: 1 as asked, 0.17 when a signal cuts it short */
static void fake_sleep(int ms) { wall += ms / 1000.0 * sleep_scale; }

static uint32_t pulses_until(double x)
{
    int lo = 0, hi = n_pulses;
    while (lo < hi) {
        int m = (lo + hi) / 2;
        if (pulse_t[m] <= x)
            lo = m + 1;
        else
            hi = m;
    }
    return (uint32_t)lo;
}

static int fake_get(void *ctx, uint8_t *buf, size_t len)
{
    (void)ctx;
    if (len < 129)
        return -1;
    memset(buf, 0, len);
    uint32_t timer = fake_mode == 1 ? 5000 + ++calls : (uint32_t)(timer0_ms + (wall - dev_start) * 1000.0);
    uint32_t pulses = pulses_until(wall);
    for (int i = 0; i < 4; i++) {
        buf[1 + i] = (uint8_t)(timer >> (8 * i));
        buf[5 + i] = (uint8_t)(pulses >> (8 * i));
    }
    return (int)len;
}

static void fake_close(void *ctx) { (void)ctx; }

static uint32_t rs = 777;
static double urand(void)
{
    rs = rs * 1664525u + 1013904223u;
    return ((rs >> 8) + 0.5) / 16777216.0;
}

static void make_pulses(double rate_cps)
{
    double t = BASE;
    n_pulses = 0;
    while (n_pulses < N_PULSES) {
        t += -log(urand()) / rate_cps;
        pulse_t[n_pulses++] = t;
    }
}

static yhid_t hid(void)
{
    yhid_t h;
    memset(&h, 0, sizeof h);
    h.get_feature = fake_get;
    h.close = fake_close;
    snprintf(h.name, sizeof h.name, "obdev.at radioactiveathome.org GRS");
    h.version = 0x0251;
    return h;
}

static uint32_t got_start[16], got_count[16];
static int got_n;
static void on_bin(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    got_start[got_n] = start;
    got_count[got_n++] = count;
}

/* the monitor's loop, on simulated time */
static void run(ysrc_t *src, double until)
{
    ybinner_t b;
    ybin_init(&b, 600, on_bin, 0);
    ysample_t s;
    memset(&s, 0, sizeof s);
    while (wall < until) {
        if (src->poll(src, 600, &s) != 0)
            break;
        ybin_feed(&b, &s);
        wall += src->next_wait_ms(src, ybin_to_edge(&b, s.t), 600) / 1000.0;
    }
}

static yrah_env_t env(const char *host)
{
    yrah_env_t e = {fake_now, fake_sleep, host};
    return e;
}

static void bins_match_truth(void)
{
    make_pulses(0.5);
    wall = BASE + 123.37;
    dev_start = wall;
    fake_mode = 0;
    got_n = 0;
    yhid_t h = hid();
    yrah_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = yrah_make(&h, &e, &dev);
    CHECK(src != NULL, "a fake counter is found");
    if (!src)
        return;
    CHECK(dev.sens_x100 == 17123 && dev.dead_us == 190, "tube calibration %u %u", dev.sens_x100, dev.dead_us);
    CHECK(strlen(dev.uid) == 16 && strcmp(dev.fw, "rah-2.51") == 0, "identity %s %s", dev.uid, dev.fw);
    run(src, BASE + 1800.5);
    CHECK(got_n == 2, "two bins between the edges (got %d)", got_n);
    for (int i = 0; i < got_n; i++) {
        uint32_t truth = pulses_until(got_start[i] + 600.0) - pulses_until((double)got_start[i]);
        printf("bin %u: %u counted, %u true\n", got_start[i], got_count[i], truth);
        CHECK(got_start[i] == (uint32_t)BASE + 600 + 600 * i, "bin %d starts at %u", i, got_start[i]);
        CHECK(got_count[i] + 1 >= truth && got_count[i] <= truth + 1, "bin %d counts %u, truth %u", i, got_count[i], truth);
    }
    src->close(src);
}

static void late_readings_make_no_bin(void)
{
    make_pulses(0.5);
    wall = BASE + 0.5;                          /* always half a second after an edge: never on an edge */
    dev_start = wall;
    got_n = 0;
    yhid_t h = hid();
    yrah_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = yrah_make(&h, &e, &dev);
    CHECK(src != NULL, "counter found (late)");
    if (!src)
        return;
    ybinner_t b;
    ybin_init(&b, 600, on_bin, 0);
    ysample_t s;
    memset(&s, 0, sizeof s);
    for (int i = 0; i < 1500; i++) {
        wall = BASE + 0.5 + i;
        if (src->poll(src, 600, &s) != 0)
            break;
        ybin_feed(&b, &s);
    }
    CHECK(got_n == 0, "readings away from the edge never make a bin (got %d)", got_n);
    src->close(src);
}

static void other_device_is_refused(void)
{
    make_pulses(0.5);
    wall = BASE + 10;
    dev_start = wall;
    fake_mode = 1;
    yhid_t h = hid();
    yrah_env_t e = env("pc-one");
    ydev_t dev;
    CHECK(yrah_make(&h, &e, &dev) == NULL, "a device whose timer is not in milliseconds is refused");
    fake_mode = 0;
}

/* the check compares the counter's timer with the time that really passed: a wait that takes longer than asked (a busy
 * computer) is fine, and so is one that a signal shortened as long as it was not almost nothing (on Linux the BOINC library's
 * timer signal cut 300 ms to 50 ms, and every Radioactive@Home counter was refused) */
static void wait_not_exact(void)
{
    static const double scale[3] = {3.0, 0.5, 0.05};
    static const int accepted[3] = {1, 1, 0};
    for (int k = 0; k < 3; k++) {
        make_pulses(0.5);
        wall = BASE + 10;
        dev_start = wall;
        fake_mode = 0;
        sleep_scale = scale[k];
        yhid_t h = hid();
        yrah_env_t e = env("pc-one");
        ydev_t dev;
        ysrc_t *src = yrah_make(&h, &e, &dev);
        CHECK((src != NULL) == accepted[k], "a wait of %.0f%% of 300 ms: counter %s", scale[k] * 100, src ? "accepted" : "refused");
        if (src)
            src->close(src);
    }
    sleep_scale = 1.0;
}

static void identity(void)
{
    make_pulses(0.5);
    wall = BASE + 10;
    dev_start = wall;
    yhid_t h = hid();
    yrah_env_t e1 = env("pc-one"), e2 = env("pc-two");
    ydev_t d1, d1b, d2;
    ysrc_t *a = yrah_make(&h, &e1, &d1);
    ysrc_t *b = yrah_make(&h, &e1, &d1b);
    ysrc_t *c = yrah_make(&h, &e2, &d2);
    CHECK(a && b && c, "three sources");
    if (a && b && c) {
        CHECK(strcmp(d1.uid, d1b.uid) == 0, "the same computer gives the same identity");
        CHECK(strcmp(d1.uid, d2.uid) != 0, "another computer gives another identity");
    }
    if (a) a->close(a);
    if (b) b->close(b);
    if (c) c->close(c);
}

int main(void)
{
    bins_match_truth();
    late_readings_make_no_bin();
    other_device_is_refused();
    wait_not_exact();
    identity();
    if (fails) {
        printf("%d checks failed\n", fails);
        return 1;
    }
    printf("all Radioactive@Home source checks passed\n");
    return 0;
}
