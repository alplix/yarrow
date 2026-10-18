#include "board.h"
#include "regs.h"

/* GPIO pin modes (CNF[1:0] MODE[1:0]) */
#define PIN_OUT_PP   0x3u
#define PIN_AF_PP    0xBu
#define PIN_IN_PULL  0x8u
#define PIN_IN_FLOAT 0x4u

static uint32_t ticks_per_ms = 8000; /* HSI until the PLL is up */

volatile uint32_t g_ms;
volatile bool g_tick_irq;

uint32_t board_cpu_hz(void) { return ticks_per_ms * 1000u; }

void gpio_mode(uint32_t port, unsigned pin, uint32_t mode)
{
    volatile uint32_t *cr = pin < 8 ? &GPIO_CRL(port) : &GPIO_CRH(port);
    unsigned shift = (pin & 7u) * 4u;
    *cr = (*cr & ~(0xFu << shift)) | (mode << shift);
}

void gpio_write(uint32_t port, unsigned pin, bool high)
{
    GPIO_BSRR(port) = high ? (1u << pin) : (1u << (pin + 16u));
}

bool gpio_read(uint32_t port, unsigned pin)
{
    return (GPIO_IDR(port) >> pin) & 1u;
}

static void clock_init(void)
{
    uint32_t guard;

    RCC_CR |= 1u << 16; /* HSEON */
    for (guard = 0; guard < 200000 && !(RCC_CR & (1u << 17)); guard++) {}
    if (!(RCC_CR & (1u << 17)))
        return; /* stay on the internal oscillator */

    FLASH_ACR = 0x12u; /* 2 wait states, prefetch on */

    /* PLL source HSE, x12 -> 96 MHz; APB1 = /2; ADC = /8 */
    RCC_CFGR = (1u << 16) | (10u << 18) | (4u << 8) | (3u << 14);
    RCC_CR |= 1u << 24; /* PLLON */
    for (guard = 0; guard < 200000 && !(RCC_CR & (1u << 25)); guard++) {}
    if (!(RCC_CR & (1u << 25)))
        return;

    RCC_CFGR |= 2u; /* SW = PLL */
    for (guard = 0; guard < 200000 && ((RCC_CFGR >> 2) & 3u) != 2u; guard++) {}
    if (((RCC_CFGR >> 2) & 3u) == 2u)
        ticks_per_ms = 96000;
}

void board_init(void)
{
    /* Clocks: AFIO, GPIOA, GPIOB, GPIOC */
    RCC_APB2ENR |= (1u << 0) | (1u << 2) | (1u << 3) | (1u << 4);

    /* Latch power before anything else can go wrong. */
    gpio_write(GPIOA, 10, true);
    gpio_mode(GPIOA, 10, PIN_OUT_PP);

    clock_init();

    /* Keep SWD usable, drop JTAG so PB3/PB4 are free. */
    AFIO_MAPR = (AFIO_MAPR & ~(7u << 24)) | (2u << 24);

    gpio_mode(GPIOA, 9, PIN_IN_PULL);
    gpio_write(GPIOA, 9, true); /* ODR bit selects pull-up */

    gpio_write(GPIOB, 13, false);
    gpio_mode(GPIOB, 13, PIN_OUT_PP); /* backlight, off until the first frame is ready */
}

void delay_ms(uint32_t ms)
{
    if (g_tick_irq) { /* the tick interrupt owns SysTick now */
        uint32_t t = g_ms;
        while ((uint32_t)(g_ms - t) <= ms)
            __asm__ volatile("wfi");
        return;
    }
    SYST_RVR = ticks_per_ms - 1u;
    SYST_CVR = 0;
    SYST_CSR = 5u; /* core clock, enabled, no interrupt */
    while (ms--) {
        while (!(SYST_CSR & (1u << 16))) {}
    }
    SYST_CSR = 0;
}

bool board_power_key_low(void) { return !gpio_read(GPIOA, 9); }

void board_power_off(void)
{
    gpio_write(GPIOA, 10, false);
    for (;;) {}
}

void board_backlight(bool on) { gpio_write(GPIOB, 13, on); }

void watchdog_start(void)
{
    /* Order matters: start the counter first (this also switches the 40 kHz oscillator on), then unlock and set the
     * period, wait until the new values have been transferred, and only then reload. Setting the period first
     * leaves the counter running with its short default period (about 0.4 s) until the first reload. */
    IWDG_KR = 0xCCCC;
    IWDG_KR = 0x5555;
    IWDG_PR = 4;     /* 40 kHz / 64 = 625 Hz */
    IWDG_RLR = 2500; /* 4 s */
    for (uint32_t guard = 0; guard < 2000000 && IWDG_SR; guard++) {}
    IWDG_KR = 0xAAAA;
}

void watchdog_kick(void) { IWDG_KR = 0xAAAA; }
