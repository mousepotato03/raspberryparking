#include <stdio.h>
#include <signal.h>
#include <bcm2835.h>
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "lcd/framebuffer.h"
#include "input/button.h"
#include "input/joystick.h"
#include "game/car_physics.h"
#include "game/collision.h"
#include "../assets/images.h"
#include "../assets/car.h"
#include "../assets/handle.h"
#include "../assets/easy_map.h"
#include "../assets/intro.h"
#include "../assets/hard_map.h"
#include "../assets/obstacle.h"
#include "../assets/game_over.h"
#include "../assets/complete.h"

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
#define CAR_HITBOX_WIDTH  25
#define CAR_HITBOX_HEIGHT 45  // 55 -> 45: 상하 경계 충돌 마진 확보

// Handle bitmap size and position constants
#define HANDLE_WIDTH  80
#define HANDLE_HEIGHT 80
#define HANDLE_X      (HANDLE_WIDTH / 2)              // Handle center X (left side)
#define HANDLE_Y      (ST7789_HEIGHT - HANDLE_HEIGHT / 2)  // Handle center Y (bottom)

// Transparent color for bitmaps
#define TRANSPARENT_COLOR 0x0000

// Debug hitbox colors
#define DEBUG_COLOR_PLAYER   0x001F  // Blue
#define DEBUG_COLOR_OBSTACLE 0xF800  // Red
#define DEBUG_COLOR_GOAL     0x07E0  // Green

// Obstacle constants
#define OBSTACLE_WIDTH  75
#define OBSTACLE_HEIGHT 75
#define OBSTACLE_HITBOX_WIDTH  35
#define OBSTACLE_HITBOX_HEIGHT 55
#define MAX_OBSTACLES 8

// Goal constants (Easy map)
#define GOAL_X 25
#define GOAL_Y 55
#define GOAL_TOLERANCE 10  // 골인 허용 오차 (px)
#define GOAL_SUCCESS_DELAY 5000  // 5초 (ms)

// Start position (Easy map - blue border)
#define START_EASY_X 205
#define START_EASY_Y 165

// Hard map start position (blue border - left bottom)
#define START_HARD_X 35
#define START_HARD_Y 180

// Hard map goal position (red border - right top)
#define GOAL_HARD_X 200
#define GOAL_HARD_Y 30
#define GOAL_HARD_WIDTH 20
#define GOAL_HARD_HEIGHT 20

// Game state enum
typedef enum {
    GAME_STATE_INTRO,
    GAME_STATE_PLAYING,
    GAME_STATE_GAMEOVER,
    GAME_STATE_GOAL_SUCCESS
} game_state_t;

