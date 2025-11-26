/**
 * @file framebuffer.h
 * @brief Frame buffer management for LCD display
 *
 * This module provides a 2D array-based frame buffer for managing screen content.
 * Objects and bitmaps are drawn to the frame buffer in memory, then the entire
 * buffer is sent to the LCD display at once.
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include "st7789.h"
#include "../../assets/images.h"

/**
 * @brief Initialize the frame buffer
 *
 * Must be called before using any other frame buffer functions.
 */
void fb_init(void);

/**
 * @brief Clear the entire frame buffer with a color
 * @param color RGB565 color value
 */
void fb_clear(uint16_t color);

/**
 * @brief Set a single pixel in the frame buffer
 * @param x X coordinate (0-239)
 * @param y Y coordinate (0-239)
 * @param color RGB565 color value
 */
void fb_set_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Get a pixel color from the frame buffer
 * @param x X coordinate (0-239)
 * @param y Y coordinate (0-239)
 * @return RGB565 color value
 */
uint16_t fb_get_pixel(uint16_t x, uint16_t y);

/**
 * @brief Draw a filled rectangle to the frame buffer
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param w Width of rectangle
 * @param h Height of rectangle
 * @param color RGB565 color value
 */
void fb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw a bitmap to the frame buffer
 *
 * Copies bitmap pixel data into the frame buffer at the specified position.
 * Performs boundary checking and clips the bitmap if it extends beyond screen edges.
 *
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param bmp Pointer to bitmap structure
 */
void fb_draw_bitmap(uint16_t x, uint16_t y, const bitmap* bmp);

/**
 * @brief Draw a circle outline to the frame buffer
 * @param x0 X coordinate of center
 * @param y0 Y coordinate of center
 * @param radius Radius of the circle
 * @param color RGB565 color value
 */
void fb_draw_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color);

/**
 * @brief Draw a filled circle to the frame buffer
 * @param x0 X coordinate of center
 * @param y0 Y coordinate of center
 * @param radius Radius of the circle
 * @param color RGB565 color value
 */
void fb_fill_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color);

/**
 * @brief Send the entire frame buffer to the LCD display
 *
 * Transfers all 240x240 pixels from the frame buffer to the LCD in one operation.
 * This should be called once per frame after all drawing operations are complete.
 */
void fb_flush(void);

/**
 * @brief Get pointer to the frame buffer array
 * @return Pointer to the frame buffer data (for advanced usage)
 */
uint16_t* fb_get_buffer(void);

#endif // FRAMEBUFFER_H
