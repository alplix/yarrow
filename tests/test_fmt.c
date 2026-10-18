#include "fmt.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define EXPECT(got, want) do { if (strcmp((got), (want)) != 0) { failures++; printf("FAIL line %d: got \"%s\" want \"%s\"\n", __LINE__, (got), (want)); } } while (0)

int main(void)
{
    char b[32];
    dose_unit_t u;
    dose_total_unit_t t;

    fmt_u(b, 0); EXPECT(b, "0");
    fmt_u(b, 4294967295u); EXPECT(b, "4294967295");
    fmt_fixed2(b, 5); EXPECT(b, "0.05");
    fmt_fixed2(b, 1234); EXPECT(b, "12.34");
    fmt_fixed1(b, 1234); EXPECT(b, "123.4");

    fmt_dose_rate(b, 0, &u); EXPECT(b, "0.00");
    fmt_dose_rate(b, 120, &u); EXPECT(b, "0.12"); if (u != UNIT_USVH) { failures++; puts("FAIL unit"); }
    fmt_dose_rate(b, 42800, &u); EXPECT(b, "42.80");
    fmt_dose_rate(b, 150000, &u); EXPECT(b, "150.0"); if (u != UNIT_USVH) { failures++; puts("FAIL unit 2"); }
    fmt_dose_rate(b, 2500000, &u); EXPECT(b, "2.50"); if (u != UNIT_MSVH) { failures++; puts("FAIL unit 3"); }

    fmt_dose_total(b, 420, &t); EXPECT(b, "420"); if (t != DOSE_NSV) { failures++; puts("FAIL dunit"); }
    fmt_dose_total(b, 2900, &t); EXPECT(b, "2.9"); if (t != DOSE_USV) { failures++; puts("FAIL dunit 2"); }
    fmt_dose_total(b, 412000, &t); EXPECT(b, "412"); if (t != DOSE_USV) { failures++; puts("FAIL dunit 3"); }
    fmt_dose_total(b, 1200000, &t); EXPECT(b, "1.20"); if (t != DOSE_MSV) { failures++; puts("FAIL dunit 4"); }

    fmt_duration(b, 720); EXPECT(b, "12 dk");
    fmt_duration(b, 4800); EXPECT(b, "1 sa 20 dk");
    fmt_duration(b, 41 * 3600); EXPECT(b, "41 sa");
    fmt_duration(b, 100 * 3600); EXPECT(b, "100 sa");

    /* 2026-10-14 09:41:00 UTC is 1791970860 */
    fmt_clock(b, 1791970860u, 0); EXPECT(b, "09:41");
    fmt_clock(b, 1791970860u, 3 * 3600); EXPECT(b, "12:41");
    fmt_date(b, 1791970860u, 0); EXPECT(b, "14 Eki");
    fmt_date_time(b, 1791970860u, 3 * 3600); EXPECT(b, "14 Eki 12:41");
    fmt_date(b, 0, 0); EXPECT(b, "1 Oca");
    fmt_date(b, 951782400u, 0); EXPECT(b, "29 Şub"); /* 2000-02-29 */

    {
        uint32_t y, m, d, hh, mm;
        if (time_from_civil(2026, 10, 14, 9, 41) != 1791970860u) { failures++; puts("FAIL time_from_civil"); }
        time_to_civil(1791970860u, &y, &m, &d, &hh, &mm);
        if (y != 2026 || m != 10 || d != 14 || hh != 9 || mm != 41) { failures++; puts("FAIL time_to_civil"); }
        if (time_from_civil(2000, 2, 29, 0, 0) != 951782400u) { failures++; puts("FAIL leap day"); }
        if (days_in_month(2024, 2) != 29 || days_in_month(2100, 2) != 28 || days_in_month(2026, 4) != 30) { failures++; puts("FAIL days_in_month"); }
        for (uint32_t t = 1700000000u; t < 1900000000u; t += 86399u * 7u + 61u) { /* round trip */
            time_to_civil(t, &y, &m, &d, &hh, &mm);
            if (time_from_civil(y, m, d, hh, mm) != t - t % 60u) { failures++; puts("FAIL round trip"); break; }
        }
    }
    printf(failures ? "\n%d check(s) FAILED\n" : "all fmt checks passed\n", failures);
    return failures ? 1 : 0;
}
