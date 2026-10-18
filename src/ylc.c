#include "ylc.h"
#include <stddef.h>

#define PROB_BITS 12
#define PROB_INIT (1u << (PROB_BITS - 1))
#define PROB_MIN  31u
#define PROB_MAX  ((1u << PROB_BITS) - 31u)
#define ADAPT     5
#define MEAN_CAP  ((1u << 22) - 1u)
#define TOP       (1u << 24)

static unsigned ilog2(uint32_t v)
{
    unsigned l = 0;
    while (v >>= 1)
        l++;
    return l;
}

static void model_init(ylc_model_t *m, uint32_t mean_q8)
{
    uint16_t *p = (uint16_t *)m;
    for (size_t i = 0; i < offsetof(ylc_model_t, mean_q8) / sizeof(uint16_t); i++)
        p[i] = PROB_INIT;
    m->mean_q8 = mean_q8;
}

/* Context from the running mean: half octaves up to a mean of 64, whole octaves above. */
static unsigned ctx_of(uint32_t mean_q8)
{
    uint32_t v = mean_q8 + 1;
    unsigned l = ilog2(v);
    if (l < 15)
        return 2 * l + (l >= 1 ? (v >> (l - 1)) & 1u : 0u);
    unsigned c = 30 + (l - 15);
    return c < YLC_CTX ? c : YLC_CTX - 1;
}

static void model_update_mean(ylc_model_t *m, uint32_t x)
{
    int32_t target = (int32_t)((x > MEAN_CAP ? MEAN_CAP : x) << 8);
    int32_t mean = (int32_t)m->mean_q8;
    m->mean_q8 = (uint32_t)(mean + ((target - mean) >> ADAPT));
}

static uint32_t pred_of(const ylc_model_t *m) { return (m->mean_q8 + 128) >> 8; }

static void adapt(uint16_t *p, unsigned bit)
{
    unsigned v = *p;
    if (bit == 0)
        v += ((1u << PROB_BITS) - v) >> ADAPT;
    else
        v -= v >> ADAPT;
    *p = (uint16_t)(v < PROB_MIN ? PROB_MIN : v > PROB_MAX ? PROB_MAX : v);
}

/* ---- encoder ---- */

static void enc_byte(ylc_enc_t *e, uint8_t b)
{
    if (e->first) { /* the first byte of this coder is always zero; leave it out */
        e->first = false;
        return;
    }
    e->out[e->len++] = b;
}

static void shift_low(ylc_enc_t *e)
{
    if ((uint32_t)e->low < 0xFF000000u || (e->low >> 32) != 0) {
        uint8_t carry = (uint8_t)(e->low >> 32);
        uint8_t temp = e->cache;
        do {
            enc_byte(e, (uint8_t)(temp + carry));
            temp = 0xFF;
        } while (--e->cache_size != 0);
        e->cache = (uint8_t)(e->low >> 24);
    }
    e->cache_size++;
    e->low = (e->low & 0x00FFFFFFu) << 8;
}

static void enc_normalize(ylc_enc_t *e)
{
    while (e->range < TOP) {
        e->range <<= 8;
        shift_low(e);
    }
}

static void enc_bit(ylc_enc_t *e, uint16_t *p, unsigned bit)
{
    uint32_t bound = (e->range >> PROB_BITS) * *p;
    if (bit == 0) {
        e->range = bound;
    } else {
        e->low += bound;
        e->range -= bound;
    }
    adapt(p, bit);
    enc_normalize(e);
}

static void enc_raw(ylc_enc_t *e, unsigned bit)
{
    e->range >>= 1;
    if (bit)
        e->low += e->range;
    enc_normalize(e);
}

/* "More follows" flag with a fixed probability of 65535/65536: costs next to nothing per sample. */
static void enc_flag(ylc_enc_t *e, unsigned more)
{
    uint32_t bound = (e->range >> 16) * 65535u;
    if (more) {
        e->range = bound;
    } else {
        e->low += bound;
        e->range -= bound;
    }
    enc_normalize(e);
}

void ylc_enc_init(ylc_enc_t *e, uint8_t *out, uint32_t cap, uint32_t mean_q8)
{
    model_init(&e->model, mean_q8);
    e->low = 0;
    e->range = 0xFFFFFFFFu;
    e->cache = 0;
    e->cache_size = 1;
    e->first = true;
    e->out = out;
    e->cap = cap;
    e->len = 0;
    e->count = 0;
}

