#include "ylog.h"
#include <stddef.h>
#include <string.h>

/* Page:    [8 byte header: "YL", seq32, check, version] [segment] [segment] ...
 * Segment: [14 byte header: magic, flags, start32, interval16, mean24, check, len16] [ylc stream]
 * len stays 0xFFFF (blank) until the segment is closed or recovered. The stream's real end is always taken
 * from the last non-blank byte, so a blank or torn len can never extend a stream into blank flash. */
#define PAGE_HDR 8u
#define SEG_HDR  14u
#define MIN_STREAM 64u

#define PAGE_MAGIC0 0x59
#define PAGE_MAGIC1 0x4C
#define SEG_MAGIC   0xA7
#define FLAG_TIME_VALID 1u

/* ---- small helpers ---- */

static uint32_t rd32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t rd16(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8; }
static uint32_t even_up(uint32_t v) { return (v + 1u) & ~1u; }

static uint8_t xor_check(const uint8_t *p, unsigned n, uint8_t salt)
{
    uint8_t c = salt;
    while (n--)
        c ^= *p++;
    return c;
}

static int page_header_ok(const uint8_t *h, uint32_t *seq)
{
    if (h[0] != PAGE_MAGIC0 || h[1] != PAGE_MAGIC1 || h[6] != xor_check(h, 6, 0xA5))
        return 0;
    *seq = rd32(h + 2);
    return 1;
}

/* One past the last byte that is not blank (0xFF) in [from, to), or `from` if the range is blank. */
static uint32_t last_non_ff(const uint8_t *pg, uint32_t from, uint32_t to)
{
    for (uint32_t i = to; i > from; i--)
        if (pg[i - 1] != 0xFF)
            return i;
    return from;
}

typedef struct {
    uint32_t off;          /* header offset in the page */
    uint32_t start_time;
    uint32_t interval;
    bool time_valid;
    uint32_t mean_q8;
    uint32_t stream_off;   /* first stream byte in the page */
    uint32_t data_end;     /* one past the last used stream byte (page offset) */
    uint32_t valid_len;    /* stream bytes the decoder may rely on */
    bool open;             /* len was never recorded */
    bool len_torn;         /* len holds an impossible value that cannot be rewritten */
    uint32_t end;          /* first byte after this segment */
} seg_t;

/* Parses the segment at `off` in a loaded page. Returns 0 when there is none (free space or damage). */
static int parse_segment(const uint8_t *pg, uint32_t size, uint32_t off, seg_t *s)
{
    if (off + SEG_HDR > size)
        return 0;
    const uint8_t *h = pg + off;
    if (h[0] != SEG_MAGIC || h[11] != xor_check(h, 11, 0x5A))
        return 0;

    s->off = off;
    s->time_valid = (h[1] & FLAG_TIME_VALID) != 0;
    s->start_time = rd32(h + 2);
    s->interval = rd16(h + 6);
    s->mean_q8 = h[8] | (uint32_t)h[9] << 8 | (uint32_t)h[10] << 16;
    s->stream_off = off + SEG_HDR;

    uint32_t len = rd16(h + 12);
    s->open = len == 0xFFFF;
    s->len_torn = !s->open && s->stream_off + len > size;
    uint32_t limit = (s->open || s->len_torn) ? size : s->stream_off + len;
    s->data_end = last_non_ff(pg, s->stream_off, limit);

    /* The last halfword written may be half programmed: never rely on it. */
    uint32_t used = s->data_end - s->stream_off;
    s->valid_len = used ? (used - 1) & ~1u : 0;
    s->end = (s->open || s->len_torn) ? s->stream_off + even_up(used) : s->stream_off + even_up(len);
    return 1;
}

/* ---- opening ---- */

static int fl_read(const ylog_flash_t *fl, uint32_t addr, void *buf, uint32_t n) { return fl->read(fl->ctx, addr, buf, n); }

static void fail(ylog_t *l) { l->failed = true; }

