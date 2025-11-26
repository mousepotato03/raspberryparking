#pragma once

#include <bcm2835.h>
#include <stdint.h>

// LCD (ST7789) Pin Definitions
#define TFT_CS      8
#define TFT_DC      25
#define TFT_RST     24
#define TFT_BL      26

// Joystick Pin Definitions (Digital 4-way)
#define JOY_UP      17
#define JOY_DOWN    22
#define JOY_LEFT    27
#define JOY_RIGHT   23

// Button Pin Definitions
#define BUTTON_A    5
#define BUTTON_B    6

/**
 * Initialize BCM2835 library and all GPIO pins
 * Returns: 1 on success, 0 on failure
 */
int32_t gpio_init_all(void);

/**
 * Cleanup and close BCM2835 library
 */
void gpio_cleanup(void);
