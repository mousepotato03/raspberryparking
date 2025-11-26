#include <stdio.h>
#include <signal.h>
#include <bcm2835.h>
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "lcd/framebuffer.h"
#include "input/button.h"
#include "input/joystick.h"
#include "../assets/images.h"
#include "../assets/car.h"

// Global flag for graceful shutdown
static volatile int g_running = 1;

// Car bitmap size constants
#define CAR_WIDTH  50
#define CAR_HEIGHT 50

// Car position and properties
static int16_t car_x = (ST7789_WIDTH - CAR_WIDTH) / 2;   // Start at center (95)
static int16_t car_y = (ST7789_HEIGHT - CAR_HEIGHT) / 2; // Start at center (95)
static const int16_t move_speed = 3;                      // Pixels to move per input

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

void draw_ui(void) {
    // Clear frame buffer
    fb_clear(COLOR_BLACK);

    // Draw the car bitmap at current position
    fb_draw_bitmap(car_x, car_y, car_50x50_bitmap);

    // Send frame buffer to LCD
    fb_flush();
}

void update_ui(void) {
    // Read joystick input
    joystick_state_t joy = joystick_read_state();

    // Update car position based on joystick input
    if (joy.up) {
        car_y -= move_speed;
    }
    if (joy.down) {
        car_y += move_speed;
    }
    if (joy.left) {
        car_x -= move_speed;
    }
    if (joy.right) {
        car_x += move_speed;
    }

    // Keep car within screen boundaries
    if (car_x < 0) {
        car_x = 0;
    }
    if (car_x + CAR_WIDTH > ST7789_WIDTH) {
        car_x = ST7789_WIDTH - CAR_WIDTH;
    }
    if (car_y < 0) {
        car_y = 0;
    }
    if (car_y + CAR_HEIGHT > ST7789_HEIGHT) {
        car_y = ST7789_HEIGHT - CAR_HEIGHT;
    }

    // Clear frame buffer
    fb_clear(COLOR_BLACK);

    // Draw the car bitmap at current position
    fb_draw_bitmap(car_x, car_y, car_50x50_bitmap);

    // Send frame buffer to LCD
    fb_flush();
}

void run_interactive_demo(void) {
    printf("\n=== Joystick-Controlled Car Demo ===\n");
    printf("Use joystick to move the car\n");
    printf("Press Ctrl+C to exit\n\n");

    // Draw initial UI
    draw_ui();

    uint32_t frame_count = 0;

    while (g_running) {
        // Update UI every frame
        update_ui();

        // Print position every 100 frames (~1 second)
        if (frame_count % 100 == 0) {
            printf("Car position: (%d, %d)\n", car_x, car_y);
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
