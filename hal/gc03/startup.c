/* Start-up code. Everything marked BOOT sits in the first 1 KB after the vector table so that
 * even a partly written image can still run the integrity check and switch the device off. */

#include "image.h"
#include "regs.h"
#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _stack_top;
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

/* Any exception that is not expected ends here instead of leaving the device latched on. */
BOOT __attribute__((noreturn)) static void fault_handler(void) { power_off_now(); }

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

    /* A watchdog reset means the last run hung: do not run again, switch off. */
    if (RCC_CSR & (1u << 29)) {
        RCC_CSR |= 1u << 24; /* clear the reset flags */
        power_off_now();
    }

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