// Obstacle struct
typedef struct {
    int16_t x;
    int16_t y;
    int16_t angle;  // 회전 각도 (0: 세로, 90: 가로)
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

// Obstacles array (easy map)
static obstacle_t g_obstacles[MAX_OBSTACLES];
static int g_obstacle_count = 0;

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

// Check collision with any obstacle (OBB vs AABB)
bool check_obstacle_collision(void) {
    int16_t car_x = car_get_screen_x(&g_car);
    int16_t car_y = car_get_screen_y(&g_car);

    // Player OBB (rotated hitbox)
    obb_t player_obb = {
        .cx = car_x,
        .cy = car_y,
        .half_w = CAR_HITBOX_WIDTH / 2,
        .half_h = CAR_HITBOX_HEIGHT / 2,
        .angle = g_car.angle
    };

    for (int i = 0; i < g_obstacle_count; i++) {
        if (!g_obstacles[i].active) continue;

        // 90도 회전 시 width와 height 교환
        int16_t half_w = (g_obstacles[i].angle == 90) ?
            OBSTACLE_HITBOX_HEIGHT / 2 : OBSTACLE_HITBOX_WIDTH / 2;
        int16_t half_h = (g_obstacles[i].angle == 90) ?
            OBSTACLE_HITBOX_WIDTH / 2 : OBSTACLE_HITBOX_HEIGHT / 2;

        // Obstacle AABB (non-rotated, but dimensions swapped if rotated 90)
        aabb_t obstacle_aabb = {
            .cx = g_obstacles[i].x,
            .cy = g_obstacles[i].y,
            .half_w = half_w,
            .half_h = half_h
        };

        if (check_collision_obb_aabb(&player_obb, &obstacle_aabb)) {
            return true;
        }
    }
    return false;
}

// Check if car reached the goal (player must fully cover the goal area)
bool check_goal_reached(void) {
    int16_t car_x = car_get_screen_x(&g_car);
    int16_t car_y = car_get_screen_y(&g_car);

    // Player hitbox bounds (AABB)
    int16_t player_half_w = CAR_HITBOX_WIDTH / 2;
    int16_t player_half_h = CAR_HITBOX_HEIGHT / 2;
    int16_t player_left = car_x - player_half_w;
    int16_t player_right = car_x + player_half_w;
    int16_t player_top = car_y - player_half_h;
    int16_t player_bottom = car_y + player_half_h;

    // Goal area bounds
    int16_t goal_left, goal_right, goal_top, goal_bottom;
    if (g_current_map_type == MAP_EASY) {
        goal_left = GOAL_X - GOAL_TOLERANCE;
        goal_right = GOAL_X + GOAL_TOLERANCE;
        goal_top = GOAL_Y - GOAL_TOLERANCE;
        goal_bottom = GOAL_Y + GOAL_TOLERANCE;
    } else {
        goal_left = GOAL_HARD_X - GOAL_HARD_WIDTH / 2;
        goal_right = GOAL_HARD_X + GOAL_HARD_WIDTH / 2;
        goal_top = GOAL_HARD_Y - GOAL_HARD_HEIGHT / 2;
        goal_bottom = GOAL_HARD_Y + GOAL_HARD_HEIGHT / 2;
    }

    // Check if player fully contains the goal area
    return (player_left <= goal_left && player_right >= goal_right &&
            player_top <= goal_top && player_bottom >= goal_bottom);
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
    g_obstacle_count = 0;
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
        // Easy map: 8 obstacles
        // 윗줄 4개 (골인지점 제외): X = 65, 105, 145, 185, Y = 55
        // 아랫줄 4개 (시작지점 제외): X = 25, 65, 105, 145, Y = 165
        g_obstacle_count = 8;
        // 윗줄 장애물
        g_obstacles[0] = (obstacle_t){85, 55, 0, true};
        g_obstacles[1] = (obstacle_t){125, 55, 0, true};
        g_obstacles[2] = (obstacle_t){165, 55, 0, true};
        g_obstacles[3] = (obstacle_t){205, 55, 0, true};
        // 아랫줄 장애물
        g_obstacles[4] = (obstacle_t){45, 165, 0, true};
        g_obstacles[5] = (obstacle_t){85, 165, 0, true};
        g_obstacles[6] = (obstacle_t){125, 165, 0, true};
        g_obstacles[7] = (obstacle_t){165, 165, 0, true};
        printf("Selected: Easy Map (with 8 obstacles)\n");
    } else {
        g_current_map_bitmap = &hard_map_240x240_bitmap;
        // Hard map: 7 obstacles in parking spaces
        // 좌상단 세로 주차칸 2개, 좌하단 시작지점 위 1개, 우측 가로 주차칸 4개
        // 시작 지점(파란색)과 골인 지점(빨간색)은 제외
        g_obstacle_count = 7;
        // 좌상단 세로 주차칸 (2개) - 세로 방향
        g_obstacles[0] = (obstacle_t){30, 40, 0, true};
        g_obstacles[1] = (obstacle_t){30, 95, 0, true};
        // 좌하단 시작지점 위 주차칸 (1개) - 세로 방향
        g_obstacles[2] = (obstacle_t){35, 135, 0, true};
        // 우측 가로 주차라인 (4개) - 가로 방향 (90도 회전)
        g_obstacles[3] = (obstacle_t){95, 205, 90, true};
        g_obstacles[4] = (obstacle_t){135, 205, 90, true};
        g_obstacles[5] = (obstacle_t){175, 205, 90, true};
        g_obstacles[6] = (obstacle_t){215, 205, 90, true};
        printf("Selected: Hard Map (with 7 obstacles)\n");
    }
}

