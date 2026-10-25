#include "yarrow_gmc.h"
#include "yarrow_serial.h"
#include "yarrow_src.h"

#include <stdio.h>
#include <string.h>

static int io_write(void *ctx, const void *p, size_t n) { return yser_write((yser_t *)ctx, p, n); }
static int io_read(void *ctx, void *buf, size_t n, int ms) { return yser_read((yser_t *)ctx, buf, n, ms); }
static void io_close(void *ctx) { yser_close((yser_t *)ctx); }
static void io_drain(void *ctx) { yser_drain((yser_t *)ctx); }

/* Asks every serial port of this computer for a GQ GMC counter. The only thing sent is <GETVER>>, which asks the version. */
static ysrc_t *ysrc_gmc_find(ydev_t *dev)
{
    static char ports[64][64];
    int n = yser_ports(ports, 64);
    char host[64] = "";
    yhost_name(host, sizeof host);
    static const int bauds[2] = {115200, 57600};          /* GMC-300 V3 and older: 57600 */
    for (int i = 0; i < n; i++) {
        for (int b = 0; b < 2; b++) {
            yser_t *s = yser_open_baud(ports[i], bauds[b]);
            if (!s) {                                     /* the port does not exist or is in use */
                static char told[16][64];
                static int n_told;
                int known = 0;
                for (int k = 0; k < n_told; k++)
                    if (strcmp(told[k], ports[i]) == 0)
                        known = 1;
                if (!known && n_told < 16) {
                    snprintf(told[n_told++], sizeof told[0], "%s", ports[i]);
                    fprintf(stderr, "ygmc: %s could not be opened (another program may be using it, or it is not for this user)\n", ports[i]);
                }
                break;
            }
            ygmc_io_t io = {s, io_write, io_read, io_close};
            ygmc_env_t env = {yhost_wall, io_drain, host};
            ysrc_t *src = ygmc_make(&io, &env, dev);
            if (src)
                return src;
            yser_close(s);
        }
    }
    return NULL;
}

/* The Yarrow detector first (it answers on its own serial port and knows its identity); then the counters of other makers. A
 * volunteer who named a serial port asks for a Yarrow detector on exactly that port. */
ysrc_t *ysrc_find(const char *port, ydev_t *dev)
{
    ysrc_t *s = ysrc_yarrow_find(port, dev);
    if (s || (port && port[0]))
        return s;
    s = ysrc_rah_find(dev);
    if (s)
        return s;
    return ysrc_gmc_find(dev);
}
