#include "joystick.h"
#include "../common/gpio_init.h"
#include <bcm2835.h>

// Debounce delay in milliseconds
#define JOY_DEBOUNCE_MS 20

joystick_state_t joystick_read_state(void) {
    joystick_state_t state;

    // Read all joystick pins (LOW = pressed due to pull-up)
    state.up = (bcm2835_gpio_lev(JOY_UP) == LOW) ? 1 : 0;
    state.down = (bcm2835_gpio_lev(JOY_DOWN) == LOW) ? 1 : 0;
    state.left = (bcm2835_gpio_lev(JOY_LEFT) == LOW) ? 1 : 0;
    state.right = (bcm2835_gpio_lev(JOY_RIGHT) == LOW) ? 1 : 0;

    return state;
}

joystick_dir_t joystick_get_direction(void) {
    joystick_state_t state = joystick_read_state();

    // Priority: UP > DOWN > LEFT > RIGHT
    if (state.up) {
        return JOY_DIR_UP;
    }
    if (state.down) {
        return JOY_DIR_DOWN;
    }
    if (state.left) {
        return JOY_DIR_LEFT;
    }
    if (state.right) {
        return JOY_DIR_RIGHT;
    }

    return JOY_DIR_NONE;
}

uint8_t joystick_is_up(void) {
    uint8_t state1 = (bcm2835_gpio_lev(JOY_UP) == LOW);
    bcm2835_delay(JOY_DEBOUNCE_MS);
    uint8_t state2 = (bcm2835_gpio_lev(JOY_UP) == LOW);
    return (state1 && state2);
}

uint8_t joystick_is_down(void) {
    uint8_t state1 = (bcm2835_gpio_lev(JOY_DOWN) == LOW);
    bcm2835_delay(JOY_DEBOUNCE_MS);
    uint8_t state2 = (bcm2835_gpio_lev(JOY_DOWN) == LOW);
    return (state1 && state2);
}

uint8_t joystick_is_left(void) {
    uint8_t state1 = (bcm2835_gpio_lev(JOY_LEFT) == LOW);
    bcm2835_delay(JOY_DEBOUNCE_MS);
    uint8_t state2 = (bcm2835_gpio_lev(JOY_LEFT) == LOW);
    return (state1 && state2);
}

uint8_t joystick_is_right(void) {
    uint8_t state1 = (bcm2835_gpio_lev(JOY_RIGHT) == LOW);
    bcm2835_delay(JOY_DEBOUNCE_MS);
    uint8_t state2 = (bcm2835_gpio_lev(JOY_RIGHT) == LOW);
    return (state1 && state2);
}

joystick_dir_t joystick_wait_any(void) {
    joystick_dir_t dir;

    // Wait until any direction is pressed
    do {
        bcm2835_delay(10);
        dir = joystick_get_direction();
    } while (dir == JOY_DIR_NONE);

    // Debounce delay
    bcm2835_delay(JOY_DEBOUNCE_MS);

    return dir;
}
