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
#include "../assets/obstacle.h"
#include "../assets/game_over.h"

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
#define CAR_HITBOX_HEIGHT 30  // 55 -> 45: 상하 경계 충돌 마진 확보

// Handle bitmap size and position constants
#define HANDLE_WIDTH  80
#define HANDLE_HEIGHT 80
#define HANDLE_X      (HANDLE_WIDTH / 2)              // Handle center X (left side)
#define HANDLE_Y      (ST7789_HEIGHT - HANDLE_HEIGHT / 2)  // Handle center Y (bottom)

// Transparent color for bitmaps
#define TRANSPARENT_COLOR 0x0000

// Obstacle constants
#define OBSTACLE_WIDTH  100
#define OBSTACLE_HEIGHT 100
#define OBSTACLE_HITBOX_WIDTH  30
#define OBSTACLE_HITBOX_HEIGHT 30
#define OBSTACLE_EASY_X 180
#define OBSTACLE_EASY_Y 60

// Game state enum
typedef enum {
    GAME_STATE_INTRO,
    GAME_STATE_PLAYING,
    GAME_STATE_GAMEOVER
} game_state_t;

// Obstacle struct
typedef struct {
    int16_t x;
    int16_t y;
    bool active;
} obstacle_t;

// Car state (physics-based)
static car_state_t g_car;

// Handle angle for UI (-45 ~ +45 degrees)
static int16_t g_handle_angle = 0;
#define HANDLE_ANGLE_MAX 45
#define HANDLE_ANGLE_RETURN_SPEED 5

// Game state
static game_state_t g_game_state = GAME_STATE_INTRO;

// Current map type (for obstacle logic)
static map_type_t g_current_map_type = MAP_EASY;

// Obstacle (easy map only)
static obstacle_t g_obstacle = {0, 0, false};

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\nShutdown signal received...\n");
}

// AABB collision detection
bool check_collision_aabb(int16_t x1, int16_t y1, int16_t w1, int16_t h1,
                          int16_t x2, int16_t y2, int16_t w2, int16_t h2) {
    int16_t half_w1 = w1 / 2, half_h1 = h1 / 2;
    int16_t half_w2 = w2 / 2, half_h2 = h2 / 2;
    return (x1 - half_w1 < x2 + half_w2 && x1 + half_w1 > x2 - half_w2 &&
            y1 - half_h1 < y2 + half_h2 && y1 + half_h1 > y2 - half_h2);
}

// Check collision with obstacle
bool check_obstacle_collision(void) {
    if (!g_obstacle.active) return false;
    int16_t car_x = car_get_screen_x(&g_car);
    int16_t car_y = car_get_screen_y(&g_car);
    return check_collision_aabb(car_x, car_y, CAR_HITBOX_WIDTH, CAR_HITBOX_HEIGHT,
                                g_obstacle.x, g_obstacle.y,
                                OBSTACLE_HITBOX_WIDTH, OBSTACLE_HITBOX_HEIGHT);
}

// Show game over screen
void show_game_over_screen(void) {
    fb_draw_bitmap(0, 0, &game_over_240x240_bitmap);
    fb_flush();
    printf("GAME OVER! Press any button to restart.\n");
}

// Wait for any key press
bool wait_for_any_key(void) {
    if (button_read_raw(BTN_A) == BUTTON_PRESSED ||
        button_read_raw(BTN_B) == BUTTON_PRESSED) {
        return true;
    }
    joystick_state_t joy = joystick_read_state();
    return (joy.up || joy.down || joy.left || joy.right);
}

// Restart game to intro
void restart_game(void) {
    bcm2835_delay(200);  // Debounce
    g_game_state = GAME_STATE_INTRO;
    g_obstacle.active = false;
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
    g_current_map_type = map;
    if (map == MAP_EASY) {
        g_current_map_bitmap = &easy_map_240x240_bitmap;
        // Easy map: activate obstacle at top-right
        g_obstacle.x = OBSTACLE_EASY_X;
        g_obstacle.y = OBSTACLE_EASY_Y;
        g_obstacle.active = true;
        printf("Selected: Easy Map (with obstacle)\n");
    } else {
        g_current_map_bitmap = &hard_map_240x240_bitmap;
        // Hard map: no obstacle
        g_obstacle.active = false;
        printf("Selected: Hard Map (no obstacle)\n");
    }
}

void draw_game(void) {
    // Draw background map (covers entire screen)
    fb_draw_bitmap(0, 0, g_current_map_bitmap);

    // Draw obstacle (if active)
    if (g_obstacle.active) {
        fb_draw_bitmap_rotated(g_obstacle.x, g_obstacle.y,
                               &obstacle_100x100_bitmap, 0, TRANSPARENT_COLOR);
    }

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

    // Check obstacle collision
    if (g_game_state == GAME_STATE_PLAYING && check_obstacle_collision()) {
        printf("Collision detected!\n");
        g_game_state = GAME_STATE_GAMEOVER;
        return;
    }

    // Draw game
    draw_game();
}

void run_interactive_demo(void) {
    while (g_running) {
        switch (g_game_state) {
            case GAME_STATE_INTRO: {
                // Show intro screen
                printf("\n=== RaspberryParking ===\n");
                printf("Press A for Easy Map, B for Hard Map\n");
                show_intro_screen();

                // Wait for map selection
                map_type_t selected_map = wait_for_map_selection();
                if (!g_running) break;  // Check for shutdown signal

                set_current_map(selected_map);

                // Game instructions
                printf("\n=== Game Controls ===\n");
                printf("A button: Accelerate forward\n");
                printf("B button: Accelerate backward (reverse)\n");
                printf("Joystick left/right: Steer\n");
                printf("Joystick down: Brake\n");
                printf("Press Ctrl+C to exit\n\n");

                // Initialize car at bottom-right position, facing up (0 degrees)
                int16_t start_x = ST7789_WIDTH - CAR_WIDTH / 2 - 10;
                int16_t start_y = ST7789_HEIGHT - CAR_HEIGHT / 2 - 10;
                car_physics_init(&g_car, start_x, start_y, 0);
                g_handle_angle = 0;

                // Start playing
                g_game_state = GAME_STATE_PLAYING;
                draw_game();
                break;
            }

            case GAME_STATE_PLAYING:
                update_game();
                bcm2835_delay(10);  // ~100 FPS
                break;

            case GAME_STATE_GAMEOVER:
                show_game_over_screen();

                // Wait for any key to restart
                while (g_running && !wait_for_any_key()) {
                    bcm2835_delay(50);
                }

                if (g_running) {
                    restart_game();
                }
                break;
        }
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
