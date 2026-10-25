/* Log store tests on a RAM model of NOR flash with power-cut injection. */
#include "ylog.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PSIZE 1024u
#define NPAGES 16u
#define INTERVAL 60u
#define T0 1000000u
#define TRUTH_N 60000u

static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
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
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

static uint32_t truth[TRUTH_N];
static uint32_t truth_at(uint32_t t) { return truth[(t - T0) / INTERVAL]; }

/* ---- flash model ---- */
typedef struct {
    uint8_t mem[NPAGES * PSIZE];
    long ops_left;     /* program/erase operations until power is cut; <0 = never */
    bool dead;
    int errors;        /* rule violations: programming a non-blank halfword */
} ramflash_t;

static int rf_read(void *c, uint32_t a, void *b, uint32_t n)
{
    ramflash_t *f = c;
    if (a + n > sizeof f->mem)
        return -1;
    memcpy(b, f->mem + a, n);
    return 0;
}

static int rf_prog(void *c, uint32_t a, const void *b, uint32_t n)
{
    ramflash_t *f = c;
    if (f->dead)
        return -1;
    if ((a & 1) || (n & 1) || a + n > sizeof f->mem) {
        f->errors++;
        return -1;
    }
    const uint8_t *s = b;
    bool cut = f->ops_left >= 0 && --f->ops_left < 0;
    uint32_t cut_at = cut ? rnd() % (n / 2 + 1) : n / 2; /* halfwords completed before the power went */
    for (uint32_t i = 0; i < n / 2; i++) {
        uint8_t *m = f->mem + a + 2 * i;
        if (i < cut_at) {
            if (m[0] != 0xFF || m[1] != 0xFF)
                f->errors++;
            m[0] &= s[2 * i];
            m[1] &= s[2 * i + 1];
        } else if (i == cut_at && cut) { /* torn halfword: some bits cleared, others not */
            uint8_t keep0 = (uint8_t)rnd(), keep1 = (uint8_t)rnd();
            m[0] &= (uint8_t)(s[2 * i] | keep0);
            m[1] &= (uint8_t)(s[2 * i + 1] | keep1);
        }
    }
    if (cut) {
        f->dead = true;
        return -1;
    }
    return 0;
}

static int rf_erase(void *c, uint32_t page)
{
    ramflash_t *f = c;
    if (f->dead || page >= NPAGES)
        return -1;
    if (f->ops_left >= 0 && --f->ops_left < 0) { /* power cut during erase: contents undefined */
        for (uint32_t i = 0; i < PSIZE; i++)
            f->mem[page * PSIZE + i] = (rnd() & 3) ? 0xFF : (uint8_t)rnd();
        f->dead = true;
        return -1;
    }
    memset(f->mem + page * PSIZE, 0xFF, PSIZE);
    return 0;
}

static void rf_init(ramflash_t *f)
{
    memset(f, 0, sizeof *f);
    memset(f->mem, 0xFF, sizeof f->mem);
    f->ops_left = -1;
}

static ylog_flash_t rf_dev(ramflash_t *f)
{
    ylog_flash_t d = {PSIZE, NPAGES, rf_read, rf_prog, rf_erase, f};
    return d;
}

/* ---- checking ---- */
static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef struct {
    uint32_t count, bad, order_bad, last_time, first_time;
    bool any;
} verify_t;

static bool verify_visit(void *u, uint32_t t, uint32_t interval, bool valid, uint32_t v)
{
    verify_t *s = u;
    (void)valid;
    if (interval != INTERVAL || t < T0 || (t - T0) % INTERVAL || (t - T0) / INTERVAL >= TRUTH_N || truth_at(t) != v) {
        if (getenv("TRIAL"))
            printf("  bad sample: time %u (index %u) interval %u value %u expected %u\n", (unsigned)t,
                   (unsigned)((t - T0) / INTERVAL), (unsigned)interval, (unsigned)v,
                   (t >= T0 && (t - T0) / INTERVAL < TRUTH_N) ? (unsigned)truth_at(t) : 0u);
        s->bad++;
    }
    if (s->any && t <= s->last_time)
        s->order_bad++;
    if (!s->any)
        s->first_time = t;
    s->any = true;
    s->last_time = t;
    s->count++;
    return true;
}

static verify_t verify(ramflash_t *f)
{
    static uint8_t scratch[PSIZE];
    ylog_flash_t d = rf_dev(f);
    verify_t s = {0};
    ylog_scan(&d, scratch, verify_visit, &s);
    return s;
}


