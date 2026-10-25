/* Tests of the GQ GMC source against a fake counter on a fake serial port: identity, the device's own calibration, the
 * heartbeat stream cut into bins, a lost second, a broken frame, and a device that is not a GMC. */
#include "yarrow_gmc.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

#define BASE 1792400400.0            /* a multiple of 600 */
#define N_PULSES 6000

static double wall;
static double pulse_t[N_PULSES];
static int n_pulses;
static double phase;                 /* heartbeats arrive at k + phase */
static double next_beat;
static int beating;
static int lose_at, beats_sent;      /* the heartbeat with this number is not sent (0: none) */
static int bad_frame_at;
static uint8_t queue[600];
static size_t qn, qpos;
static int not_gmc;
static uint8_t cfg[512];
static size_t cfg_len;

static uint32_t rs = 4242;
static double urand(void)
{
    rs = rs * 1664525u + 1013904223u;
    return ((rs >> 8) + 0.5) / 16777216.0;
}

static void make_pulses(double rate)
{
    double t = BASE;
    n_pulses = 0;
    while (n_pulses < N_PULSES) {
        t += -log(urand()) / rate;
        pulse_t[n_pulses++] = t;
    }
}

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

static void put_float(uint8_t *p, float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    p[0] = (uint8_t)(u >> 24); p[1] = (uint8_t)(u >> 16); p[2] = (uint8_t)(u >> 8); p[3] = (uint8_t)u;
}

static void put_cal(int idx, unsigned cpm, float sv)
{
    static const int at[3] = {0x08, 0x0e, 0x14};
    cfg[at[idx]] = (uint8_t)(cpm >> 8);
    cfg[at[idx] + 1] = (uint8_t)cpm;
    put_float(cfg + at[idx] + 2, sv);
}

static void reset_device(void)
{
    qn = qpos = 0;
    beating = 0;
    lose_at = bad_frame_at = beats_sent = 0;
    not_gmc = 0;
    memset(cfg, 0, sizeof cfg);
    cfg_len = 256;
    put_cal(0, 1000, 6.5f);
    put_cal(1, 2000, 13.0f);
    put_cal(2, 3000, 19.5f);
}

static void queue_bytes(const void *p, size_t n)
{
    memcpy(queue + qn, p, n);
    qn += n;
}

static int f_write(void *ctx, const void *p, size_t n)
{
    (void)ctx;
    char c[64];
    size_t k = n < sizeof c - 1 ? n : sizeof c - 1;
    memcpy(c, p, k);
    c[k] = 0;
    if (!strcmp(c, "<GETVER>>")) {
        if (not_gmc)
            queue_bytes("NOT A GMC AT ", 13);
        else
            queue_bytes("GMC-300Re 4.20", 14);
    } else if (!strcmp(c, "<GETSERIAL>>")) {
        queue_bytes("\x01\x02\x03\x04\x05\x06\x07", 7);
    } else if (!strcmp(c, "<GETCFG>>")) {
        queue_bytes(cfg, cfg_len);
    } else if (!strcmp(c, "<HEARTBEAT1>>")) {
        beating = 1;
        next_beat = floor(wall) + 1 + phase;
        if (next_beat - wall > 1.0)
            next_beat -= 1.0;
    } else if (!strcmp(c, "<HEARTBEAT0>>")) {
        beating = 0;
    }
    return 0;
}

static int f_read(void *ctx, void *buf, size_t n, int ms)
{
    (void)ctx; (void)ms;
    uint8_t *o = buf;
    size_t got = 0;
    while (got < n && qpos < qn)
        o[got++] = queue[qpos++];
    if (got == n || !beating)
        return (int)got;
    /* the next heartbeat: the pulses counted in the second that just ended */
    for (;;) {
        wall = next_beat;
        double t1 = next_beat;
        next_beat += 1.0;
        beats_sent++;
        if (beats_sent == lose_at)
            continue;
        uint32_t c = pulses_until(t1) - pulses_until(t1 - 1.0);
        unsigned v = c & 0x3fffu;
        if (beats_sent == bad_frame_at)
            v |= 0x8000u;
        o[got++] = (uint8_t)(v >> 8);
        o[got++] = (uint8_t)v;
        return (int)got;
    }
}

static void f_close(void *ctx) { (void)ctx; }
static void f_drain(void *ctx) { (void)ctx; qn = qpos = 0; }
static double f_now(void) { return wall; }

static ygmc_io_t io(void)
{
    ygmc_io_t i = {0, f_write, f_read, f_close};
    return i;
}

static ygmc_env_t env(const char *host)
{
    ygmc_env_t e = {f_now, f_drain, host};
    return e;
}

static uint32_t got_start[16], got_count[16];
static int got_n;
static void on_bin(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    got_start[got_n] = start;
    got_count[got_n++] = count;
}

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
    }
}

static void start(double w0)
{
    wall = w0;
    got_n = 0;
}