int ylog_open(ylog_t *l, const ylog_flash_t *fl)
{
    memset(l, 0, offsetof(ylog_t, buf));
    l->fl = fl;
    if (fl->page_size > YLOG_MAX_PAGE || (fl->page_size & 1u) || fl->page_count == 0 || fl->page_size < 256)
        return -1;

    uint32_t best = 0, best_seq = 0;
    bool found = false;
    uint8_t h[8];
    for (uint32_t p = 0; p < fl->page_count; p++) {
        uint32_t seq;
        if (fl_read(fl, p * fl->page_size, h, 8) != 0)
            return -1;
        if (!page_header_ok(h, &seq))
            continue;
        if (!found || (int32_t)(seq - best_seq) > 0) {
            best = p;
            best_seq = seq;
            found = true;
        }
    }

    if (!found) { /* empty region: the first page used will be page 0 */
        l->cur_page = fl->page_count - 1;
        l->cur_seq = 0;
        l->page_open = false;
        return 0;
    }

    l->cur_page = best;
    l->cur_seq = best_seq;
    uint8_t *pg = l->buf;
    if (fl_read(fl, best * fl->page_size, pg, fl->page_size) != 0)
        return -1;

    /* Walk the segments: remember the last one's final mean and record its length if it was left open. */
    uint32_t off = PAGE_HDR;
    seg_t s, last = {0};
    bool have_last = false;
    while (parse_segment(pg, fl->page_size, off, &s)) {
        last = s;
        have_last = true;
        off = s.end;
    }

    bool page_sealed = false;
    if (have_last) {
        ylc_dec_t d;
        ylc_dec_init(&d, pg + last.stream_off, last.valid_len, last.mean_q8);
        uint32_t x;
        while (ylc_dec_get(&d, &x)) {}
        l->last_mean_q8 = d.model.mean_q8;

        if (last.len_torn) {
            page_sealed = true; /* cannot be repaired: leave the rest of this page alone */
        } else if (last.open) {
            uint32_t len = even_up(last.data_end - last.stream_off);
            uint8_t lenb[2] = {(uint8_t)len, (uint8_t)(len >> 8)};
            if (fl->prog(fl->ctx, best * fl->page_size + last.off + 12, lenb, 2) != 0)
                fail(l);
        }
    }

    /* Anything non-blank after the parsed segments (damaged header, torn write) stays untouched. */
    uint32_t used = even_up(last_non_ff(pg, PAGE_HDR, fl->page_size));
    if (used < off)
        used = off;
    if (page_sealed)
        used = fl->page_size;
    l->page_free = used;
    l->page_open = fl->page_size - used >= SEG_HDR + MIN_STREAM;
    return l->failed ? -1 : 0;
}

/* ---- writing ---- */

static bool start_page(ylog_t *l)
{
    const ylog_flash_t *fl = l->fl;
    uint32_t next = (l->cur_page + 1) % fl->page_count;
    uint32_t base = next * fl->page_size;

    /* erase unless the page is already blank */
    bool blank = true;
    for (uint32_t i = 0; i < fl->page_size && blank; i += 64) {
        uint8_t chunk[64];
        uint32_t n = fl->page_size - i < 64 ? fl->page_size - i : 64;
        if (fl_read(fl, base + i, chunk, n) != 0) { fail(l); return false; }
        for (uint32_t k = 0; k < n; k++)
            if (chunk[k] != 0xFF) { blank = false; break; }
    }
    if (!blank && fl->erase(fl->ctx, next) != 0) { fail(l); return false; }

    uint8_t h[PAGE_HDR];
    h[0] = PAGE_MAGIC0;
    h[1] = PAGE_MAGIC1;
    wr32(h + 2, l->cur_seq + 1);
    h[6] = xor_check(h, 6, 0xA5);
    h[7] = 0x01;
    if (fl->prog(fl->ctx, base, h, PAGE_HDR) != 0) { fail(l); return false; }

    l->cur_page = next;
    l->cur_seq++;
    l->page_free = PAGE_HDR;
    l->page_open = true;
    return true;
}

static bool open_segment(ylog_t *l, uint32_t time, uint32_t interval, bool time_valid)
{
    const ylog_flash_t *fl = l->fl;
    if (!l->page_open || fl->page_size - l->page_free < SEG_HDR + MIN_STREAM) {
        if (!start_page(l))
            return false;
    }

    uint32_t mean = l->last_mean_q8 > 0xFFFFFFu ? 0xFFFFFFu : l->last_mean_q8;
    uint32_t iv = interval > 0xFFFF ? 0xFFFF : interval;
    uint8_t h[SEG_HDR];
    h[0] = SEG_MAGIC;
    h[1] = time_valid ? FLAG_TIME_VALID : 0;
    wr32(h + 2, time);
    h[6] = (uint8_t)iv;
    h[7] = (uint8_t)(iv >> 8);
    h[8] = (uint8_t)mean;
    h[9] = (uint8_t)(mean >> 8);
    h[10] = (uint8_t)(mean >> 16);
    h[11] = xor_check(h, 11, 0x5A);
    h[12] = h[13] = 0xFF;

    uint32_t base = l->cur_page * fl->page_size;
    if (fl->prog(fl->ctx, base + l->page_free, h, SEG_HDR) != 0) { fail(l); return false; }

    l->seg_off = l->page_free;
    l->seg_start = time;
    l->seg_interval = interval;
    l->seg_next_time = time;
    l->seg_written = 0;
    l->page_free += SEG_HDR;
    /* leave room for the two zero bytes that guard a finished stream */
    ylc_enc_init(&l->enc, l->buf, fl->page_size - l->page_free - 4, mean);
    l->seg_open = true;
    return true;
}

void ylog_sync(ylog_t *l)
{
    if (!l->seg_open || l->failed)
        return;
    uint32_t target = l->enc.len & ~1u;
    if (target > l->seg_written) {
        uint32_t addr = l->cur_page * l->fl->page_size + l->seg_off + SEG_HDR + l->seg_written;
        if (l->fl->prog(l->fl->ctx, addr, l->buf + l->seg_written, target - l->seg_written) != 0) {
            fail(l);
            return;
        }
        l->seg_written = target;
    }
}

