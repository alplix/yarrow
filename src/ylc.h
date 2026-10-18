/* Yarrow log codec: lossless adaptive range coding of per-interval pulse counts.
 *
 * Each sample is coded relative to a running mean with context-modelled binary decisions, which
 * lands within a few percent of the Poisson entropy of the data. Every sample is preceded by a
 * "more follows" flag that costs almost nothing; a finished stream ends with the flag cleared, so the
 * decoder stops at exactly the last sample and can never invent extra ones.
 *
 * Streams are independent (one per flash segment) and decodable while still being written: bytes handed
 * out by the encoder are final, and the decoder stops at the last sample fully covered by the stored bytes. */
#ifndef YARROW_YLC_H
#define YARROW_YLC_H

#include <stdbool.h>
#include <stdint.h>

#define YLC_CTX  44
#define YLC_UCAP 11

typedef struct {
    uint16_t z[YLC_CTX];
    uint16_t s[YLC_CTX];
    uint16_t u[YLC_CTX][2][YLC_UCAP];
    uint16_t m[YLC_CTX][2][YLC_UCAP][2];
    uint32_t mean_q8; /* running mean of the samples, 24.8 fixed point */
} ylc_model_t;

typedef struct {
    ylc_model_t model;
    uint64_t low;
    uint32_t range;
    uint32_t cache_size;
    uint8_t cache;
    bool first;
    uint8_t *out;
    uint32_t cap;
    uint32_t len;   /* final bytes written to out */
    uint32_t count; /* samples accepted */
} ylc_enc_t;

typedef struct {
    ylc_model_t model;
    uint32_t code;
    uint32_t range;
    const uint8_t *in;
    uint32_t len;
    uint32_t pos;
    uint32_t decision_pos;
    bool ended;     /* the stream's end flag was seen */
} ylc_dec_t;

/* Worst-case bytes one sample can add, plus the end flag and flush. */
#define YLC_RESERVE 28

/* mean_q8 seeds the running mean (0 for a fresh log, the segment header value otherwise). */
void ylc_enc_init(ylc_enc_t *e, uint8_t *out, uint32_t cap, uint32_t mean_q8);

/* Adds a sample. Returns false, leaving the stream untouched, if the buffer has no room left. */
bool ylc_enc_put(ylc_enc_t *e, uint32_t x);

/* Ends the stream (end flag, flush) and returns its final length. No more samples afterwards. */
uint32_t ylc_enc_finish(ylc_enc_t *e);

/* Mean (24.8) after the samples so far, to seed the next segment. */
uint32_t ylc_enc_mean(const ylc_enc_t *e);

/* len is how many stream bytes are valid; the decoder never relies on anything beyond them. */
void ylc_dec_init(ylc_dec_t *d, const uint8_t *in, uint32_t len, uint32_t mean_q8);

/* Decodes one sample. Returns false at the end of the stream, when the stored bytes run out, or on damage. */
bool ylc_dec_get(ylc_dec_t *d, uint32_t *x);

#endif
