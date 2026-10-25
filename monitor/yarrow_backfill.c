#include "yarrow_backfill.h"

#include <string.h>

#define ZERO_RUN_S 120   /* this many seconds without one pulse: the tube voltage was off, not the radiation */

typedef struct {
    uint32_t interval, from, until;
    ybin_emit_fn emit;
    void *ctx;
    uint32_t n;

    int open;            /* a bin is being collected */
    uint32_t bs;         /* its start */
    uint32_t next;       /* time the next sample must have */
    uint32_t covered;
    uint64_t sum;
    uint32_t zero_run;
    int bad;
    uint32_t last_start; /* start of the last bin emitted */
} bf_t;

static void finish(bf_t *b)
{
    if (!b->open)
        return;
    b->open = 0;
    if (b->bad || b->covered != b->interval || b->sum == 0 || b->sum > 0xFFFFFFFFu)
        return;
    if (b->bs < b->from || b->bs + b->interval > b->until)
        return;
    if (b->n && b->bs <= b->last_start)
        return;
    b->last_start = b->bs;
    b->emit(b->ctx, b->bs, (uint32_t)b->sum);
    b->n++;
}

static bool visit(void *user, uint32_t t, uint32_t iv, bool valid, uint32_t v)
{
    bf_t *b = user;
    if (!valid || t < YARROW_MIN_TIME || iv == 0 || b->interval % iv != 0 || t % iv != 0) {
        finish(b);
        return true;
    }
    uint32_t bs = t - t % b->interval;
    if (b->open && bs != b->bs)
        finish(b);
    if (!b->open) {
        b->open = 1;
        b->bs = bs;
        b->next = bs;
        b->covered = 0;
        b->sum = 0;
        b->zero_run = 0;
        b->bad = 0;
    }
    if (t != b->next) {                          /* a hole or a repeat inside the bin */
        b->bad = 1;
        b->next = t;
    }
    b->next = t + iv;
    b->covered += iv;
    b->sum += v;
    if (v == 0) {
        b->zero_run += iv;
        if (b->zero_run >= ZERO_RUN_S || iv >= 60)
            b->bad = 1;
    } else {
        b->zero_run = 0;
    }
    return true;
}

uint32_t ybf_bins(const ylog_flash_t *fl, uint8_t *scratch, uint32_t interval, uint32_t from, uint32_t until, ybin_emit_fn emit,
                  void *ctx)
{
    bf_t b;
    memset(&b, 0, sizeof b);
    b.interval = interval;
    b.from = from;
    b.until = until;
    b.emit = emit;
    b.ctx = ctx;
    ylog_scan(fl, scratch, visit, &b);
    finish(&b);
    return b.n;
}

static int b64val(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int ybf_page_from_reply(const char *reply, uint8_t *out, uint32_t size)
{
    const char *p = strchr(reply, '\n');          /* skip "OK page size" */
    if (!p)
        return -1;
    uint32_t n = 0;
    unsigned acc = 0;
    int bits = 0;
    for (p++; *p; p++) {
        if (p[0] == 'E' && p[1] == 'N' && p[2] == 'D')
            break;
        if (*p == '\n' || *p == '\r' || *p == '=')
            continue;
        int v = b64val(*p);
        if (v < 0)
            return -1;
        acc = (acc << 6) | (unsigned)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n >= size)
                return -1;
            out[n++] = (uint8_t)(acc >> bits);
            acc &= (1u << bits) - 1;
        }
    }
    return (int)n;
}
