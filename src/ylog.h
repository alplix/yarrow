/* Power-fail-safe log store on NOR flash.
 *
 * The log region is a ring of erase pages. A page starts with a small header (sequence number) followed by
 * segments; a segment is one uninterrupted run of samples with a fixed interval, stored as one ylc stream.
 * New sessions append a new segment to the page that still has room, so switching the device on and off
 * does not burn pages. Only flash's 1 -> 0 programming is used after an erase, and everything written is
 * either valid or ignored on the next start. */
#ifndef YARROW_YLOG_H
#define YARROW_YLOG_H

#include "ylc.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef YLOG_MAX_PAGE
#define YLOG_MAX_PAGE 1024
#endif

typedef struct {
    uint32_t page_size;   /* erase unit, multiple of 2 */
    uint32_t page_count;  /* pages in the log region */
    /* addresses are byte offsets from the start of the region; prog needs even addr and size and may only clear bits */
    int (*read)(void *ctx, uint32_t addr, void *buf, uint32_t n);
    int (*prog)(void *ctx, uint32_t addr, const void *buf, uint32_t n);
    int (*erase)(void *ctx, uint32_t page);
    void *ctx;
} ylog_flash_t;

typedef struct {
    const ylog_flash_t *fl;
    bool failed;          /* a flash operation failed: logging stopped */

    uint32_t cur_page;    /* page being written */
    uint32_t cur_seq;
    bool page_open;       /* the current page still has room for another segment */
    uint32_t page_free;   /* first unused byte of the current page */

    /* open segment */
    bool seg_open;
    uint32_t seg_off;     /* header offset inside the page */
    uint32_t seg_start;   /* start time */
    uint32_t seg_interval;
    uint32_t seg_next_time;
    uint32_t seg_written; /* stream bytes already in flash */
    ylc_enc_t enc;
    uint8_t buf[YLOG_MAX_PAGE];

    uint32_t last_mean_q8;
} ylog_t;

/* Scans the region, repairs the last page and gets ready to append. Returns 0 on success. */
int ylog_open(ylog_t *l, const ylog_flash_t *fl);

/* Appends one sample taken at `time` covering `interval` seconds. Starts a new segment when the time
 * does not follow the previous sample. Returns false if logging has failed. */
bool ylog_add(ylog_t *l, uint32_t time, uint32_t interval, bool time_valid, uint32_t value);

/* Programs every stream byte that is final. Call regularly (the sooner after a sample, the less a power cut costs). */
void ylog_sync(ylog_t *l);

/* Clean stop: finishes the stream and records its length and sample count. */
void ylog_close(ylog_t *l);

/* Visits every stored sample from the oldest to the newest. Return false from fn to stop.
 * scratch must hold one page. */
typedef bool (*ylog_visit_fn)(void *user, uint32_t time, uint32_t interval, bool time_valid, uint32_t value);
void ylog_scan(const ylog_flash_t *fl, uint8_t *scratch, ylog_visit_fn fn, void *user);

typedef struct {
    uint32_t pages_used;
    uint32_t bytes_used;   /* header and stream bytes in use */
    uint32_t bytes_total;
    uint32_t oldest_time;  /* time of the first sample still stored, 0 if none */
    uint32_t newest_time;
} ylog_stats_t;

void ylog_stats(const ylog_flash_t *fl, uint8_t *scratch, ylog_stats_t *s);

#endif
