#include "st7789.h"
#include "../common/gpio_init.h"
#include <stdio.h>

void st7789_write_command(uint8_t cmd) {
    bcm2835_gpio_clr(TFT_DC);  // DC = LOW (command mode)
    bcm2835_spi_transfer(cmd);
}

void st7789_write_data(uint8_t data) {
    bcm2835_gpio_set(TFT_DC);  // DC = HIGH (data mode)
    bcm2835_spi_transfer(data);
}

void st7789_init(void) {
    // Initialize SPI
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);  // ~31.25 MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    // Hardware reset
    bcm2835_gpio_clr(TFT_RST);
    bcm2835_delay(100);
    bcm2835_gpio_set(TFT_RST);
    bcm2835_delay(100);

    // Software reset
    st7789_write_command(ST7789_SWRESET);
    bcm2835_delay(150);

    // Sleep out
    st7789_write_command(ST7789_SLPOUT);
    bcm2835_delay(500);

    // Color mode: 16-bit (RGB565)
    st7789_write_command(ST7789_COLMOD);
    st7789_write_data(0x55);
    bcm2835_delay(10);

    // Memory access control (display orientation)
    st7789_write_command(ST7789_MADCTL);
    st7789_write_data(0x00);

    // Column address set (X range)
    st7789_write_command(ST7789_CASET);
    st7789_write_data(0x00);
    st7789_write_data(0x00);
    st7789_write_data((ST7789_WIDTH - 1) >> 8);
    st7789_write_data((ST7789_WIDTH - 1) & 0xFF);

    // Row address set (Y range)
    st7789_write_command(ST7789_RASET);
    st7789_write_data(0x00);
    st7789_write_data(0x00);
    st7789_write_data((ST7789_HEIGHT - 1) >> 8);
    st7789_write_data((ST7789_HEIGHT - 1) & 0xFF);

    // Display on
    st7789_write_command(ST7789_DISPON);
    bcm2835_delay(100);

    printf("ST7789 LCD initialized\n");
}

void st7789_fill_screen(uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    st7789_write_command(ST7789_RAMWR);
    bcm2835_gpio_set(TFT_DC);  // Data mode

    int32_t i, j;
    for (i = 0; i < ST7789_HEIGHT; i++) {
        for (j = 0; j < ST7789_WIDTH; j++) {
            bcm2835_spi_transfer(hi);
            bcm2835_spi_transfer(lo);
        }
    }
}

void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column address set
    st7789_write_command(ST7789_CASET);
    st7789_write_data(x0 >> 8);
    st7789_write_data(x0 & 0xFF);
    st7789_write_data(x1 >> 8);
    st7789_write_data(x1 & 0xFF);

    // Row address set
    st7789_write_command(ST7789_RASET);
    st7789_write_data(y0 >> 8);
    st7789_write_data(y0 & 0xFF);
    st7789_write_data(y1 >> 8);
    st7789_write_data(y1 & 0xFF);
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    st7789_set_window(x, y, x, y);
    st7789_write_command(ST7789_RAMWR);
    st7789_write_data(color >> 8);
    st7789_write_data(color & 0xFF);
}

void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;

    if (x1 >= ST7789_WIDTH) {
        x1 = ST7789_WIDTH - 1;
    }
    if (y1 >= ST7789_HEIGHT) {
        y1 = ST7789_HEIGHT - 1;
    }

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    st7789_set_window(x, y, x1, y1);
    st7789_write_command(ST7789_RAMWR);
    bcm2835_gpio_set(TFT_DC);  // Data mode

    int32_t i, j;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            bcm2835_spi_transfer(hi);
            bcm2835_spi_transfer(lo);
        }
    }
}

void st7789_write_framebuffer(uint16_t* buffer, size_t length) {
    if (buffer == NULL || length == 0) {
        return;
    }

    // Set window to entire screen
    st7789_set_window(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);

    // Start RAM write
    st7789_write_command(ST7789_RAMWR);
    bcm2835_gpio_set(TFT_DC);  // Data mode

    // Send all pixels
    for (size_t i = 0; i < length; i++) {
        uint16_t pixel = buffer[i];
        bcm2835_spi_transfer(pixel >> 8);    // High byte
        bcm2835_spi_transfer(pixel & 0xFF);  // Low byte
    }
}

uint16_t st7789_rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
