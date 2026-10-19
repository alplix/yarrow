#include "yarrow_bin.h"
#include "yarrow_trickle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* A fake device: pulses arrive at the start of each second, so the true count of any whole-second span is known. */
typedef struct {
    uint32_t t, pulses, hv_s;
    int hv_on;
    uint32_t per_s[40000];     /* pulses added in the second starting at index (t - t0) */
    uint32_t t0;
} dev_t_;

static uint32_t rnd_state = 12345;
static uint32_t rnd(void) { rnd_state = rnd_state * 1664525u + 1013904223u; return rnd_state >> 8; }

static dev_t_ dev;
static double host;

static void dev_start(uint32_t t0)
{
    memset(&dev, 0, sizeof dev);
    dev.t = dev.t0 = t0;
    dev.hv_on = 1;
    for (int i = 0; i < 40000; i++)
        dev.per_s[i] = rnd() % 4;          /* 0..3 pulses a second */
}

static void dev_second(void)
{
    dev.t++;
    host += 1.0;
    if (dev.hv_on) {
        dev.pulses += dev.per_s[dev.t - dev.t0];
        dev.hv_s++;
    }
}

static ysample_t sample(void)
{
    ysample_t s = {dev.t, dev.pulses, dev.hv_s, host};
    return s;
}

static uint32_t got_start[200], got_count[200];
static int got_n;
static void on_bin(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    got_start[got_n] = start;
    got_count[got_n++] = count;
}

static uint32_t truth(uint32_t start, uint32_t len)
{
    uint32_t sum = 0;
    for (uint32_t t = start + 1; t <= start + len; t++)
        sum += dev.per_s[t - dev.t0];
    return sum;
}

/* run the monitor loop for `secs` seconds; fast = poll 10 times a second around the edges */
static void run(ybinner_t *b, int secs, int fast, void (*event)(int sec))
{
    for (int i = 0; i < secs; i++) {
        if (event)
            event(i);
        uint32_t to_edge = ybin_to_edge(b, dev.t);
        if (fast && (to_edge <= 3 || to_edge >= b->interval - 2)) {
            for (int k = 0; k < 10; k++) {
                ysample_t s = sample();
                s.host_s = host + k * 0.1;
                ybin_feed(b, &s);
            }
        } else {
            ysample_t s = sample();
            ybin_feed(b, &s);
        }
        dev_second();
    }
}

static void check_all_exact(const char *what)
{
    for (int i = 0; i < got_n; i++)
        CHECK(got_count[i] == truth(got_start[i], 600), "%s: bin %u is %u, truth %u", what, (unsigned)got_start[i],
              (unsigned)got_count[i], (unsigned)truth(got_start[i], 600));
}

static void normal(void)
{
    ybinner_t b;
    got_n = 0;
    dev_start(1792347017u);
    ybin_init(&b, 600, on_bin, 0);
    run(&b, 3 * 3600, 1, 0);
    CHECK(got_n == 17 || got_n == 18, "normal: %d bins", got_n);
    CHECK(got_n && got_start[0] % 600 == 0, "bins start on the grid");
    check_all_exact("normal");
}

static void restart_event(int sec)
{
    if (sec == 4000) {                 /* the device restarts: counters back to zero */
        dev.pulses = 0;
        dev.hv_s = 0;
    }
}

static void restart(void)
{
    ybinner_t b;
    got_n = 0;
    dev_start(1792347000u);
    ybin_init(&b, 600, on_bin, 0);
    run(&b, 3 * 3600, 1, restart_event);
    uint32_t bad = 1792347000u + 4000 - (1792347000u + 4000) % 600;
    for (int i = 0; i < got_n; i++)
        CHECK(got_start[i] != bad, "restart: the bin with the restart was reported");
    CHECK(got_n >= 15, "restart: only %d bins", got_n);
}

static void hv_event(int sec)
{
    dev.hv_on = !(sec >= 4100 && sec < 4160);
}

static void hv_off(void)
{
    ybinner_t b;
    got_n = 0;
    dev_start(1792347000u);
    ybin_init(&b, 600, on_bin, 0);
    run(&b, 2 * 3600, 1, hv_event);
    uint32_t bad = 1792347000u + 4100 - (1792347000u + 4100) % 600;
    for (int i = 0; i < got_n; i++)
        CHECK(got_start[i] != bad, "hv off: the bin with the voltage gap was reported");
    CHECK(got_n >= 10, "hv off: only %d bins", got_n);
}

