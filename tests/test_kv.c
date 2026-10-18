/* Key/value store tests on a RAM model of two flash pages with power-cut injection. */
#include "kv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PSIZE 1024u

static uint64_t rng_state = 0xD1B54A32D192ED03ull;
static uint32_t rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint32_t)(rng_state >> 11);
}

typedef struct {
    uint8_t mem[2 * PSIZE];
    long ops_left;
    bool dead;
    int errors;
    long ops_done;
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
    f->ops_done++;
    bool cut = f->ops_left >= 0 && --f->ops_left < 0;
    uint32_t cut_at = cut ? rnd() % (n / 2 + 1) : n / 2;
    for (uint32_t i = 0; i < n / 2; i++) {
        uint8_t *m = f->mem + a + 2 * i;
        if (i < cut_at) {
            if (m[0] != 0xFF || m[1] != 0xFF)
                f->errors++;
            m[0] &= s[2 * i];
            m[1] &= s[2 * i + 1];
        } else if (i == cut_at && cut) {
            m[0] &= (uint8_t)(s[2 * i] | (uint8_t)rnd());
            m[1] &= (uint8_t)(s[2 * i + 1] | (uint8_t)rnd());
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
    if (f->dead || page >= 2)
        return -1;
    f->ops_done++;
    if (f->ops_left >= 0 && --f->ops_left < 0) {
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
    ylog_flash_t d = {PSIZE, 2, rf_read, rf_prog, rf_erase, f};
    return d;
}

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static kv_t kv_obj;

typedef struct {
    bool set[256];
    uint8_t len[256];
    uint8_t val[256][KV_MAX_VALUE];
} model_t;

static void check_model(kv_t *kv, const model_t *m, int skip_key, const char *what)
{
    for (int k = 0; k < 256; k++) {
        if (k == skip_key)
            continue;
        uint8_t buf[KV_MAX_VALUE];
        int n = kv_get(kv, (uint8_t)k, buf, sizeof buf);
        if (!m->set[k]) {
            CHECK(n == -1, "%s: key %d should be unset", what, k);
        } else {
            CHECK(n == m->len[k] && memcmp(buf, m->val[k], m->len[k]) == 0, "%s: key %d has the wrong value", what, k);
        }
    }
}

static void test_basic(void)
{
    ramflash_t f;
    rf_init(&f);
    ylog_flash_t d = rf_dev(&f);
    CHECK(kv_open(&kv_obj, &d) == 0, "open blank");
    model_t m;
    memset(&m, 0, sizeof m);

    for (int i = 0; i < 4000; i++) {
        uint8_t key = (uint8_t)(rnd() % 20);
        uint8_t len = (uint8_t)(rnd() % (KV_MAX_VALUE + 1));
        uint8_t val[KV_MAX_VALUE];
        for (int k = 0; k < len; k++)
            val[k] = (uint8_t)rnd();
        CHECK(kv_set(&kv_obj, key, val, len) == 0, "set %d", i);
        m.set[key] = true;
        m.len[key] = len;
        memcpy(m.val[key], val, len);
    }
    check_model(&kv_obj, &m, -1, "after 4000 writes");

    CHECK(kv_open(&kv_obj, &d) == 0, "reopen");
    check_model(&kv_obj, &m, -1, "after reopen");

    long before = f.ops_done;
    uint8_t buf[KV_MAX_VALUE];
    int n = kv_get(&kv_obj, 3, buf, sizeof buf);
    if (n >= 0)
        kv_set(&kv_obj, 3, buf, (uint32_t)n);
    CHECK(f.ops_done == before, "unchanged write used flash");

    uint32_t v;
    CHECK(kv_set_u32(&kv_obj, 200, 0xDEADBEEF) == 0 && kv_get_u32(&kv_obj, 200, &v) && v == 0xDEADBEEF, "u32 helper");
    CHECK(f.errors == 0, "flash rule violations: %d", f.errors);
    printf("basic: ok (%ld flash operations for 4000 writes)\n", f.ops_done);
}

static void test_power_cuts(void)
{
    long cuts = 0;
    for (int trial = 0; trial < 6000; trial++) {
        rng_state = 0xD1B54A32D192ED03ull ^ ((uint64_t)(trial + 1) * 0x2545F4914F6CDD1Dull);
        for (int w = 0; w < 4; w++)
            rnd();
        ramflash_t f;
        rf_init(&f);
        ylog_flash_t d = rf_dev(&f);
        model_t m;
        memset(&m, 0, sizeof m);
        int pending = -1;
        uint8_t pend_val[KV_MAX_VALUE];
        uint8_t pend_len = 0;

        CHECK(kv_open(&kv_obj, &d) == 0, "initial open");

        for (int round = 0; round < 4; round++) {
            f.ops_left = (long)(rnd() % 300);
            for (int i = 0; i < 400; i++) {
                uint8_t key = (uint8_t)(rnd() % 12);
                uint8_t len = (uint8_t)(rnd() % (KV_MAX_VALUE + 1));
                uint8_t val[KV_MAX_VALUE];
                for (int k = 0; k < len; k++)
                    val[k] = (uint8_t)rnd();
                if (kv_set(&kv_obj, key, val, len) == 0) {
                    m.set[key] = true;
                    m.len[key] = len;
                    memcpy(m.val[key], val, len);
                    pending = -1;
                } else {
                    pending = key; /* cut: this write may or may not have landed */
                    pend_len = len;
                    memcpy(pend_val, val, len);
                    break;
                }
            }
            if (f.dead)
                cuts++;

            f.dead = false;
            f.ops_left = -1;
            if (rnd() % 4 == 0) { /* sometimes the power is cut again while recovering */
                f.ops_left = (long)(rnd() % 6);
                kv_open(&kv_obj, &d);
                f.dead = false;
                f.ops_left = -1;
            }
            if (kv_open(&kv_obj, &d) != 0) {
                CHECK(0, "trial %d round %d: reopen failed", trial, round);
                return;
            }

            check_model(&kv_obj, &m, pending, "after cut");
            if (pending >= 0) {
                uint8_t buf[KV_MAX_VALUE];
                int n = kv_get(&kv_obj, (uint8_t)pending, buf, sizeof buf);
                bool old_ok = m.set[pending] ? (n == m.len[pending] && memcmp(buf, m.val[pending], m.len[pending]) == 0) : n == -1;
                bool new_ok = n == pend_len && memcmp(buf, pend_val, pend_len) == 0;
                CHECK(old_ok || new_ok, "trial %d: interrupted key %d holds neither old nor new value", trial, pending);
                if (new_ok) {
                    m.set[pending] = true;
                    m.len[pending] = pend_len;
                    memcpy(m.val[pending], pend_val, pend_len);
                }
                pending = -1;
            }
        }
        CHECK(f.errors == 0, "trial %d: flash rule violations %d", trial, f.errors);
        if (failures > 5)
            return;
    }
    printf("power cuts: %ld cuts in 6000 trials, no lost or wrong values\n", cuts);
    CHECK(cuts > 3000, "too few cuts happened (%ld)", cuts);
}

int main(void)
{
    test_basic();
    test_power_cuts();
    printf(failures ? "\n%d check(s) FAILED\n" : "\nall kv checks passed\n", failures);
    return failures ? 1 : 0;
}
