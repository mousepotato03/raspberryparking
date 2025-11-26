#include "button.h"
#include "../common/gpio_init.h"
#include <bcm2835.h>

// Button pin mapping
static const uint8_t s_button_pins[BTN_COUNT] = {
    BUTTON_A,
    BUTTON_B
};

uint8_t button_read_raw(button_id_t btn) {
    if (btn >= BTN_COUNT) {
        return BUTTON_RELEASED;
    }

    // Read GPIO level (LOW = pressed due to pull-up)
    uint8_t level = bcm2835_gpio_lev(s_button_pins[btn]);
    return (level == LOW) ? BUTTON_PRESSED : BUTTON_RELEASED;
}

uint8_t button_read(button_id_t btn) {
    if (btn >= BTN_COUNT) {
        return BUTTON_RELEASED;
    }

    // Read initial state
    uint8_t state1 = button_read_raw(btn);

    // Wait for debounce delay
    bcm2835_delay(DEBOUNCE_DELAY_MS);

    // Read again and compare
    uint8_t state2 = button_read_raw(btn);

    // Return stable state
    return (state1 == state2) ? state1 : BUTTON_RELEASED;
}

uint8_t button_is_pressed(button_id_t btn) {
    return (button_read(btn) == BUTTON_PRESSED);
}

void button_wait_release(button_id_t btn) {
    if (btn >= BTN_COUNT) {
        return;
    }

    while (button_read_raw(btn) == BUTTON_PRESSED) {
        bcm2835_delay(10);
    }

    // Additional debounce delay
    bcm2835_delay(DEBOUNCE_DELAY_MS);
}

void button_wait_press(button_id_t btn) {
    if (btn >= BTN_COUNT) {
        return;
    }

    while (button_read_raw(btn) == BUTTON_RELEASED) {
        bcm2835_delay(10);
    }

    // Additional debounce delay
    bcm2835_delay(DEBOUNCE_DELAY_MS);
}
