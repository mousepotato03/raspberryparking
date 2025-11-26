/**
 * @file framebuffer.c
 * @brief Frame buffer implementation
 */

#include "framebuffer.h"
#include <string.h>

// Frame buffer: 240x240 pixels, RGB565 format
// Size: 240 * 240 * 2 bytes = 115,200 bytes (~112.5 KB)
static uint16_t framebuffer[ST7789_HEIGHT][ST7789_WIDTH];

void fb_init(void) {
    // Initialize frame buffer to black
    fb_clear(0x0000);
}

void fb_clear(uint16_t color) {
    for (int y = 0; y < ST7789_HEIGHT; y++) {
        for (int x = 0; x < ST7789_WIDTH; x++) {
            framebuffer[y][x] = color;
        }
    }
}

void fb_set_pixel(uint16_t x, uint16_t y, uint16_t color) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }
    framebuffer[y][x] = color;
}

uint16_t fb_get_pixel(uint16_t x, uint16_t y) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return 0x0000;
    }
    return framebuffer[y][x];
}

void fb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    // Clip rectangle to screen boundaries
    uint16_t x1 = x + w;
    uint16_t y1 = y + h;

    if (x1 > ST7789_WIDTH) {
        x1 = ST7789_WIDTH;
    }
    if (y1 > ST7789_HEIGHT) {
        y1 = ST7789_HEIGHT;
    }

    // Draw rectangle to frame buffer
    for (uint16_t py = y; py < y1; py++) {
        for (uint16_t px = x; px < x1; px++) {
            framebuffer[py][px] = color;
        }
    }
}

void fb_draw_bitmap(uint16_t x, uint16_t y, const bitmap* bmp) {
    if (bmp == NULL || bmp->bitmap == NULL) {
        return;
    }

    // Draw bitmap with boundary checking and clipping
    for (uint16_t by = 0; by < bmp->height; by++) {
        for (uint16_t bx = 0; bx < bmp->width; bx++) {
            uint16_t screen_x = x + bx;
            uint16_t screen_y = y + by;

            // Skip pixels outside screen boundaries
            if (screen_x >= ST7789_WIDTH || screen_y >= ST7789_HEIGHT) {
                continue;
            }

            // Copy pixel from bitmap to frame buffer
            uint16_t pixel_color = bmp->bitmap[by * bmp->width + bx];
            framebuffer[screen_y][screen_x] = pixel_color;
        }
    }
}

// Helper function to draw 8 symmetric points of a circle
static void draw_circle_points(int16_t x0, int16_t y0, int16_t x, int16_t y, uint16_t color) {
    // Draw 8 symmetric points
    if (x0 + x >= 0 && x0 + x < ST7789_WIDTH && y0 + y >= 0 && y0 + y < ST7789_HEIGHT)
        framebuffer[y0 + y][x0 + x] = color;
    if (x0 - x >= 0 && x0 - x < ST7789_WIDTH && y0 + y >= 0 && y0 + y < ST7789_HEIGHT)
        framebuffer[y0 + y][x0 - x] = color;
    if (x0 + x >= 0 && x0 + x < ST7789_WIDTH && y0 - y >= 0 && y0 - y < ST7789_HEIGHT)
        framebuffer[y0 - y][x0 + x] = color;
    if (x0 - x >= 0 && x0 - x < ST7789_WIDTH && y0 - y >= 0 && y0 - y < ST7789_HEIGHT)
        framebuffer[y0 - y][x0 - x] = color;
    if (x0 + y >= 0 && x0 + y < ST7789_WIDTH && y0 + x >= 0 && y0 + x < ST7789_HEIGHT)
        framebuffer[y0 + x][x0 + y] = color;
    if (x0 - y >= 0 && x0 - y < ST7789_WIDTH && y0 + x >= 0 && y0 + x < ST7789_HEIGHT)
        framebuffer[y0 + x][x0 - y] = color;
    if (x0 + y >= 0 && x0 + y < ST7789_WIDTH && y0 - x >= 0 && y0 - x < ST7789_HEIGHT)
        framebuffer[y0 - x][x0 + y] = color;
    if (x0 - y >= 0 && x0 - y < ST7789_WIDTH && y0 - x >= 0 && y0 - x < ST7789_HEIGHT)
        framebuffer[y0 - x][x0 - y] = color;
}

void fb_draw_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    if (radius < 0) return;

    // Midpoint circle algorithm
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        draw_circle_points(x0, y0, x, y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fb_fill_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    if (radius < 0) return;

    // Draw filled circle by drawing horizontal lines
    for (int16_t y = -radius; y <= radius; y++) {
        for (int16_t x = -radius; x <= radius; x++) {
            // Check if point is inside circle using distance formula
            if (x * x + y * y <= radius * radius) {
                int16_t px = x0 + x;
                int16_t py = y0 + y;

                // Boundary check
                if (px >= 0 && px < ST7789_WIDTH && py >= 0 && py < ST7789_HEIGHT) {
                    framebuffer[py][px] = color;
                }
            }
        }
    }
}

void fb_flush(void) {
    // Send entire frame buffer to LCD
    // This function will use the st7789_write_framebuffer() function
    st7789_write_framebuffer((uint16_t*)framebuffer, ST7789_WIDTH * ST7789_HEIGHT);
}

uint16_t* fb_get_buffer(void) {
    return (uint16_t*)framebuffer;
}
