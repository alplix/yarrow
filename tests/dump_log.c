/* Writes a log into a RAM flash image and saves it next to the samples the device decoder returns,
 * so tools/ylog_decode.py can be checked against the C implementation. */
#include "ylog.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PSIZE 1024u
#define NPAGES 12u
static uint8_t mem[NPAGES * PSIZE];

static int rd(void *c, uint32_t a, void *b, uint32_t n) { (void)c; memcpy(b, mem + a, n); return 0; }
static int pr(void *c, uint32_t a, const void *b, uint32_t n)
{
    (void)c;
    const uint8_t *s = b;
    for (uint32_t i = 0; i < n; i++)
        mem[a + i] &= s[i];
    return 0;
}
static int er(void *c, uint32_t p) { (void)c; memset(mem + p * PSIZE, 0xFF, PSIZE); return 0; }

static uint64_t rs = 0x5DEECE66Dull;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (uint32_t)(rs >> 11); }
static double uni(void) { return ((rnd() & 0xFFFFFF) + 0.5) / 16777216.0; }
static uint32_t poisson(double lam)
{
    double L = exp(-lam), p = 1.0;
    uint32_t k = 0;
    do { k++; p *= uni(); } while (p > L);
    return k - 1;
}

static FILE *csv;
static bool visit(void *u, uint32_t t, uint32_t iv, bool valid, uint32_t v)
{
    (void)u;
    fprintf(csv, "%u,%u,%d,%u\n", (unsigned)t, (unsigned)iv, valid ? 1 : 0, (unsigned)v);
    return true;
}

static ylog_t log_obj;

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[512];
    memset(mem, 0xFF, sizeof mem);
    ylog_flash_t dev = {PSIZE, NPAGES, rd, pr, er, 0};

    uint32_t t = 1791970860u;
    for (int session = 0; session < 5; session++) {
        ylog_open(&log_obj, &dev);
        double lam = session == 2 ? 300.0 : 6.0;
        for (int i = 0; i < 2500 + session * 700; i++) {
            ylog_add(&log_obj, t, 60, true, poisson(lam));
            t += 60;
        }
        if (session != 3) /* one session ends without a clean close */
            ylog_close(&log_obj);
        t += 600;
    }

    snprintf(path, sizeof path, "%s/flash.bin", dir);
    FILE *f = fopen(path, "wb");
    fwrite(mem, 1, sizeof mem, f);
    fclose(f);

    static uint8_t scratch[PSIZE];
    snprintf(path, sizeof path, "%s/expected.csv", dir);
    csv = fopen(path, "w");
    fprintf(csv, "time,interval,time_valid,value\n");
    ylog_scan(&dev, scratch, visit, 0);
    fclose(csv);
    return 0;
}
