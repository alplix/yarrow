/* USB HID access for counters that talk through a feature report (Radioactive@Home). Linux uses hidraw, Windows hid.dll;
 * other systems find nothing. Tests replace the backend with yhid_ops_t. */
#ifndef YARROW_HID_H
#define YARROW_HID_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    /* Reads feature report 0 into buf (buf[0] is the report id, the data follows); returns the bytes read or -1. */
    int (*get_feature)(void *ctx, uint8_t *buf, size_t len);
    void (*close)(void *ctx);
    char name[96];           /* USB product string, empty when unknown */
    char serial[64];         /* USB serial string, empty when it has none */
} yhid_t;

/* Opens the first HID device with this vendor and product id whose product string contains name_part. 0 on success. */
int yhid_open(uint16_t vid, uint16_t pid, const char *name_part, yhid_t *out);

/* Wall clock in seconds since 1970 with sub-second precision, and the name of this computer. */
double yhost_wall(void);
void yhost_name(char *out, size_t cap);

#endif
