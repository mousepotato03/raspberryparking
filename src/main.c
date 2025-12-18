#include <stdio.h>
#include <signal.h>
#include <bcm2835.h>
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "lcd/framebuffer.h"
#include "input/button.h"
#include "input/joystick.h"
#include "game/car_physics.h"
#include "../assets/images.h"
#include "../assets/car.h"
#include "../assets/handle.h"
#include "../assets/easy_map.h"
#include "../assets/intro.h"
#include "../assets/hard_map.h"

// Global flag for graceful shutdown
static volatile int g_running = 1;

// Map type definitions
typedef enum {
    MAP_EASY,
    MAP_HARD
} map_type_t;

// Current map pointer
static const bitmap* g_current_map_bitmap = NULL;

// Car bitmap size constants
#define CAR_WIDTH  100
#define CAR_HEIGHT 100

// Car hitbox size (actual car bounds within bitmap)
#define CAR_HITBOX_WIDTH  30
#define CAR_HITBOX_HEIGHT 55

// Handle bitmap size and position constants
#define HANDLE_WIDTH  80
#define HANDLE_HEIGHT 80
#define HANDLE_X      (HANDLE_WIDTH / 2)              // Handle center X (left side)
#define HANDLE_Y      (ST7789_HEIGHT - HANDLE_HEIGHT / 2)  // Handle center Y (bottom)

// Transparent color for bitmaps
#define TRANSPARENT_COLOR 0x0000

// Car state (physics-based)
static car_state_t g_car;

// Handle angle for UI (-45 ~ +45 degrees)
static int16_t g_handle_angle = 0;
#define HANDLE_ANGLE_MAX 45
#define HANDLE_ANGLE_RETURN_SPEED 5

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\nShutdown signal received...\n");
}

void show_intro_screen(void) {
    fb_draw_bitmap(0, 0, &intro_240x240_bitmap);
    fb_flush();
}

map_type_t wait_for_map_selection(void) {
    while (g_running) {
        if (button_read_raw(BTN_A) == BUTTON_PRESSED) {
            button_wait_release(BTN_A);
            return MAP_EASY;
        }
        if (button_read_raw(BTN_B) == BUTTON_PRESSED) {
            button_wait_release(BTN_B);
            return MAP_HARD;
        }
        bcm2835_delay(10);
    }
    return MAP_EASY;  // Default if interrupted
}

void set_current_map(map_type_t map) {
    if (map == MAP_EASY) {
        g_current_map_bitmap = &easy_map_240x240_bitmap;
        printf("Selected: Easy Map\n");
    } else {
        g_current_map_bitmap = &hard_map_240x240_bitmap;
        printf("Selected: Hard Map\n");
    }
}

void draw_game(void) {
    // Draw background map (covers entire screen)
    fb_draw_bitmap(0, 0, g_current_map_bitmap);

    // Get car screen position (center-based)
    int16_t car_cx = car_get_screen_x(&g_car);
    int16_t car_cy = car_get_screen_y(&g_car);

    // Draw the car bitmap rotated at current position and angle
    fb_draw_bitmap_rotated(car_cx, car_cy, &car_100x100_bitmap,
                           g_car.angle, TRANSPARENT_COLOR);

    // Draw the handle bitmap rotated at bottom-left corner
    fb_draw_bitmap_rotated(HANDLE_X, HANDLE_Y, &handle_80x80_bitmap,
                           g_handle_angle, TRANSPARENT_COLOR);

    // Send frame buffer to LCD
    fb_flush();
}

void process_input(void) {
    // Read joystick input
    joystick_state_t joy = joystick_read_state();

    // === Acceleration / Deceleration ===
    // Button A: Forward acceleration
    if (button_read_raw(BTN_A) == BUTTON_PRESSED) {
        car_apply_acceleration(&g_car, &default_car_params, true);
    }
    // Button B: Reverse acceleration
    else if (button_read_raw(BTN_B) == BUTTON_PRESSED) {
        car_apply_acceleration(&g_car, &default_car_params, false);
    }

    // Joystick down: Brake
    if (joy.down) {
        car_apply_brake(&g_car, &default_car_params);
    }

    // === Steering ===
    // Joystick left: Turn left
    if (joy.left) {
        car_apply_turn(&g_car, &default_car_params, -1);
        g_handle_angle = -HANDLE_ANGLE_MAX;
    }
    // Joystick right: Turn right
    else if (joy.right) {
        car_apply_turn(&g_car, &default_car_params, +1);
        g_handle_angle = HANDLE_ANGLE_MAX;
    }
    else {
        // Return handle to center gradually
        if (g_handle_angle > 0) {
            g_handle_angle -= HANDLE_ANGLE_RETURN_SPEED;
            if (g_handle_angle < 0) g_handle_angle = 0;
        }
        else if (g_handle_angle < 0) {
            g_handle_angle += HANDLE_ANGLE_RETURN_SPEED;
            if (g_handle_angle > 0) g_handle_angle = 0;
        }
    }
}

void update_game(void) {
    // Process player input
    process_input();

    // Update physics
    car_physics_update(&g_car, &default_car_params);

    // Keep car within screen boundaries (use hitbox size, not bitmap size)
    car_clamp_to_screen(&g_car, ST7789_WIDTH, ST7789_HEIGHT, CAR_HITBOX_WIDTH, CAR_HITBOX_HEIGHT);

    // Draw game
    draw_game();
}

void run_interactive_demo(void) {
    // Show intro screen
    printf("\n=== RaspberryParking ===\n");
    printf("Press A for Easy Map, B for Hard Map\n");
    show_intro_screen();

    // Wait for map selection
    map_type_t selected_map = wait_for_map_selection();
    set_current_map(selected_map);

    // Game instructions
    printf("\n=== Game Controls ===\n");
    printf("A button: Accelerate forward\n");
    printf("B button: Accelerate backward (reverse)\n");
    printf("Joystick left/right: Steer (reversed when reversing)\n");
    printf("Joystick down: Brake\n");
    printf("Press Ctrl+C to exit\n\n");

    // Initialize car at center-right position, facing up (0 degrees)
    int16_t start_x = ST7789_WIDTH - CAR_WIDTH / 2 - 10;   // Right side
    int16_t start_y = ST7789_HEIGHT - CAR_HEIGHT / 2 - 10; // Bottom
    car_physics_init(&g_car, start_x, start_y, 0);

    // Draw initial frame
    draw_game();

    while (g_running) {
        // Update game every frame
        update_game();
        bcm2835_delay(10);  // ~100 FPS
    }
}

int main(void) {
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize GPIO and BCM2835 library
    if (!gpio_init_all()) {
        printf("Failed to initialize GPIO\n");
        return 1;
    }

    // Initialize ST7789 LCD
    st7789_init();

    // Initialize frame buffer
    fb_init();
    printf("Frame buffer initialized\n");

    // Run interactive demo
    run_interactive_demo();

    // Cleanup
    printf("\nCleaning up...\n");
    fb_clear(COLOR_BLACK);
    fb_flush();
    bcm2835_spi_end();
    gpio_cleanup();

    printf("Program terminated successfully\n");
    return 0;
}