static void calibration_and_identity(void)
{
    make_pulses(0.5);
    reset_device();
    start(BASE + 100);
    ygmc_io_t i = io();
    ygmc_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = ygmc_make(&i, &e, &dev);
    CHECK(src != NULL, "a fake GMC is found");
    if (src) {
        CHECK(dev.sens_x100 == 15385, "sensitivity from the first calibration point: %u", dev.sens_x100);
        CHECK(strcmp(dev.fw, "GMC-300Re-4.20") == 0, "firmware text %s", dev.fw);
        CHECK(strlen(dev.uid) == 16, "identity %s", dev.uid);
        src->close(src);
    }
    reset_device();
    put_cal(0, 0, 0.0f);
    start(BASE + 100);
    src = ygmc_make(&i, &e, &dev);
    CHECK(src != NULL && dev.sens_x100 == 15385, "an empty first point falls through to the next (%u)", src ? dev.sens_x100 : 0);
    if (src)
        src->close(src);
    reset_device();
    put_cal(0, 0, 0.0f);
    put_cal(1, 0, 0.0f);
    put_cal(2, 5, 9999.0f);
    start(BASE + 100);
    CHECK(ygmc_make(&i, &e, &dev) == NULL, "no usable calibration: the counter is not used");
    {   /* the configuration page of a real GMC-300Re 4.54 (bytes 8 to 25, as reported by a volunteer): little endian floats */
        static const uint8_t real[18] = {0x00, 0x3c, 0x14, 0xae, 0xc7, 0x3e, 0x00, 0xf0, 0x14, 0xae, 0xc7, 0x3f, 0x03, 0xe8, 0x00, 0x00, 0xd0, 0x40};
        uint8_t page[256];
        memset(page, 0, sizeof page);
        memcpy(page + 8, real, sizeof real);
        CHECK(ygmc_sensitivity(page, sizeof page) == 15385, "the page of a real GMC-300Re 4.54 gives 153.85 cpm per uSv/h (%u)", ygmc_sensitivity(page, sizeof page));
    }
    reset_device();
    not_gmc = 1;
    start(BASE + 100);
    CHECK(ygmc_make(&i, &e, &dev) == NULL, "another device is refused");
    reset_device();
    ydev_t d1, d2;
    start(BASE + 100);
    ysrc_t *a = ygmc_make(&i, &e, &d1);
    ygmc_env_t e2 = env("pc-two");
    start(BASE + 100);
    ysrc_t *b = ygmc_make(&i, &e2, &d2);
    CHECK(a && b && strcmp(d1.uid, d2.uid) == 0, "a counter with a serial number keeps its identity on another computer");
    if (a) a->close(a);
    if (b) b->close(b);
}

static void bins(double ph)
{
    make_pulses(0.5);
    reset_device();
    phase = ph;
    start(BASE + 123.2);
    ygmc_io_t i = io();
    ygmc_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = ygmc_make(&i, &e, &dev);
    CHECK(src != NULL, "counter found (phase %.2f)", ph);
    if (!src)
        return;
    run(src, BASE + 1800.9);
    CHECK(got_n == 2, "two bins (phase %.2f, got %d)", ph, got_n);
    for (int k = 0; k < got_n; k++) {
        double e1 = got_start[k];
        /* the heartbeat nearest to the edge closes the window: heartbeats come at whole seconds + phase */
        double a1 = floor(e1) + ph;
        if (a1 - e1 >= 0.5)
            a1 -= 1.0;
        double a2 = a1 + 600.0;
        uint32_t truth = pulses_until(a2) - pulses_until(a1);
        printf("phase %.2f bin %u: %u counted, %u true\n", ph, got_start[k], got_count[k], truth);
        CHECK(got_start[k] == (uint32_t)BASE + 600 + 600 * (uint32_t)k, "bin start %u", got_start[k]);
        CHECK(got_count[k] == truth, "bin %d counts %u, truth %u (phase %.2f)", k, got_count[k], truth, ph);
    }
    src->close(src);
}

static void lost_second(void)
{
    make_pulses(0.5);
    reset_device();
    phase = 0.4;
    start(BASE + 123.2);
    ygmc_io_t i = io();
    ygmc_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = ygmc_make(&i, &e, &dev);
    if (!src) {
        CHECK(0, "counter found (lost second)");
        return;
    }
    lose_at = 1400;                              /* a second missing in the middle of the bin that starts 1200 s after BASE */
    run(src, BASE + 2500.0);
    int hit = 0;
    for (int k = 0; k < got_n; k++)
        if (got_start[k] == (uint32_t)BASE + 1200)
            hit = 1;
    int other = 0;
    for (int k = 0; k < got_n; k++)
        if (got_start[k] == (uint32_t)BASE + 600)
            other = 1;
    CHECK(!hit, "the bin with a lost second is not reported");
    CHECK(other, "the bin before it is still reported (%d bins)", got_n);
    src->close(src);
}

static void broken_frame(void)
{
    make_pulses(0.5);
    reset_device();
    phase = 0.4;
    start(BASE + 123.2);
    ygmc_io_t i = io();
    ygmc_env_t e = env("pc-one");
    ydev_t dev;
    ysrc_t *src = ygmc_make(&i, &e, &dev);
    if (!src) {
        CHECK(0, "counter found (frame)");
        return;
    }
    bad_frame_at = 5;
    ysample_t s;
    int rc = 0;
    for (int k = 0; k < 6; k++)
        if (src->poll(src, 600, &s) != 0)
            rc = 1;
    CHECK(rc != 0, "a frame with reserved bits set is an error");
    src->close(src);
}

int main(void)
{
    calibration_and_identity();
    bins(0.37);
    bins(0.70);
    lost_second();
    broken_frame();
    if (fails) {
        printf("%d checks failed\n", fails);
        return 1;
    }
    printf("all GQ GMC source checks passed\n");
    return 0;
}