static void dump_flash(ramflash_t *f)
{
    for (uint32_t p = 0; p < NPAGES; p++) {
        const uint8_t *pg = f->mem + p * PSIZE;
        if (pg[0] != 0x59 || pg[1] != 0x4C) {
            int blank = 1;
            for (uint32_t i = 0; i < PSIZE; i++) if (pg[i] != 0xFF) { blank = 0; break; }
            printf("  page %2u: %s\n", (unsigned)p, blank ? "blank" : "INVALID HEADER");
            continue;
        }
        uint32_t lastnz = 0;
        for (uint32_t i = 0; i < PSIZE; i++) if (pg[i] != 0xFF) lastnz = i + 1;
        printf("  page %2u: seq %u, used to %u\n", (unsigned)p, (unsigned)(pg[2] | pg[3] << 8 | pg[4] << 16), (unsigned)lastnz);
        uint32_t off = 8;
        while (off + 14 <= PSIZE && pg[off] == 0xA7) {
            const uint8_t *h = pg + off;
            uint32_t start = h[2] | h[3] << 8 | h[4] << 16 | (uint32_t)h[5] << 24;
            uint32_t len = h[12] | h[13] << 8;
            uint8_t chk = 0x5A;
            for (int i = 0; i < 11; i++) chk ^= h[i];
            printf("    seg @%4u: first idx %u, len %u, header check %s\n", (unsigned)off,
                   (unsigned)((start - T0) / INTERVAL), (unsigned)len, chk == h[11] ? "ok" : "BAD");
            if (len == 0xFFFF) break;
            off += 14 + ((len + 1) & ~1u);
        }
    }
}

static ylog_t log_obj; /* big: keep it static */

/* Writes samples [from, from+n) of the truth array; returns how many were accepted before a failure. */
static uint32_t write_run(ylog_t *l, uint32_t from, uint32_t n)
{
    uint32_t i = 0;
    for (; i < n; i++)
        if (!ylog_add(l, T0 + (from + i) * INTERVAL, INTERVAL, true, truth[from + i]))
            break;
    return i;
}

static void test_wrap_clean(void)
{
    ramflash_t f;
    rf_init(&f);
    ylog_flash_t d = rf_dev(&f);
    CHECK(ylog_open(&log_obj, &d) == 0, "open empty");
    uint32_t n = write_run(&log_obj, 0, TRUTH_N);
    ylog_close(&log_obj);
    CHECK(n == TRUTH_N, "wrote %u of %u", (unsigned)n, TRUTH_N);
    verify_t v = verify(&f);
    CHECK(v.bad == 0 && v.order_bad == 0, "wrap: %u bad, %u out of order", (unsigned)v.bad, (unsigned)v.order_bad);
    CHECK(v.any && v.last_time == T0 + (TRUTH_N - 1) * INTERVAL, "wrap: newest sample missing");
    CHECK(f.errors == 0, "flash rule violations: %d", f.errors);
    printf("ring wrap: %u samples kept of %u written (%u pages)\n", (unsigned)v.count, TRUTH_N, NPAGES);
    CHECK(v.count > TRUTH_N / 4, "ring kept too little");
}

/* samples written before the clock was set (time_valid false) and samples dated before the first release (the factory clock the
 * stock firmware left behind, flagged valid) do not count in the statistics: the oldest record is the first one with a real time */
static void test_stats_ignore_unset_clock(void)
{
    const uint32_t real = YARROW_MIN_TIME + 5000u;
    ramflash_t f;
    rf_init(&f);
    ylog_flash_t d = rf_dev(&f);
    CHECK(ylog_open(&log_obj, &d) == 0, "open empty");
    for (uint32_t i = 0; i < 100; i++)
        CHECK(ylog_add(&log_obj, 500u + i * INTERVAL, INTERVAL, false, 20 + i % 5), "add unset-clock sample");
    for (uint32_t i = 0; i < 100; i++)                  /* a factory date, 381 days before: flagged valid, but not a real time */
        CHECK(ylog_add(&log_obj, real - 381u * 86400u + i * INTERVAL, INTERVAL, true, 20 + i % 5), "add factory-dated sample");
    for (uint32_t i = 0; i < 100; i++)
        CHECK(ylog_add(&log_obj, real + i * INTERVAL, INTERVAL, true, 20 + i % 5), "add sample");
    ylog_close(&log_obj);
    ylog_stats_t st;
    static uint8_t scratch[PSIZE];
    ylog_stats(&d, scratch, &st);
    CHECK(st.oldest_time == real, "oldest record is the first one with a real time: %u", (unsigned)st.oldest_time);
    CHECK(st.newest_time == real + 99 * INTERVAL, "newest %u", (unsigned)st.newest_time);
    rf_init(&f);
    CHECK(ylog_open(&log_obj, &d) == 0, "open empty again");
    for (uint32_t i = 0; i < 50; i++)
        ylog_add(&log_obj, 500u + i * INTERVAL, INTERVAL, false, 20);
    for (uint32_t i = 0; i < 50; i++)
        ylog_add(&log_obj, 1700000000u + i * INTERVAL, INTERVAL, true, 20);
    ylog_close(&log_obj);
    ylog_stats(&d, scratch, &st);
    CHECK(st.oldest_time == 0 && st.newest_time == 0, "only unreal times: no dates (%u, %u)", (unsigned)st.oldest_time, (unsigned)st.newest_time);
}

