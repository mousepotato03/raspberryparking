#pragma once

#include <bcm2835.h>
#include <stdint.h>
#include <stddef.h>

// Display dimensions
#define ST7789_WIDTH   240
#define ST7789_HEIGHT  240

// ST7789 commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_COLMOD  0x3A

// RGB565 color definitions
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

/**
 * Write command to ST7789
 */
void st7789_write_command(uint8_t cmd);

/**
 * Write data to ST7789
 */
void st7789_write_data(uint8_t data);

/**
 * Initialize ST7789 LCD
 */
void st7789_init(void);

/**
 * Fill entire screen with a color
 */
void st7789_fill_screen(uint16_t color);

/**
 * Set drawing window
 */
void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * Draw a single pixel
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * Draw a filled rectangle
 */
void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Write frame buffer to LCD
 * Optimized function to send entire frame buffer at once
 * @param buffer Pointer to frame buffer array (RGB565 format)
 * @param length Number of pixels to write
 */
void st7789_write_framebuffer(uint16_t* buffer, size_t length);

/**
 * Convert RGB888 to RGB565
 */
uint16_t st7789_rgb_to_565(uint8_t r, uint8_t g, uint8_t b);
