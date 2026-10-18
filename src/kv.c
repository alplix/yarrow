#include "kv.h"
#include <stddef.h>
#include <string.h>

/* Page:   [8 byte header: "KV", seq32, check, 0x01]  [record]...
 * Record: [0xB5, key, len, hdrcheck] [payload, padded to even] [crc16 of key, len, payload] */
#define PAGE_HDR 8u
#define REC_MAGIC 0xB5
#define NO_REC 0xFFFFu

static uint32_t even_up(uint32_t v) { return (v + 1u) & ~1u; }
static uint32_t rd32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

static uint16_t crc16(const uint8_t *p, uint32_t n)
{
    uint16_t c = 0xFFFF;
    while (n--) {
        c ^= (uint16_t)(*p++ << 8);
        for (int i = 0; i < 8; i++)
            c = (c & 0x8000) ? (uint16_t)((c << 1) ^ 0x1021) : (uint16_t)(c << 1);
    }
    return c;
}

static uint8_t hdr_check(const uint8_t *h) { return (uint8_t)(h[0] ^ h[1] ^ h[2] ^ 0x3C); }

static int hdr_ok(const uint8_t *h, uint32_t *seq)
{
    uint8_t c = 0xA5;
    for (int i = 0; i < 6; i++)
        c ^= h[i];
    if (h[0] != 'K' || h[1] != 'V' || h[6] != c)
        return 0;
    *seq = rd32(h + 2);
    return 1;
}

static uint32_t last_non_ff(const uint8_t *pg, uint32_t from, uint32_t to)
{
    for (uint32_t i = to; i > from; i--)
        if (pg[i - 1] != 0xFF)
            return i;
    return from;
}

/* Result of walking a page. */
typedef struct {
    uint16_t at[256];  /* offset of the newest valid record per key */
    uint32_t end;      /* offset after the last record that could be parsed */
    bool clean;        /* the walk reached blank flash with nothing odd in between */
} walk_t;

static void walk(const uint8_t *pg, uint32_t size, walk_t *w)
{
    for (int i = 0; i < 256; i++)
        w->at[i] = NO_REC;
    uint32_t off = PAGE_HDR;
    w->clean = true;
    while (off + 4 <= size) {
        const uint8_t *r = pg + off;
        if (r[0] == 0xFF && r[1] == 0xFF && r[2] == 0xFF && r[3] == 0xFF)
            break; /* blank: end of records */
        if (r[0] != REC_MAGIC || r[3] != hdr_check(r) || r[2] > KV_MAX_VALUE) {
            w->clean = false; /* torn or damaged header: nothing after it can be trusted */
            break;
        }
        uint32_t total = 4 + even_up(r[2]) + 2;
        if (off + total > size) {
            w->clean = false;
            break;
        }
        uint16_t stored = (uint16_t)(pg[off + total - 2] | pg[off + total - 1] << 8);
        uint8_t tmp[2 + KV_MAX_VALUE];
        tmp[0] = r[1];
        tmp[1] = r[2];
        memcpy(tmp + 2, r + 4, r[2]);
        if (stored == crc16(tmp, 2u + r[2]))
            w->at[r[1]] = (uint16_t)off;
        else
            w->clean = false; /* torn payload: skipped, but the record length is known so we can go on */
        off += total;
    }
    w->end = off;
}

static int fl_read(const ylog_flash_t *fl, uint32_t a, void *b, uint32_t n) { return fl->read(fl->ctx, a, b, n); }

/* Builds one record into out; returns its length. */
static uint32_t make_record(uint8_t *out, uint8_t key, const uint8_t *data, uint32_t len)
{
    out[0] = REC_MAGIC;
    out[1] = key;
    out[2] = (uint8_t)len;
    out[3] = hdr_check(out);
    memcpy(out + 4, data, len);
    if (len & 1u)
        out[4 + len] = 0xFF;
    uint8_t tmp[2 + KV_MAX_VALUE];
    tmp[0] = key;
    tmp[1] = (uint8_t)len;
    memcpy(tmp + 2, data, len);
    uint16_t c = crc16(tmp, 2 + len);
    uint32_t total = 4 + even_up(len) + 2;
    out[total - 2] = (uint8_t)c;
    out[total - 1] = (uint8_t)(c >> 8);
    return total;
}