static void close_segment(ylog_t *l)
{
    if (!l->seg_open)
        return;
    l->seg_open = false;
    if (l->failed)
        return;

    uint32_t len = ylc_enc_finish(&l->enc);
    /* pad to a whole halfword, then add a zero halfword: the decoder never trusts the last halfword */
    while (len & 1u)
        l->buf[len++] = 0x00;
    l->buf[len++] = 0x00;
    l->buf[len++] = 0x00;

    const ylog_flash_t *fl = l->fl;
    uint32_t base = l->cur_page * fl->page_size + l->seg_off;
    if (len > l->seg_written &&
        fl->prog(fl->ctx, base + SEG_HDR + l->seg_written, l->buf + l->seg_written, len - l->seg_written) != 0) {
        fail(l);
        return;
    }
    uint8_t lb[2] = {(uint8_t)len, (uint8_t)(len >> 8)};
    if (fl->prog(fl->ctx, base + 12, lb, 2) != 0) {
        fail(l);
        return;
    }
    l->last_mean_q8 = l->enc.model.mean_q8;
    l->page_free = l->seg_off + SEG_HDR + len;
    l->page_open = fl->page_size - l->page_free >= SEG_HDR + MIN_STREAM;
}

bool ylog_add(ylog_t *l, uint32_t time, uint32_t interval, bool time_valid, uint32_t value)
{
    if (l->failed)
        return false;

    if (l->seg_open && (time != l->seg_next_time || interval != l->seg_interval))
        close_segment(l);

    if (!l->seg_open && !open_segment(l, time, interval, time_valid))
        return false;

    if (!ylc_enc_put(&l->enc, value)) { /* page full: finish it and continue on the next */
        close_segment(l);
        l->page_open = false;
        if (!open_segment(l, time, interval, time_valid) || !ylc_enc_put(&l->enc, value)) {
            fail(l);
            return false;
        }
    }
    l->seg_next_time = time + interval;
    ylog_sync(l);
    return !l->failed;
}

void ylog_close(ylog_t *l) { close_segment(l); }

/* ---- reading ---- */

static bool newest_page(const ylog_flash_t *fl, uint32_t *page)
{
    bool found = false;
    uint32_t best_seq = 0;
    uint8_t h[8];
    for (uint32_t p = 0; p < fl->page_count; p++) {
        uint32_t seq;
        if (fl_read(fl, p * fl->page_size, h, 8) != 0 || !page_header_ok(h, &seq))
            continue;
        if (!found || (int32_t)(seq - best_seq) > 0) {
            *page = p;
            best_seq = seq;
            found = true;
        }
    }
    return found;
}

void ylog_scan(const ylog_flash_t *fl, uint8_t *pg, ylog_visit_fn fn, void *user)
{
    uint32_t newest;
    if (!newest_page(fl, &newest))
        return;

    for (uint32_t k = 1; k <= fl->page_count; k++) {
        uint32_t p = (newest + k) % fl->page_count;
        uint32_t seq;
        if (fl_read(fl, p * fl->page_size, pg, fl->page_size) != 0 || !page_header_ok(pg, &seq))
            continue;

        uint32_t off = PAGE_HDR;
        seg_t s;
        while (parse_segment(pg, fl->page_size, off, &s)) {
            ylc_dec_t d;
            ylc_dec_init(&d, pg + s.stream_off, s.valid_len, s.mean_q8);
            uint32_t x, i = 0;
            while (ylc_dec_get(&d, &x)) {
                if (!fn(user, s.start_time + i * s.interval, s.interval, s.time_valid, x))
                    return;
                i++;
            }
            off = s.end;
        }
    }
}

typedef struct {
    ylog_stats_t *s;
    bool first;
} stats_ctx;

static bool stats_visit(void *user, uint32_t time, uint32_t interval, bool valid, uint32_t value)
{
    (void)interval; (void)value;
    stats_ctx *c = user;
    /* samples taken before the clock was set carry the clock's start-up date, not a date: they must not stretch "days of
     * records" (a new device showed 381 days) or give the oldest record a date from before it existed */
    if (!valid || time < YARROW_MIN_TIME)
        return true;
    if (c->first) {
        c->s->oldest_time = time;
        c->first = false;
    }
    c->s->newest_time = time;
    return true;
}

void ylog_stats(const ylog_flash_t *fl, uint8_t *pg, ylog_stats_t *s)
{
    memset(s, 0, sizeof *s);
    s->bytes_total = fl->page_size * fl->page_count;
    for (uint32_t p = 0; p < fl->page_count; p++) {
        uint32_t seq;
        if (fl_read(fl, p * fl->page_size, pg, fl->page_size) != 0 || !page_header_ok(pg, &seq))
            continue;
        s->pages_used++;
        s->bytes_used += last_non_ff(pg, 0, fl->page_size);
    }
    stats_ctx c = {s, true};
    ylog_scan(fl, pg, stats_visit, &c);
}