bool ylc_enc_put(ylc_enc_t *e, uint32_t x)
{
    if (e->len + e->cache_size + YLC_RESERVE > e->cap)
        return false;

    ylc_model_t *m = &e->model;
    unsigned c = ctx_of(m->mean_q8);
    uint32_t pred = pred_of(m);

    enc_flag(e, 1);
    int64_t d = (int64_t)x - (int64_t)pred;
    enc_bit(e, &m->z[c], d != 0);
    if (d != 0) {
        unsigned neg = 0;
        if (pred > 0) {
            neg = d < 0;
            enc_bit(e, &m->s[c], neg);
        }
        uint32_t n = (uint32_t)(d < 0 ? -d : d); /* |d| < 2^32 because x and pred are 32 bit */
        unsigned nb = ilog2(n);
        for (unsigned i = 0; i < nb; i++)
            enc_bit(e, &m->u[c][neg][i < YLC_UCAP ? i : YLC_UCAP - 1], 1);
        enc_bit(e, &m->u[c][neg][nb < YLC_UCAP ? nb : YLC_UCAP - 1], 0);
        unsigned nbc = nb < YLC_UCAP ? nb : YLC_UCAP - 1;
        for (unsigned j = 0; j < nb; j++) {
            unsigned bit = (unsigned)((n >> (nb - 1 - j)) & 1u);
            if (j < 2)
                enc_bit(e, &m->m[c][neg][nbc][j], bit);
            else
                enc_raw(e, bit);
        }
    }
    model_update_mean(m, x);
    e->count++;
    return true;
}

uint32_t ylc_enc_finish(ylc_enc_t *e)
{
    enc_flag(e, 0);
    for (int i = 0; i < 5; i++)
        shift_low(e);
    return e->len;
}

uint32_t ylc_enc_mean(const ylc_enc_t *e) { return e->model.mean_q8; }

/* ---- decoder ---- */

static uint8_t dec_byte(ylc_dec_t *d)
{
    uint8_t b = d->pos < d->len ? d->in[d->pos] : 0xFF; /* erased flash reads as 0xFF */
    d->pos++;
    return b;
}

static void dec_normalize(ylc_dec_t *d)
{
    while (d->range < TOP) {
        d->range <<= 8;
        d->code = (d->code << 8) | dec_byte(d);
    }
}

static unsigned dec_bit(ylc_dec_t *d, uint16_t *p)
{
    uint32_t bound = (d->range >> PROB_BITS) * *p;
    unsigned bit;
    d->decision_pos = d->pos;
    if (d->code < bound) {
        d->range = bound;
        bit = 0;
    } else {
        d->code -= bound;
        d->range -= bound;
        bit = 1;
    }
    adapt(p, bit);
    dec_normalize(d);
    return bit;
}

static unsigned dec_raw(ylc_dec_t *d)
{
    unsigned bit;
    d->decision_pos = d->pos;
    d->range >>= 1;
    if (d->code >= d->range) {
        d->code -= d->range;
        bit = 1;
    } else {
        bit = 0;
    }
    dec_normalize(d);
    return bit;
}

void ylc_dec_init(ylc_dec_t *d, const uint8_t *in, uint32_t len, uint32_t mean_q8)
{
    model_init(&d->model, mean_q8);
    d->in = in;
    d->len = len;
    d->pos = 0;
    d->decision_pos = 0;
    d->ended = false;
    d->range = 0xFFFFFFFFu;
    d->code = 0;
    for (int i = 0; i < 4; i++)
        d->code = (d->code << 8) | dec_byte(d);
}

static unsigned dec_flag(ylc_dec_t *d)
{
    uint32_t bound = (d->range >> 16) * 65535u;
    unsigned more;
    d->decision_pos = d->pos;
    if (d->code < bound) {
        d->range = bound;
        more = 1;
    } else {
        d->code -= bound;
        d->range -= bound;
        more = 0;
    }
    dec_normalize(d);
    return more;
}

bool ylc_dec_get(ylc_dec_t *d, uint32_t *x)
{
    if (d->ended)
        return false;
    unsigned more = dec_flag(d);
    if (d->decision_pos > d->len)
        return false;
    if (!more) {
        d->ended = true;
        return false;
    }

    ylc_model_t *m = &d->model;
    unsigned c = ctx_of(m->mean_q8);
    uint32_t pred = pred_of(m);
    int64_t v = pred;

    if (dec_bit(d, &m->z[c])) {
        unsigned neg = 0;
        if (pred > 0)
            neg = dec_bit(d, &m->s[c]);
        unsigned nb = 0;
        while (dec_bit(d, &m->u[c][neg][nb < YLC_UCAP ? nb : YLC_UCAP - 1])) {
            if (++nb > 32)
                return false; /* damaged stream */
        }
        unsigned nbc = nb < YLC_UCAP ? nb : YLC_UCAP - 1;
        uint64_t n = 1;
        for (unsigned j = 0; j < nb; j++) {
            unsigned bit = j < 2 ? dec_bit(d, &m->m[c][neg][nbc][j]) : dec_raw(d);
            n = (n << 1) | bit;
        }
        /* n now carries the leading one, so it equals the magnitude */
        v = neg ? v - (int64_t)n : v + (int64_t)n;
    }

    if (d->decision_pos > d->len)
        return false;
    if (v < 0 || v > 0xFFFFFFFFLL)
        return false;

    *x = (uint32_t)v;
    model_update_mean(m, *x);
    return true;
}