static void test_sessions(bool clean)
{
    ramflash_t f;
    rf_init(&f);
    ylog_flash_t d = rf_dev(&f);
    uint32_t pos = 0, written = 0, unclean = 0;
    for (int session = 0; session < 150 && pos < TRUTH_N - 4000; session++) {
        CHECK(ylog_open(&log_obj, &d) == 0, "open session %d", session);
        uint32_t n = 1 + rnd() % 3000;
        written += write_run(&log_obj, pos, n);
        pos += n + rnd() % 5; /* gap between sessions */
        if (clean || (rnd() & 1))
            ylog_close(&log_obj);
        else
            unclean++;
    }
    verify_t v = verify(&f);
    ylog_stats_t st;
    static uint8_t scratch[PSIZE];
    ylog_stats(&d, scratch, &st);
    printf("%s sessions: %u written, %u recovered, %u unclean, %u of %u pages used\n", clean ? "clean" : "mixed",
           (unsigned)written, (unsigned)v.count, (unsigned)unclean, (unsigned)st.pages_used, NPAGES);
    CHECK(v.bad == 0 && v.order_bad == 0, "sessions: %u bad, %u out of order", (unsigned)v.bad, (unsigned)v.order_bad);
    CHECK(f.errors == 0, "flash rule violations: %d", f.errors);
}

static void test_power_cuts(void)
{
    uint32_t cuts = 0, recovered_total = 0, bad_total = 0;
    const char *only = getenv("TRIAL");
    for (int trial = 0; trial < 3000; trial++) {
        if (only && atoi(only) != trial)
            continue;
        rng_state = 0x9E3779B97F4A7C15ull ^ ((uint64_t)(trial + 1) * 0x2545F4914F6CDD1Dull);
        for (int w = 0; w < 5; w++)
            rnd();
        ramflash_t f;
        rf_init(&f);
        ylog_flash_t d = rf_dev(&f);
        uint32_t pos = 0;

        /* a few normal sessions first so the cut can hit a populated ring */
        int pre = rnd() % 4;
        for (int s = 0; s < pre; s++) {
            ylog_open(&log_obj, &d);
            uint32_t n = 1 + rnd() % 4000;
            write_run(&log_obj, pos, n);
            pos += n + 2;
            if (rnd() & 1)
                ylog_close(&log_obj);
        }

        /* the session that loses power */
        f.ops_left = (long)(rnd() % 400);
        ylog_open(&log_obj, &d);
        uint32_t n = 500 + rnd() % 6000;
        write_run(&log_obj, pos, n);
        if (!f.dead && (rnd() & 1))
            ylog_close(&log_obj);
        pos += n + 3;
        if (f.dead)
            cuts++;

        /* power returns: nothing may be misread, and logging must carry on */
        f.dead = false;
        f.ops_left = -1;
        CHECK(ylog_open(&log_obj, &d) == 0, "reopen after cut (trial %d)", trial);
        uint32_t after = write_run(&log_obj, pos, 200 + rnd() % 2000);
        ylog_close(&log_obj);
        CHECK(after > 0, "no samples accepted after the cut (trial %d)", trial);

        verify_t v = verify(&f);
        if (only && (v.bad || v.order_bad))
            dump_flash(&f);
        recovered_total += v.count;
        bad_total += v.bad + v.order_bad;
        if (v.bad || v.order_bad) {
            CHECK(0, "trial %d: %u wrong samples, %u out of order", trial, (unsigned)v.bad, (unsigned)v.order_bad);
            break;
        }
        CHECK(f.errors == 0, "trial %d: flash rule violations %d", trial, f.errors);
    }
    printf("power cuts: %u cuts in 3000 trials, %u samples recovered, %u wrong\n", (unsigned)cuts,
           (unsigned)recovered_total, (unsigned)bad_total);
    CHECK(cuts > 1000, "too few cuts actually happened (%u)", (unsigned)cuts);
}

int main(void)
{
    for (uint32_t i = 0; i < TRUTH_N; i++)
        truth[i] = poisson(10);
    test_wrap_clean();
    test_stats_ignore_unset_clock();
    test_sessions(true);
    test_sessions(false);
    test_power_cuts();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall ylog checks passed\n", failures);
    return failures ? 1 : 0;
}
