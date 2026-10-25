/* Bins from the device's log: gaps, zeros, limits, and the base64 page reply. */
#include "yarrow_backfill.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PSIZE 1024u
#define NPAGES 16u
#define T0 1792000800u        /* a multiple of 600 */
#define IV 60u
#define N 300u                /* five hours of one-minute samples */

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint8_t mem[NPAGES * PSIZE];
static int rf_read(void *c, uint32_t a, void *b, uint32_t n) { (void)c; memcpy(b, mem + a, n); return 0; }
static int rf_prog(void *c, uint32_t a, const void *b, uint32_t n)
{
    (void)c;
    const uint8_t *s = b;
    for (uint32_t i = 0; i < n; i++)
        mem[a + i] &= s[i];
    return 0;
}
static int rf_erase(void *c, uint32_t p) { (void)c; memset(mem + p * PSIZE, 0xFF, PSIZE); return 0; }

static uint32_t val[N];
static uint32_t got_start[64], got_count[64], got_n;
static void emit(void *ctx, uint32_t start, uint32_t count)
{
    (void)ctx;
    if (got_n < 64) {
        got_start[got_n] = start;
        got_count[got_n] = count;
    }
    got_n++;
}

static ylog_flash_t fl = {PSIZE, NPAGES, rf_read, rf_prog, rf_erase, 0};
static uint8_t scratch[PSIZE];

static uint32_t truth_bin(uint32_t b)
{
    uint32_t s = 0;
    for (uint32_t i = b * 10; i < b * 10 + 10; i++)
        s += val[i];
    return s;
}

int main(void)
{
    memset(mem, 0xFF, sizeof mem);
    static ylog_t log;
    CHECK(ylog_open(&log, &fl) == 0, "open");
    uint32_t seed = 12345;
    for (uint32_t i = 0; i < N; i++) {
        seed = seed * 1664525u + 1013904223u;
        val[i] = 8 + (seed >> 24) % 5;             /* about 10 a minute */
    }
    val[200] = 0;                                    /* a minute without a pulse: the tube was off */
    for (uint32_t i = 0; i < N; i++) {
        if (i >= 100 && i <= 124)
            continue;                                /* the device was off for 25 minutes */
        ylog_add(&log, T0 + i * IV, IV, true, val[i]);
    }
    ylog_close(&log);

    got_n = 0;
    uint32_t n = ybf_bins(&fl, scratch, 600, T0, T0 + N * IV, emit, 0);
    CHECK(n == 26 && got_n == 26, "26 bins of 30 (hole takes 3, the zero minute 1): %u", (unsigned)n);
    int wrong = 0, order = 0;
    for (uint32_t i = 0; i < got_n && i < 64; i++) {
        uint32_t b = (got_start[i] - T0) / 600;
        if (got_start[i] % 600 || got_count[i] != truth_bin(b))
            wrong++;
        if (i && got_start[i] <= got_start[i - 1])
            order++;
        CHECK(!(b >= 10 && b <= 12) && b != 20, "bin %u should not be there", (unsigned)b);
    }
    CHECK(!wrong && !order, "counts match the truth and come in order (%d wrong, %d out of order)", wrong, order);

    got_n = 0;
    n = ybf_bins(&fl, scratch, 600, T0 + 1200, T0 + 3600, emit, 0);
    CHECK(n == 4 && got_start[0] == T0 + 1200 && got_start[3] == T0 + 3000, "the window from/until holds (%u bins)", (unsigned)n);
    got_n = 0;
    n = ybf_bins(&fl, scratch, 600, T0 + 1250, T0 + 3550, emit, 0);
    CHECK(n == 2, "bins that are not wholly inside the window are left out (%u)", (unsigned)n);
    got_n = 0;
    n = ybf_bins(&fl, scratch, 1800, T0, T0 + N * IV, emit, 0);
    CHECK(n == 7, "half-hour bins: of ten, the hole spoils two and the zero minute one (%u)", (unsigned)n);
    got_n = 0;
    n = ybf_bins(&fl, scratch, 100, T0, T0 + N * IV, emit, 0);
    CHECK(n == 0, "a bin length the samples do not divide gives nothing (%u)", (unsigned)n);

    /* the page reply: what the firmware sends is base64 lines, END */
    {
        static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        static char reply[2 * PSIZE + 400];
        size_t k = (size_t)snprintf(reply, sizeof reply, "OK 3 %u\n", PSIZE);
        uint8_t *d = mem + 3 * PSIZE;
        int col = 0;
        for (uint32_t i = 0; i < PSIZE; i += 3) {
            uint32_t v = (uint32_t)d[i] << 16 | (i + 1 < PSIZE ? (uint32_t)d[i + 1] << 8 : 0) | (i + 2 < PSIZE ? d[i + 2] : 0);
            reply[k++] = tbl[(v >> 18) & 63];
            reply[k++] = tbl[(v >> 12) & 63];
            reply[k++] = i + 1 < PSIZE ? tbl[(v >> 6) & 63] : '=';
            reply[k++] = i + 2 < PSIZE ? tbl[v & 63] : '=';
            col += 4;
            if (col >= 76) {
                reply[k++] = '\n';
                col = 0;
            }
        }
        if (col)
            reply[k++] = '\n';
        k += (size_t)snprintf(reply + k, 8, "END\n");
        static uint8_t out[PSIZE];
        int m = ybf_page_from_reply(reply, out, PSIZE);
        CHECK(m == (int)PSIZE && memcmp(out, d, PSIZE) == 0, "a LOGPAGE reply decodes to the page (%d bytes)", m);
        reply[k - 6] = '!';
        CHECK(ybf_page_from_reply(reply, out, PSIZE) == -1, "a damaged reply is refused");
    }
    printf(failures ? "\n%d check(s) FAILED\n" : "all backfill checks passed\n", failures);
    return failures ? 1 : 0;
}
