/* Bins from the device's own log, for the time the monitor was not running (BOINC closed, cable out) while the detector kept
 * counting. A bin is only made when the log covers all of it without a hole, with samples that carry a real date, and
 * when a stretch of zeros does not suggest the tube voltage was off: it is never estimated or padded. */
#ifndef YARROW_BACKFILL_H
#define YARROW_BACKFILL_H

#include "yarrow_bin.h"
#include "ylog.h"

/* Bins of `interval` seconds that start at or after `from` and end at or before `until`, in time order, from the log in `fl`
 * (scratch holds one page). Returns how many bins were emitted. */
uint32_t ybf_bins(const ylog_flash_t *fl, uint8_t *scratch, uint32_t interval, uint32_t from, uint32_t until, ybin_emit_fn emit,
                  void *ctx);

/* Decodes the base64 lines of a LOGPAGE reply (after the "OK page size" line, up to END) into out; returns the byte count or -1. */
int ybf_page_from_reply(const char *reply, uint8_t *out, uint32_t size);

#endif
