/* Collects finished bins and writes them as a 'yarrow' trickle (format: docs/boinc-monitor.md). */
#ifndef YARROW_TRICKLE_H
#define YARROW_TRICKLE_H

#include <stddef.h>
#include <stdint.h>

#define YT_MAX_PENDING 1008
#define YT_MAX_SPAN 144          /* bins in one trickle: a day of 10-minute bins */

typedef struct {
    char uid[17];                /* 16 hex digits, hash of the chip id */
    char fw[17];
    uint32_t sens_x100;
    uint32_t dead_us;
    char cc[3];
    char city[41];
    uint32_t interval;
} ydev_t;

typedef struct {
    uint32_t n;
    uint32_t start[YT_MAX_PENDING];
    uint32_t count[YT_MAX_PENDING];
} ypending_t;

/* bins must arrive in time order; returns 0 on success, -1 when the list is full (the oldest bin is dropped) */
int ypending_add(ypending_t *p, uint32_t start, uint32_t count);

/* Writes the first chunk of pending bins as trickle text. Returns how many pending bins it holds (to remove after
 * a successful send), or 0 when there is nothing to send or the buffer is too small. */
uint32_t ypending_xml(const ypending_t *p, const ydev_t *d, char *out, size_t cap);
void ypending_drop(ypending_t *p, uint32_t k);

/* Checkpoint text: one "start count" per line. */
size_t ypending_save(const ypending_t *p, char *out, size_t cap);
void ypending_load(ypending_t *p, const char *text);

/* Settings from the project preferences are checked here: 2 capital letters and a plain city name. */
int ydev_valid_place(const char *cc, const char *city);

#endif