void draw_game(void) {
    // Draw background map (covers entire screen)
    fb_draw_bitmap(0, 0, g_current_map_bitmap);

    // Draw all obstacles
    for (int i = 0; i < g_obstacle_count; i++) {
        if (g_obstacles[i].active) {
            fb_draw_bitmap_rotated(g_obstacles[i].x, g_obstacles[i].y,
                                   &obstacle_75x75_bitmap, g_obstacles[i].angle, TRANSPARENT_COLOR);
        }
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

    // === Debug: Draw hitbox outlines ===
    // 1. Player hitbox (Blue) - Rotated OBB
    fb_draw_rotated_rect_outline(car_cx, car_cy,
                                  CAR_HITBOX_WIDTH / 2, CAR_HITBOX_HEIGHT / 2,
                                  g_car.angle, DEBUG_COLOR_PLAYER);

    // 2. Obstacle hitboxes (Red)
    for (int i = 0; i < g_obstacle_count; i++) {
        if (g_obstacles[i].active) {
            // 90도 회전 시 width와 height 교환
            int16_t obs_w = (g_obstacles[i].angle == 90) ?
                OBSTACLE_HITBOX_HEIGHT : OBSTACLE_HITBOX_WIDTH;
            int16_t obs_h = (g_obstacles[i].angle == 90) ?
                OBSTACLE_HITBOX_WIDTH : OBSTACLE_HITBOX_HEIGHT;
            fb_draw_rect_outline(g_obstacles[i].x, g_obstacles[i].y,
                                 obs_w, obs_h, DEBUG_COLOR_OBSTACLE);
        }
    }

    // 3. Goal area (Green)
    if (g_current_map_type == MAP_EASY) {
        fb_draw_rect_outline(GOAL_X, GOAL_Y, GOAL_TOLERANCE * 2, GOAL_TOLERANCE * 2, DEBUG_COLOR_GOAL);
    } else {
        fb_draw_rect_outline(GOAL_HARD_X, GOAL_HARD_Y, GOAL_HARD_WIDTH, GOAL_HARD_HEIGHT, DEBUG_COLOR_GOAL);
    }

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

    // Check goal reached (Easy map only)
    if (g_game_state == GAME_STATE_PLAYING && check_goal_reached()) {
        printf("Goal reached!\n");
        g_game_state = GAME_STATE_GOAL_SUCCESS;
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

                // Initialize car at start position based on map type
                int16_t start_x, start_y;
                if (g_current_map_type == MAP_EASY) {
                    // Easy map: start at blue border (bottom-right)
                    start_x = START_EASY_X;
                    start_y = START_EASY_Y;
                } else {
                    // Hard map: start at blue border (left-bottom)
                    start_x = START_HARD_X;
                    start_y = START_HARD_Y;
                }
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

            case GAME_STATE_GOAL_SUCCESS: {
                draw_game();  // Show final position

                if (g_current_map_type == MAP_EASY) {
                    // Easy map clear: switch to Hard map
                    printf("Switching to Hard Map in 5 seconds...\n");
                    bcm2835_delay(GOAL_SUCCESS_DELAY);

                    set_current_map(MAP_HARD);

                    // Initialize car at Hard map start position (blue border)
                    car_physics_init(&g_car, START_HARD_X, START_HARD_Y, 0);
                    g_handle_angle = 0;

                    g_game_state = GAME_STATE_PLAYING;
                    draw_game();
                } else {
                    // Hard map clear: show complete screen and return to intro
                    printf("SUCCESS! Returning to intro in 5 seconds...\n");
                    fb_draw_bitmap(0, 0, &complete_240x240_bitmap);
                    fb_flush();
                    bcm2835_delay(GOAL_SUCCESS_DELAY);

                    g_game_state = GAME_STATE_INTRO;
                    g_obstacle_count = 0;
                }
                break;
            }
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
