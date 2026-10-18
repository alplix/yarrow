#include "fmt.h"
#include "strings.h"

int fmt_u(char *out, uint32_t v)
{
    char tmp[11];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    int len = n;
    for (int i = 0; i < len; i++)
        out[i] = tmp[len - 1 - i];
    out[len] = 0;
    return len;
}

static int put2(char *out, uint32_t v)
{
    out[0] = (char)('0' + (v / 10u) % 10u);
    out[1] = (char)('0' + v % 10u);
    return 2;
}

int fmt_fixed2(char *out, uint32_t v)
{
    int n = fmt_u(out, v / 100u);
    out[n++] = '.';
    n += put2(out + n, v % 100u);
    out[n] = 0;
    return n;
}

int fmt_fixed1(char *out, uint32_t v)
{
    int n = fmt_u(out, v / 10u);
    out[n++] = '.';
    out[n++] = (char)('0' + v % 10u);
    out[n] = 0;
    return n;
}

int fmt_dose_rate(char *out, uint32_t nsvh, dose_unit_t *unit)
{
    if (nsvh < 100000u) { /* below 100 uSv/h: hundredths of a uSv/h */
        *unit = UNIT_USVH;
        return fmt_fixed2(out, (nsvh + 5u) / 10u);
    }
    if (nsvh < 1000000u) { /* below 1 mSv/h: tenths of a uSv/h */
        *unit = UNIT_USVH;
        return fmt_fixed1(out, (nsvh + 50u) / 100u);
    }
    *unit = UNIT_MSVH;
    return fmt_fixed2(out, (uint32_t)(((uint64_t)nsvh + 5000u) / 10000u));
}

int fmt_dose_total(char *out, uint64_t nsv, dose_total_unit_t *unit)
{
    if (nsv < 1000u) {
        *unit = DOSE_NSV;
        return fmt_u(out, (uint32_t)nsv);
    }
    if (nsv < 1000000u) { /* below 1 mSv */
        *unit = DOSE_USV;
        uint32_t usv_x10 = (uint32_t)((nsv + 50u) / 100u);
        return usv_x10 < 1000u ? fmt_fixed1(out, usv_x10) : fmt_u(out, usv_x10 / 10u);
    }
    *unit = DOSE_MSV;
    return fmt_fixed2(out, (uint32_t)((nsv + 5000u) / 10000u));
}

static int put_str(char *out, int n, const char *s)
{
    while (*s)
        out[n++] = *s++;
    out[n] = 0;
    return n;
}

int fmt_duration(char *out, uint32_t s)
{
    uint32_t m = s / 60u;
    int n;
    if (m >= 60u * 48u) /* two days or more: whole hours */
        return put_str(out, fmt_u(out, m / 60u), L_HOUR_UNIT);
    if (m >= 60u) {
        n = put_str(out, fmt_u(out, m / 60u), L_HOUR_UNIT);
        if (m % 60u) {
            out[n++] = ' ';
            n += fmt_u(out + n, m % 60u);
            n = put_str(out, n, L_MIN_UNIT);
        }
        return n;
    }
    return put_str(out, fmt_u(out, m), L_MIN_UNIT);
}

int fmt_clock(char *out, uint32_t unix_s, int32_t offset_s)
{
    uint32_t t = (uint32_t)((int64_t)unix_s + offset_s);
    uint32_t sod = t % 86400u;
    put2(out, sod / 3600u);
    out[2] = ':';
    put2(out + 3, (sod / 60u) % 60u);
    out[5] = 0;
    return 5;
}

/* days since 1970-01-01 to civil date (Howard Hinnant's algorithm) */
static void civil(uint32_t days, uint32_t *y, uint32_t *m, uint32_t *d)
{
    int64_t z = (int64_t)days + 719468;
    int64_t era = z / 146097;
    int64_t doe = z - era * 146097;
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t yy = yoe + era * 400;
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int64_t mp = (5 * doy + 2) / 153;
    *d = (uint32_t)(doy - (153 * mp + 2) / 5 + 1);
    *m = (uint32_t)(mp < 10 ? mp + 3 : mp - 9);
    *y = (uint32_t)(yy + (*m <= 2));
}

static const char *const months[12] = L_MONTHS;

int fmt_date(char *out, uint32_t unix_s, int32_t offset_s)
{
    uint32_t t = (uint32_t)((int64_t)unix_s + offset_s);
    uint32_t y, m, d;
    civil(t / 86400u, &y, &m, &d);
    int n = 0;
#if L_DATE_MON_FIRST
    n = put_str(out, n, months[m - 1]);
    out[n++] = ' ';
    n += fmt_u(out + n, d);
    n = put_str(out, n, L_DAY_SUFFIX);
#else
    n = fmt_u(out, d);
    out[n++] = ' ';
    n = put_str(out, n, months[m - 1]);
#endif
    return n;
}

int fmt_date_time(char *out, uint32_t unix_s, int32_t offset_s)
{
    int n = fmt_date(out, unix_s, offset_s);
    out[n++] = ' ';
    n += fmt_clock(out + n, unix_s, offset_s);
    return n;
}

uint32_t days_in_month(uint32_t y, uint32_t m)
{
    static const uint8_t dm[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        return 29;
    return dm[(m - 1) % 12];
}

uint32_t time_from_civil(uint32_t y, uint32_t m, uint32_t d, uint32_t hh, uint32_t mm)
{
    /* days since 1970-01-01 (Howard Hinnant's algorithm) */
    int64_t yy = (int64_t)y - (m <= 2);
    int64_t era = (yy >= 0 ? yy : yy - 399) / 400;
    int64_t yoe = yy - era * 400;
    int64_t doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + doe - 719468;
    return (uint32_t)(days * 86400 + (int64_t)hh * 3600 + (int64_t)mm * 60);
}

void time_to_civil(uint32_t unix_s, uint32_t *y, uint32_t *m, uint32_t *d, uint32_t *hh, uint32_t *mm)
{
    civil(unix_s / 86400u, y, m, d);
    *hh = (unix_s % 86400u) / 3600u;
    *mm = (unix_s % 3600u) / 60u;
}
