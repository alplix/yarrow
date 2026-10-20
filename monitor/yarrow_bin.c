#include "yarrow_bin.h"

#define MIN_CLOCK 1700000000u   /* a device clock that was never set reads far below this */
#define HOST_SLACK_S 3.0        /* device and host clocks may disagree by this much between two polls */

void ybin_init(ybinner_t *b, uint32_t interval, ybin_emit_fn emit, void *ctx)
{
    b->interval = interval;
    b->have_prev = 0;
    b->open = 0;
    b->emit = emit;
    b->ctx = ctx;
}

void ybin_break(ybinner_t *b)
{
    b->have_prev = 0;
    b->open = 0;
}

static int discontinuity(const ysample_t *p, const ysample_t *s)
{
    if (s->t < p->t || s->hv_s < p->hv_s || s->pulses < p->pulses)
        return 1;                                   /* clock set back, device restarted */
    double dt = (double)(s->t - p->t), dh = s->host_s - p->host_s;
    return dt - dh > HOST_SLACK_S || dh - dt > HOST_SLACK_S;   /* clock set forward, or the host stalled */
}

void ybin_feed(ybinner_t *b, const ysample_t *s)
{
    if (s->t < MIN_CLOCK) {
        ybin_break(b);
        return;
    }
    if (b->have_prev && discontinuity(&b->prev, s))
        b->open = 0;
    b->prev = *s;
    b->have_prev = 1;

    if (b->open && s->t >= b->bin_start + b->interval) {
        uint32_t end = b->bin_start + b->interval;
        uint32_t span = s->t - b->first.t, hv = s->hv_s - b->first.hv_s;
        /* edge samples must both be in the edge second; the voltage counter ticks on its own phase, so it may differ by 1,
         * and a real device timer is a cheap oscillator (the R@H counter runs 0.13 % slow), so allow 0.5 % of the span too */
        uint32_t slack = 1 + span / 200;
        if (s->t == end && !s->inexact && b->first.t == b->bin_start && span == b->interval && hv + slack >= span && hv <= span + slack)
            b->emit(b->ctx, b->bin_start, s->pulses - b->first.pulses);
        b->open = 0;
    }
    if (!b->open) {
        uint32_t edge = s->t - s->t % b->interval;
        if (s->t == edge && !s->inexact) {
            b->open = 1;
            b->bin_start = edge;
            b->first = *s;
        }
    }
}

uint32_t ybin_to_edge(const ybinner_t *b, uint32_t device_t)
{
    return b->interval - device_t % b->interval;
}
