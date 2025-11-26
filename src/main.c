#include <stdio.h>
#include <signal.h>
#include <bcm2835.h>
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "lcd/framebuffer.h"
#include "input/button.h"
#include "input/joystick.h"
#include "../assets/images.h"

// Global flag for graceful shutdown
static volatile int g_running = 1;

// Circle position and properties
static int16_t circle_x = ST7789_WIDTH / 2;   // Start at center (120)
static int16_t circle_y = ST7789_HEIGHT / 2;  // Start at center (120)
static const int16_t circle_radius = 20;      // Circle radius
static const int16_t move_speed = 3;          // Pixels to move per input

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

    // Draw the circle at center position
    fb_fill_circle(circle_x, circle_y, circle_radius, COLOR_CYAN);

    // Send frame buffer to LCD
    fb_flush();
}

void update_ui(void) {
    // Read joystick input
    joystick_state_t joy = joystick_read_state();

    // Update circle position based on joystick input
    if (joy.up) {
        circle_y -= move_speed;
    }
    if (joy.down) {
        circle_y += move_speed;
    }
    if (joy.left) {
        circle_x -= move_speed;
    }
    if (joy.right) {
        circle_x += move_speed;
    }

    // Keep circle within screen boundaries
    if (circle_x - circle_radius < 0) {
        circle_x = circle_radius;
    }
    if (circle_x + circle_radius >= ST7789_WIDTH) {
        circle_x = ST7789_WIDTH - circle_radius - 1;
    }
    if (circle_y - circle_radius < 0) {
        circle_y = circle_radius;
    }
    if (circle_y + circle_radius >= ST7789_HEIGHT) {
        circle_y = ST7789_HEIGHT - circle_radius - 1;
    }

    // Clear frame buffer
    fb_clear(COLOR_BLACK);

    // Draw cannon at bottom left (with 10px margin)
    fb_draw_bitmap(10, ST7789_HEIGHT - cannon_bitmap.height - 10, &cannon_bitmap);

    // Draw the circle at current position
    fb_fill_circle(circle_x, circle_y, circle_radius, COLOR_CYAN);

    // Send frame buffer to LCD
    fb_flush();
}

void run_interactive_demo(void) {
    printf("\n=== Joystick-Controlled Circle Demo ===\n");
    printf("Use joystick to move the circle\n");
    printf("Press Ctrl+C to exit\n\n");

    // Draw initial UI
    draw_ui();

    uint32_t frame_count = 0;

    while (g_running) {
        // Update UI every frame
        update_ui();

        // Print position every 100 frames (~1 second)
        if (frame_count % 100 == 0) {
            printf("Circle position: (%d, %d)\n", circle_x, circle_y);
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
