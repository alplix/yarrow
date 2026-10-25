/* Start-up code. Everything marked BOOT sits in the first 1 KB after the vector table so that
 * even a partly written image can still run the integrity check and switch the device off. */

#include "image.h"
#include "regs.h"
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _stack_top;
extern volatile uint32_t _retained[8];
extern int main(void);

#define BOOT __attribute__((section(".boot")))

__attribute__((section(".header"), used))
const volatile image_header_t yarrow_header = {IMAGE_MAGIC, 0, 0, 1};

/* Releases the power latch (PA10 low). Safe to call from any state. */
BOOT __attribute__((noreturn)) static void power_off_now(void)
{
    RCC_APB2ENR |= 1u << 2;                                   /* GPIOA clock */
    GPIO_CRH(GPIOA) = (GPIO_CRH(GPIOA) & ~(0xFu << 8)) | (0x3u << 8); /* PA10 push-pull output */
    GPIO_BSRR(GPIOA) = 1u << (10 + 16);
    for (;;) {}
}

/* Any exception that is not expected ends here. It writes where it happened into the 32 retained bytes at the top of RAM
 * (words: 0 magic, 1 crashes in a row, 2 last reason, 3 fault address, 4 fault status, 5 faults, 6 watchdog resets, 7 check) and
 * restarts the chip; the next start (hal_gc03.c, hal_gc03_boot_record) switches the device off only after three crashes in a
 * row. Before, every fault switched the device off at once, silently: an unattended counter just went dark. */
#define RET_MAGIC 0x59525731u
BOOT __attribute__((used, noreturn)) void yarrow_fault_c(const uint32_t *frame)
{
    volatile uint32_t *r = _retained;
    if (r[0] != RET_MAGIC) {
        for (int i = 1; i < 7; i++)
            r[i] = 0;
        r[0] = RET_MAGIC;
    }
    r[2] = 7;                                      /* BOOT_FAULT */
    r[3] = frame[6];                               /* the stacked program counter */
    r[4] = *(volatile uint32_t *)0xE000ED28u;      /* CFSR */
    r[5]++;
    uint32_t c = 0xA5A5A5A5u;
    for (int i = 0; i < 7; i++)
        c ^= r[i];
    r[7] = c;
    *(volatile uint32_t *)0xE000ED0Cu = 0x05FA0004u; /* SYSRESETREQ */
    for (;;) {}
}

BOOT __attribute__((naked, noreturn)) static void fault_handler(void)
{
    __asm__ volatile("tst lr, #4\n\t"
                     "ite eq\n\t"
                     "mrseq r0, msp\n\t"
                     "mrsne r0, psp\n\t"
                     "b yarrow_fault_c\n\t");
}

BOOT static int image_ok(void)
{
    const volatile image_header_t *h = &yarrow_header;
    if (h->magic != IMAGE_MAGIC || h->length < IMAGE_HEADER_OFFSET + sizeof *h || h->length > IMAGE_MAX_SIZE)
        return 0;

    const uint8_t *p = (const uint8_t *)IMAGE_BASE;
    uint32_t crc_at = IMAGE_HEADER_OFFSET + 8;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < h->length; i++) {
        uint32_t b = (i >= crc_at && i < crc_at + 4) ? 0u : p[i];
        crc ^= b;
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc == h->crc;
}

BOOT void reset_handler(void);
extern void systick_handler(void); /* hal_gc03.c */
extern void exti3_handler(void);
extern void usb_isr(void);

__attribute__((section(".vectors"), used))
void (*const vector_table[])(void) = {
    (void (*)(void))&_stack_top,
    reset_handler,
    fault_handler, /* NMI */
    fault_handler, /* HardFault */
    fault_handler, /* MemManage */
    fault_handler, /* BusFault */
    fault_handler, /* UsageFault */
    0, 0, 0, 0,
    fault_handler, /* SVC */
    0, 0,
    fault_handler, /* PendSV */
    systick_handler,
    /* interrupts 0..8: none is ever enabled, so reaching one is a bug */
    fault_handler, fault_handler, fault_handler, fault_handler, fault_handler,
    fault_handler, fault_handler, fault_handler, fault_handler,
    exti3_handler, /* IRQ 9: tube pulse */
    fault_handler, fault_handler, fault_handler, fault_handler, fault_handler, /* IRQ 10..14 */
    fault_handler, fault_handler, fault_handler, fault_handler, fault_handler, /* IRQ 15..19 */
    usb_isr,       /* IRQ 20: USB */
};

BOOT void reset_handler(void)
{
    if (!image_ok())
        power_off_now();

    /* The reset flags are read and acted on in main (hal_gc03_boot_record): a hung run no longer ends here, in the first 1 KB. */

    /* The bootloader hands over with its own vector table in place. */
    *(volatile uint32_t *)0xE000ED08 = IMAGE_BASE;

    for (uint32_t *src = &_sidata, *dst = &_sdata; dst < &_edata;)
        *dst++ = *src++;
    for (uint32_t *dst = &_sbss; dst < &_ebss;)
        *dst++ = 0;

    /* The bootloader may jump here with interrupts masked; every vector is filled, so unmask them. */
    __asm__ volatile("cpsie i" ::: "memory");

    main();
    power_off_now();
}
