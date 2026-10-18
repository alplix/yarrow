/* Tiny number formatting without a C library. All functions write a NUL terminated string and return its length. */
#ifndef YARROW_FMT_H
#define YARROW_FMT_H

#include <stdint.h>

/* 1234 -> "1234" */
int fmt_u(char *out, uint32_t v);

/* value in hundredths: 1234 -> "12.34", 5 -> "0.05" */
int fmt_fixed2(char *out, uint32_t v_x100);

/* value in tenths: 1234 -> "123.4" */
int fmt_fixed1(char *out, uint32_t v_x10);

/* Dose rate in nSv/h as digits only, plus the unit to show next to it: "0.12" with "uSv/h", "1.23" with "mSv/h". */
typedef enum { UNIT_NSVH, UNIT_USVH, UNIT_MSVH } dose_unit_t;
int fmt_dose_rate(char *out, uint32_t nsvh, dose_unit_t *unit);

/* Dose in nSv: "412" with "uSv", "1.20" with "mSv". */
typedef enum { DOSE_NSV, DOSE_USV, DOSE_MSV } dose_total_unit_t;
int fmt_dose_total(char *out, uint64_t nsv, dose_total_unit_t *unit);

/* seconds -> "1 sa 20 dk", "41 sa", "12 dk" */
int fmt_duration(char *out, uint32_t seconds);

/* unix time -> "14:32" (UTC plus offset_s) */
int fmt_clock(char *out, uint32_t unix_s, int32_t offset_s);

/* unix time -> "9 Eki" and "9 Eki 14:32" (Turkish month names) */
int fmt_date(char *out, uint32_t unix_s, int32_t offset_s);
int fmt_date_time(char *out, uint32_t unix_s, int32_t offset_s);

/* calendar helpers (proleptic Gregorian, UTC) */
uint32_t time_from_civil(uint32_t y, uint32_t m, uint32_t d, uint32_t hh, uint32_t mm);
void time_to_civil(uint32_t unix_s, uint32_t *y, uint32_t *m, uint32_t *d, uint32_t *hh, uint32_t *mm);
uint32_t days_in_month(uint32_t y, uint32_t m);

#endif
