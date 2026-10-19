/* A source of raw Geiger counts: one supported kind of counter. The monitor asks each kind in turn whether it finds a counter on
 * this computer, then polls the first that answers. Every source delivers the same thing: the counter's cumulative raw pulses with
 * a time, the identity of the counter and the sensitivity (counts per minute for 1 uSv/h, times 100) and dead time of its tube. */
#ifndef YARROW_SRC_H
#define YARROW_SRC_H

#include "yarrow_bin.h"
#include "yarrow_hid.h"
#include "yarrow_trickle.h"

typedef struct ysrc ysrc_t;

/* What housekeeping found: the source set the device's clock (the bin in progress is then not exact), or the calibration changed
 * (bins already counted keep the old one, the bin in progress is dropped). */
typedef struct {
    int clock_set;
    int cal_changed;
    uint32_t sens_x100, dead_us;
} yhk_t;

struct ysrc {
    const char *name;
    /* One reading; 0 on success. A source with a wall clock of its own (the Yarrow detector) reports its seconds, one without
     * one (every other counter) reports the host's, and marks every reading that is not taken at a bin edge as inexact. */
    int (*poll)(ysrc_t *s, uint32_t interval, ysample_t *out);
    /* Called about once a minute: lets the source keep its own clock and calibration right. May be NULL. */
    void (*housekeeping)(ysrc_t *s, const ydev_t *dev, const ysample_t *last, yhk_t *out);
    /* Seconds to wait before the next poll, given the seconds until the next bin edge. */
    int (*next_wait_ms)(const ysrc_t *s, uint32_t to_edge, uint32_t interval);
    void (*close)(ysrc_t *s);
};

/* Tries every kind of source; on success returns it and fills dev (uid, fw, sens_x100, dead_us). port: a serial port the
 * volunteer named, or empty. NULL when nothing is found. */
ysrc_t *ysrc_find(const char *port, ydev_t *dev);

/* sources */
ysrc_t *ysrc_yarrow_find(const char *port, ydev_t *dev);     /* Yarrow firmware on a USB serial port (yarrow_src_yarrow.c) */
ysrc_t *ysrc_rah_find(ydev_t *dev);                          /* Radioactive@Home USB counter (yarrow_src_rah.c) */


/* The Radioactive@Home source on an already opened HID device; env carries the clock and sleep so that tests can replace them. */
typedef struct {
    double (*now)(void);             /* wall clock, seconds since 1970 */
    void (*sleep_ms)(int ms);
    const char *host;                /* computer name, part of the identity */
} yrah_env_t;
ysrc_t *yrah_make(const yhid_t *hid, const yrah_env_t *env, ydev_t *dev);   /* NULL when the device is not such a counter */

#endif
