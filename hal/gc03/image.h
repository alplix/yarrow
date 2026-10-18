/* Image header and flash layout of the GC-03.
 *
 *   0x08000000  stock bootloader (24 KB, never touched)
 *   0x08006000  Yarrow image (header at +0x200)                      up to IMAGE_MAX_SIZE
 *   0x0801E000  settings store: two 4 KB sectors, 2 KB used in each
 *   0x08020000  log, 4 KB pages                                      up to LOG_END
 *   0x0803F000  last sector: factory tube calibration at 0x0803FC00, never erased or written */
#ifndef YARROW_IMAGE_H
#define YARROW_IMAGE_H

#include <stdint.h>

#define IMAGE_BASE          0x08006000u
#define IMAGE_MAX_SIZE      0x18000u    /* 96 KB: up to KV_BASE */
#define IMAGE_MAGIC         0x57524159u /* "YARW" little endian */
#define IMAGE_HEADER_OFFSET 0x200u

#define FLASH_SECTOR        0x1000u
#define KV_BASE             0x0801E000u
#define KV_SECTORS          2u
#define KV_PAGE             0x800u      /* logical page used inside each sector (2 KB: more room means fewer sector erases) */
#define LOG_BASE            0x08020000u
#define LOG_END             0x0803F000u
#define FACTORY_PAGE        0x0803F000u
#define FACTORY_SIGNATURE   0x0803FC00u
#define FACTORY_HV_FREQ     0x0803FC1Au
#define FACTORY_SIGNATURE_VALUE 0x4B6E4564u

typedef struct {
    uint32_t magic;
    uint32_t length;  /* whole image in bytes */
    uint32_t crc;     /* CRC-32 (zlib) of the image with this field as zero */
    uint32_t version;
} image_header_t;

/* volatile: the length and CRC are patched into the .bin after linking, so the compiler must not fold them */
extern const volatile image_header_t yarrow_header;

#endif
