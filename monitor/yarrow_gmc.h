/* GQ GMC source: the transport and the clock are injected so that tests can replace them (see yarrow_src_gmc.c). */
#ifndef YARROW_GMC_H
#define YARROW_GMC_H

#include "yarrow_src.h"

typedef struct {
    void *ctx;
    int (*write)(void *ctx, const void *p, size_t n);                  /* 0 on success */
    int (*read)(void *ctx, void *buf, size_t n, int timeout_ms);       /* bytes read */
    void (*close)(void *ctx);
} ygmc_io_t;

typedef struct {
    double (*now)(void);                 /* wall clock, seconds since 1970 */
    void (*drain)(void *ctx);            /* throw away what is waiting on the port; may be NULL */
    const char *host;
} ygmc_env_t;

ysrc_t *ygmc_make(const ygmc_io_t *io, const ygmc_env_t *env, ydev_t *dev);    /* NULL when it is not such a counter */
uint32_t ygmc_sensitivity(const uint8_t *cfg, size_t n);

#endif
