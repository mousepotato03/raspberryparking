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

// Global flag for graceful shutdown
static volatile int g_running = 1;

// Car bitmap size constants
#define CAR_WIDTH  100
#define CAR_HEIGHT 100

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

void test_display(void) {
    printf("Testing display with color patterns...\n");

    // Fill screen with different colors using frame buffer
    fb_clear(COLOR_RED);
    fb_flush();
    bcm2835_delay(500);

    fb_clear(COLOR_GREEN);
    fb_flush();
    bcm2835_delay(500);

    fb_clear(COLOR_BLUE);
    fb_flush();
    bcm2835_delay(500);

    fb_clear(COLOR_BLACK);
    fb_flush();
    bcm2835_delay(500);

    // Test bitmap drawing
    printf("Testing bitmap drawing...\n");
    fb_clear(COLOR_BLACK);

    // Draw test bitmap (4x4) at different positions
    fb_draw_bitmap(50, 50, &clr_circle_bitmap);
    fb_draw_bitmap(100, 100, &clr_circle_bitmap);
    fb_draw_bitmap(150, 150, &clr_circle_bitmap);

    fb_flush();
    bcm2835_delay(1000);

    printf("Display test complete\n");
}

void draw_game(void) {
    // Draw background map (covers entire screen)
    fb_draw_bitmap(0, 0, &easy_map_240x240_bitmap);

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

    // Keep car within screen boundaries
    car_clamp_to_screen(&g_car, ST7789_WIDTH, ST7789_HEIGHT, CAR_WIDTH, CAR_HEIGHT);

    // Draw game
    draw_game();
}

void run_interactive_demo(void) {
    printf("\n=== Car Physics Demo ===\n");
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

    uint32_t frame_count = 0;

    while (g_running) {
        // Update game every frame
        update_game();

        // Print status every 100 frames (~1 second)
        if (frame_count % 100 == 0) {
            printf("Pos: (%d, %d) Angle: %d Speed: %ld\n",
                   car_get_screen_x(&g_car),
                   car_get_screen_y(&g_car),
                   g_car.angle,
                   (long)g_car.speed);
        }

        frame_count++;
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

    // Test display
    test_display();

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
