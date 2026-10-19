/* Cuts the device's cumulative pulse counter into fixed time bins. Only exact bins are produced: both edges must be
 * read in the device's own clock second of the edge, the tube voltage must have been on for the whole bin and the
 * counter must not have restarted. Anything else drops the bin; it is never estimated or padded. */
#ifndef YARROW_BIN_H
#define YARROW_BIN_H

#include <stdint.h>

typedef struct {
    uint32_t t;       /* device clock, unix seconds */
    uint32_t pulses;  /* raw pulses since the device was switched on */
    uint32_t hv_s;    /* seconds the tube voltage has been on since the device was switched on */
    double host_s;    /* host monotonic clock when the reply arrived */
} ysample_t;

typedef void (*ybin_emit_fn)(void *ctx, uint32_t start, uint32_t count);

typedef struct {
    uint32_t interval;
    int have_prev;
    ysample_t prev;
    int open;
    uint32_t bin_start;
    ysample_t first;
    ybin_emit_fn emit;
    void *ctx;
} ybinner_t;

void ybin_init(ybinner_t *b, uint32_t interval, ybin_emit_fn emit, void *ctx);
void ybin_feed(ybinner_t *b, const ysample_t *s);
void ybin_break(ybinner_t *b);   /* the device went away or its calibration changed: forget the open bin */

/* seconds of device time until the next bin edge, for the caller's polling rhythm */
uint32_t ybin_to_edge(const ybinner_t *b, uint32_t device_t);

#endif
