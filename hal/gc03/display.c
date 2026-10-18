/* ST7789 240x320 on SPI1: SCK PA5, MOSI PA7, CS PB0, DC PC4, RESET PC5 */

#include "display.h"
#include "board.h"
#include "regs.h"

#define CS(v)  gpio_write(GPIOB, 0, (v))
#define DC(v)  gpio_write(GPIOC, 4, (v))
#define RES(v) gpio_write(GPIOC, 5, (v))

static void spi_byte(uint8_t b)
{
    while (!(SPI1_SR & 2u)) {} /* TXE */
    SPI1_DR = b;
}

static void spi_idle(void)
{
    while (!(SPI1_SR & 2u)) {}
    while (SPI1_SR & 0x80u) {} /* BSY */
}

static void cmd(uint8_t c)
{
    spi_idle();
    DC(false);
    CS(false);
    spi_byte(c);
    spi_idle();
    DC(true);
}

static void data(uint8_t d) { spi_byte(d); }

static void end(void)
{
    spi_idle();
    CS(true);
}

static void send_table(uint8_t c, const uint8_t *p, int n)
{
    cmd(c);
    for (int i = 0; i < n; i++)
        data(p[i]);
    end();
}

void display_init(void)
{
    RCC_APB2ENR |= 1u << 12; /* SPI1 */

    gpio_write(GPIOB, 0, true);
    gpio_write(GPIOC, 5, true);
    gpio_mode(GPIOB, 0, 0x3);
    gpio_mode(GPIOC, 4, 0x3);
    gpio_mode(GPIOC, 5, 0x3);
    gpio_mode(GPIOA, 5, 0xB);
    gpio_mode(GPIOA, 7, 0xB);

    SPI1_CR1 = (1u << 2) | (1u << 3) | (1u << 8) | (1u << 9); /* master, fPCLK/4, soft NSS */
    SPI1_CR1 |= 1u << 6;                                       /* enable */

    RES(true);
    delay_ms(5);
    RES(false);
    delay_ms(10);
    RES(true);
    delay_ms(120);

    cmd(0x01); end(); delay_ms(150);       /* software reset */
    cmd(0x11); end(); delay_ms(120);       /* sleep out */

    static const uint8_t colmod[] = {0x55};
    static const uint8_t madctl[] = {0x00};
    static const uint8_t vcoms[] = {0x2C};
    static const uint8_t vrhs[] = {0x12};
    static const uint8_t pwctrl1[] = {0xA4, 0xA1};
    static const uint8_t pgamma[] = {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19};
    static const uint8_t ngamma[] = {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19};

    send_table(0x3A, colmod, 1);  /* 16 bit colour */
    send_table(0x36, madctl, 1);  /* portrait, RGB order */
    send_table(0xBB, vcoms, 1);
    send_table(0xC3, vrhs, 1);
    send_table(0xD0, pwctrl1, 2);
    send_table(0xE0, pgamma, 14);
    send_table(0xE1, ngamma, 14);
    cmd(0x20); end();                      /* inversion off (this panel is wired non-inverted) */
    cmd(0x13); end();                      /* normal display mode */
    cmd(0x29); end(); delay_ms(20);        /* display on */
}

void display_raw_window(int x, int y, int w, int h)
{
    uint16_t x1 = x + w - 1, y1 = y + h - 1;
    cmd(0x2A);
    data(x >> 8); data(x); data(x1 >> 8); data(x1);
    end();
    cmd(0x2B);
    data(y >> 8); data(y); data(y1 >> 8); data(y1);
    end();
    cmd(0x2C);
}

void display_raw_pixel(uint16_t color)
{
    spi_byte(color >> 8);
    spi_byte(color);
}

void display_raw_end(void) { end(); }

void display_raw_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > DISPLAY_W || y + h > DISPLAY_H)
        return;
    display_raw_window(x, y, w, h);
    for (long n = (long)w * h; n > 0; n--)
        display_raw_pixel(color);
    display_raw_end();
}

void display_sleep(bool sleep)
{
    if (sleep) {
        cmd(0x28); end();               /* display off */
        cmd(0x10); end();               /* sleep in */
    } else {
        cmd(0x11); end(); delay_ms(120); /* sleep out */
        cmd(0x29); end();               /* display on */
    }
}
