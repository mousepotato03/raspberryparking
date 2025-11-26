#pragma once

#include <stdint.h>

/**
 * Joystick direction definitions
 */
typedef enum {
    JOY_DIR_NONE = 0,
    JOY_DIR_UP = 1,
    JOY_DIR_DOWN = 2,
    JOY_DIR_LEFT = 3,
    JOY_DIR_RIGHT = 4
} joystick_dir_t;

/**
 * Joystick state structure
 */
typedef struct {
    uint8_t up;
    uint8_t down;
    uint8_t left;
    uint8_t right;
} joystick_state_t;

/**
 * Read raw joystick state (all directions)
 * Returns: joystick_state_t with individual button states
 */
joystick_state_t joystick_read_state(void);

/**
 * Get current joystick direction (priority: UP > DOWN > LEFT > RIGHT)
 * Returns: joystick_dir_t
 */
joystick_dir_t joystick_get_direction(void);

/**
 * Check if specific direction is pressed
 * Returns: 1 if pressed, 0 if not
 */
uint8_t joystick_is_up(void);
uint8_t joystick_is_down(void);
uint8_t joystick_is_left(void);
uint8_t joystick_is_right(void);

/**
 * Wait for any joystick input (blocking)
 * Returns: direction that was pressed
 */
joystick_dir_t joystick_wait_any(void);
