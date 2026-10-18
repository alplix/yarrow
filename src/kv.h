/* Small power-fail-safe key/value store for settings and counters.
 *
 * Two flash pages used as A/B: records are appended, the newest record of a key wins, and when a page
 * fills up the live records are copied to the other page. The new page only becomes valid when its
 * header is written last, so a power cut at any moment leaves either the old or the new page intact. */
#ifndef YARROW_KV_H
#define YARROW_KV_H

#include "ylog.h" /* ylog_flash_t describes the flash window */
#include <stdbool.h>
#include <stdint.h>

#define KV_MAX_VALUE 32
#define KV_MAX_PAGE 2048 /* logical page size of the store */

typedef struct {
    const ylog_flash_t *fl; /* exactly two pages */
    uint32_t active;        /* 0 or 1 */
    uint32_t seq;
    uint32_t free_off;      /* next record offset in the active page */
    bool failed;
    uint8_t page[KV_MAX_PAGE]; /* working copy of the active page */
} kv_t;

/* Opens the store, repairing and compacting if the last run was cut short. Returns 0 on success. */
int kv_open(kv_t *kv, const ylog_flash_t *fl);

/* Copies the value of `key` into buf (up to max bytes). Returns its length, or -1 if the key is unset. */
int kv_get(const kv_t *kv, uint8_t key, void *buf, uint32_t max);

/* Stores a value (len <= KV_MAX_VALUE). Returns 0 on success. */
int kv_set(kv_t *kv, uint8_t key, const void *data, uint32_t len);

/* Helpers for 32-bit values. get returns false if unset. */
bool kv_get_u32(const kv_t *kv, uint8_t key, uint32_t *v);
int kv_set_u32(kv_t *kv, uint8_t key, uint32_t v);

#endif