/* Copies the live records of the active page to the other page and makes that one active. */
static int compact(kv_t *kv)
{
    const ylog_flash_t *fl = kv->fl;
    walk_t w;
    walk(kv->page, fl->page_size, &w);

    uint32_t target = kv->active ^ 1u;
    uint32_t base = target * fl->page_size;
    if (fl->erase(fl->ctx, target) != 0)
        return -1;

    uint32_t off = PAGE_HDR;
    uint8_t rec[4 + KV_MAX_VALUE + 2 + 2];
    for (int key = 0; key < 256; key++) {
        if (w.at[key] == NO_REC)
            continue;
        const uint8_t *r = kv->page + w.at[key];
        uint32_t n = make_record(rec, (uint8_t)key, r + 4, r[2]);
        if (off + n > fl->page_size)
            return -1; /* cannot happen with sane use: live data must fit a page */
        if (fl->prog(fl->ctx, base + off, rec, n) != 0)
            return -1;
        off += n;
    }

    uint8_t h[PAGE_HDR] = {'K', 'V', 0, 0, 0, 0, 0, 0x01};
    wr32(h + 2, kv->seq + 1);
    uint8_t c = 0xA5;
    for (int i = 0; i < 6; i++)
        c ^= h[i];
    h[6] = c;
    if (fl->prog(fl->ctx, base, h, PAGE_HDR) != 0) /* the page becomes valid only now */
        return -1;

    kv->active = target;
    kv->seq++;
    kv->free_off = off;
    return fl_read(fl, base, kv->page, fl->page_size);
}

int kv_open(kv_t *kv, const ylog_flash_t *fl)
{
    memset(kv, 0, offsetof(kv_t, page));
    kv->fl = fl;
    if (fl->page_count != 2 || fl->page_size > KV_MAX_PAGE || fl->page_size < 128 || (fl->page_size & 1u))
        return -1;

    uint8_t h[8];
    bool valid[2] = {false, false};
    uint32_t seq[2] = {0, 0};
    for (uint32_t p = 0; p < 2; p++) {
        if (fl_read(fl, p * fl->page_size, h, 8) != 0)
            return -1;
        valid[p] = hdr_ok(h, &seq[p]);
    }

    if (!valid[0] && !valid[1]) { /* first use: start an empty page 0 */
        if (fl->erase(fl->ctx, 0) != 0)
            return -1;
        uint8_t nh[PAGE_HDR] = {'K', 'V', 1, 0, 0, 0, 0, 0x01};
        uint8_t c = 0xA5;
        for (int i = 0; i < 6; i++)
            c ^= nh[i];
        nh[6] = c;
        if (fl->prog(fl->ctx, 0, nh, PAGE_HDR) != 0)
            return -1;
        kv->active = 0;
        kv->seq = 1;
    } else {
        kv->active = (valid[0] && valid[1]) ? ((int32_t)(seq[1] - seq[0]) > 0 ? 1u : 0u) : (valid[1] ? 1u : 0u);
        kv->seq = seq[kv->active];
    }

    if (fl_read(fl, kv->active * fl->page_size, kv->page, fl->page_size) != 0)
        return -1;

    walk_t w;
    walk(kv->page, fl->page_size, &w);
    uint32_t used = even_up(last_non_ff(kv->page, PAGE_HDR, fl->page_size));
    kv->free_off = used > w.end ? used : w.end;

    /* anything torn or unparsable is cleaned out by moving the live records to a fresh page */
    if (!w.clean || used > w.end) {
        if (compact(kv) != 0) {
            kv->failed = true;
            return -1;
        }
    }
    return 0;
}

int kv_get(const kv_t *kv, uint8_t key, void *buf, uint32_t max)
{
    walk_t w;
    walk(kv->page, kv->fl->page_size, &w);
    if (w.at[key] == NO_REC)
        return -1;
    const uint8_t *r = kv->page + w.at[key];
    uint32_t n = r[2] < max ? r[2] : max;
    memcpy(buf, r + 4, n);
    return r[2];
}

int kv_set(kv_t *kv, uint8_t key, const void *data, uint32_t len)
{
    if (kv->failed || len > KV_MAX_VALUE)
        return -1;
    const ylog_flash_t *fl = kv->fl;

    /* no change, no write: saves flash */
    uint8_t cur[KV_MAX_VALUE];
    int have = kv_get(kv, key, cur, sizeof cur);
    if (have == (int)len && memcmp(cur, data, len) == 0)
        return 0;

    uint8_t rec[4 + KV_MAX_VALUE + 2 + 2];
    uint32_t n = make_record(rec, key, data, len);
    if (kv->free_off + n > fl->page_size) {
        if (compact(kv) != 0) {
            kv->failed = true;
            return -1;
        }
        if (kv->free_off + n > fl->page_size) {
            kv->failed = true;
            return -1;
        }
    }
    uint32_t addr = kv->active * fl->page_size + kv->free_off;
    if (fl->prog(fl->ctx, addr, rec, n) != 0) {
        kv->failed = true;
        return -1;
    }
    memcpy(kv->page + kv->free_off, rec, n);
    kv->free_off += n;
    return 0;
}

bool kv_get_u32(const kv_t *kv, uint8_t key, uint32_t *v)
{
    uint8_t b[4];
    if (kv_get(kv, key, b, 4) != 4)
        return false;
    *v = rd32(b);
    return true;
}

int kv_set_u32(kv_t *kv, uint8_t key, uint32_t v)
{
    uint8_t b[4];
    wr32(b, v);
    return kv_set(kv, key, b, 4);
}
