#pragma once

#include <stdint.h>

// Button state definitions
#define BUTTON_RELEASED 0
#define BUTTON_PRESSED  1

// Debounce delay in milliseconds
#define DEBOUNCE_DELAY_MS 20

/**
 * Button identifiers
 */
typedef enum {
    BTN_A = 0,
    BTN_B = 1,
    BTN_COUNT = 2
} button_id_t;

/**
 * Read raw button state (no debouncing)
 * Returns: BUTTON_PRESSED or BUTTON_RELEASED
 */
uint8_t button_read_raw(button_id_t btn);

/**
 * Read button state with debouncing
 * Returns: BUTTON_PRESSED or BUTTON_RELEASED
 */
uint8_t button_read(button_id_t btn);

/**
 * Check if button is currently pressed (with debouncing)
 * Returns: 1 if pressed, 0 if released
 */
uint8_t button_is_pressed(button_id_t btn);

/**
 * Wait for button to be released (blocking)
 */
void button_wait_release(button_id_t btn);

/**
 * Wait for button to be pressed (blocking)
 */
void button_wait_press(button_id_t btn);