static void clock_jump(void)
{
    ybinner_t b;
    got_n = 0;
    dev_start(1792347000u);
    ybin_init(&b, 600, on_bin, 0);
    uint32_t t0 = dev.t0;
    for (int i = 0; i < 2 * 3600; i++) {
        if (i == 4300) {
            dev.t += 600;
            dev.t0 += 600;             /* keeps per_s indexing valid after the jump */
        }
        ysample_t s = sample();
        ybin_feed(&b, &s);
        dev_second();
    }
    uint32_t jump_bin = t0 + 4300 - (t0 + 4300) % 600;
    for (int i = 0; i < got_n; i++)
        CHECK(got_start[i] != jump_bin && got_start[i] != jump_bin + 600, "clock jump: bin %u reported", (unsigned)got_start[i]);
}

static void slow_polling(void)
{
    ybinner_t b;
    got_n = 0;
    dev_start(1792347000u);
    ybin_init(&b, 600, on_bin, 0);
    for (int i = 0; i < 3 * 3600; i += 7) {          /* one poll every 7 seconds */
        ysample_t s = sample();
        ybin_feed(&b, &s);
        for (int k = 0; k < 7; k++)
            dev_second();
    }
    check_all_exact("slow polling");       /* bins may be lost, never wrong */
}

static void trickle(void)
{
    ypending_t p;
    ydev_t d = {"9f3c0a1b22d4e5f6", "0.1.0", 8000, 90, "TR", "Denizli", 600};
    char xml[2048];
    memset(&p, 0, sizeof p);
    ypending_add(&p, 1792347600u, 12);
    ypending_add(&p, 1792348200u, 9);
    ypending_add(&p, 1792349400u, 11);       /* one bin missing */
    uint32_t k = ypending_xml(&p, &d, xml, sizeof xml);
    CHECK(k == 3, "trickle takes all three, got %u", (unsigned)k);
    CHECK(strstr(xml, "bins=\"12,9,-,11\"") != 0, "gap written as '-': %s", xml);
    CHECK(strstr(xml, "start=\"1792347600\"") != 0, "start");
    char save[256];
    ypending_save(&p, save, sizeof save);
    ypending_t q;
    ypending_load(&q, save);
    CHECK(q.n == 3 && q.start[2] == 1792349400u && q.count[1] == 9, "checkpoint round trip");
    ypending_drop(&p, k);
    CHECK(p.n == 0 && ypending_xml(&p, &d, xml, sizeof xml) == 0, "nothing left");
    CHECK(ydev_valid_place("TR", "Denizli") && ydev_valid_place("TR", "Saint-Etienne") && !ydev_valid_place("tr", "Denizli")
              && !ydev_valid_place("TR", "<b>") && !ydev_valid_place("TR", "A") && !ydev_valid_place("TRX", "Denizli"),
          "place check");
    CHECK(ydev_valid_place("FR", "Saint-\xC3\x89tienne") && ydev_valid_place("CN", "\xE5\x8C\x97\xE4\xBA\xAC") && ydev_valid_place("CN", "Xi'an")
              && ydev_valid_place("CA", "St. John's") && ydev_valid_place("TR", "\xC4\xB0stanbul") && ydev_valid_place("RU", "\xD0\x9C\xD0\xBE\xD1\x81\xD0\xBA\xD0\xB2\xD0\xB0"),
          "real place names are accepted");
    CHECK(!ydev_valid_place("DE", "Berlin 2") && !ydev_valid_place("DE", "-Berlin") && !ydev_valid_place("DE", "Berlin-") && !ydev_valid_place("DE", "Berlin, DE")
              && !ydev_valid_place("DE", "Ber(lin)") && !ydev_valid_place("DE", "Ber\tlin") && !ydev_valid_place("DE", "Berlin\n"),
          "names the server would refuse are refused here too");
    if (getenv("YARROW_PRINT_XML")) {
        ypending_add(&p, 1792347600u, 12);
        ypending_add(&p, 1792348200u, 9);
        ypending_xml(&p, &d, xml, sizeof xml);
        printf("%s\n", xml);
    }
}

int main(void)
{
    normal();
    restart();
    hv_off();
    clock_jump();
    slow_polling();
    trickle();
    if (fails) {
        printf("%d checks failed\n", fails);
        return 1;
    }
    printf("all monitor checks passed\n");
    return 0;
}
