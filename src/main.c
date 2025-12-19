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
#include "maps/map_types.h"
#include "maps/easy_map.h"
#include "maps/hard_map.h"
#include "../assets/images.h"
#include "../assets/car.h"
#include "../assets/handle.h"
#include "../assets/intro.h"
#include "../assets/obstacle.h"
#include "../assets/game_over.h"
#include "../assets/complete.h"

// Global flag for graceful shutdown
static volatile int g_running = 1;

// Current map config pointer
static const map_config_t* g_current_map = NULL;

// Car hitbox size (actual car bounds within bitmap)
#define CAR_HITBOX_WIDTH  25
#define CAR_HITBOX_HEIGHT 45 

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

// Obstacle hitbox constants
#define OBSTACLE_HITBOX_WIDTH  35
#define OBSTACLE_HITBOX_HEIGHT 55

// Timing constants
#define GOAL_SUCCESS_DELAY 5000  // 5초 (ms)

// Game state enum
typedef enum {
    GAME_STATE_INTRO,
    GAME_STATE_PLAYING,
    GAME_STATE_GAMEOVER,
    GAME_STATE_GOAL_SUCCESS
} game_state_t;

// Car state (physics-based)
static car_state_t g_car;

// Handle angle for UI (-45 ~ +45 degrees)
static int16_t g_handle_angle = 0;
#define HANDLE_ANGLE_MAX 45
#define HANDLE_ANGLE_RETURN_SPEED 5

// Timing constants (ms)
#define DEBOUNCE_DELAY_MS      200
#define FRAME_DELAY_MS         10
#define MAP_SELECTION_DELAY_MS 10
#define KEY_WAIT_DELAY_MS      50

// Game state
static game_state_t g_game_state = GAME_STATE_INTRO;

void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    printf("\nShutdown signal received...\n");
}

// Check collision with any obstacle (OBB vs AABB)
bool check_obstacle_collision(void) {
    if (!g_current_map) return false;

    int16_t car_x = car_get_screen_x(&g_car);
    int16_t car_y = car_get_screen_y(&g_car);

    obb_t player_obb = {
        .cx = car_x,
        .cy = car_y,
        .half_w = CAR_HITBOX_WIDTH / 2,
        .half_h = CAR_HITBOX_HEIGHT / 2,
        .angle = g_car.angle
    };

    const obstacle_t* obstacles = g_current_map->obstacles;
    int count = g_current_map->obstacle_count;

    for (int i = 0; i < count; i++) {
        if (!obstacles[i].active) continue;

        int16_t half_w = (obstacles[i].angle == 90) ?
            OBSTACLE_HITBOX_HEIGHT / 2 : OBSTACLE_HITBOX_WIDTH / 2;
        int16_t half_h = (obstacles[i].angle == 90) ?
            OBSTACLE_HITBOX_WIDTH / 2 : OBSTACLE_HITBOX_HEIGHT / 2;

        aabb_t obstacle_aabb = {
            .cx = obstacles[i].x,
            .cy = obstacles[i].y,
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
    if (!g_current_map) return false;

    aabb_t player = {
        .cx = car_get_screen_x(&g_car),
        .cy = car_get_screen_y(&g_car),
        .half_w = CAR_HITBOX_WIDTH / 2,
        .half_h = CAR_HITBOX_HEIGHT / 2
    };

    aabb_t goal = {
        .cx = g_current_map->goal_x,
        .cy = g_current_map->goal_y,
        .half_w = g_current_map->goal_width / 2,
        .half_h = g_current_map->goal_height / 2
    };

    return (player.cx - player.half_w <= goal.cx - goal.half_w &&
            player.cx + player.half_w >= goal.cx + goal.half_w &&
            player.cy - player.half_h <= goal.cy - goal.half_h &&
            player.cy + player.half_h >= goal.cy + goal.half_h);
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
    bcm2835_delay(DEBOUNCE_DELAY_MS);
    g_game_state = GAME_STATE_INTRO;
    g_current_map = NULL;
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
        bcm2835_delay(MAP_SELECTION_DELAY_MS);
    }
    return MAP_EASY;  // Default if interrupted
}

void set_current_map(map_type_t map) {
    g_current_map = (map == MAP_EASY) ?
        get_easy_map_config() : get_hard_map_config();
    printf("Selected: %s Map (with %d obstacles)\n",
           (map == MAP_EASY) ? "Easy" : "Hard",
           g_current_map->obstacle_count);
}

// Draw debug hitboxes for player, obstacles, and goal
static void draw_debug_hitboxes(int16_t car_cx, int16_t car_cy) {
    // Player hitbox (Blue)
    fb_draw_rotated_rect_outline(car_cx, car_cy,
                                  CAR_HITBOX_WIDTH / 2, CAR_HITBOX_HEIGHT / 2,
                                  g_car.angle, DEBUG_COLOR_PLAYER);

    // Obstacle hitboxes (Red)
    const obstacle_t* obstacles = g_current_map->obstacles;
    int count = g_current_map->obstacle_count;
    for (int i = 0; i < count; i++) {
        if (obstacles[i].active) {
            int16_t obs_w = (obstacles[i].angle == 90) ?
                OBSTACLE_HITBOX_HEIGHT : OBSTACLE_HITBOX_WIDTH;
            int16_t obs_h = (obstacles[i].angle == 90) ?
                OBSTACLE_HITBOX_WIDTH : OBSTACLE_HITBOX_HEIGHT;
            fb_draw_rect_outline(obstacles[i].x, obstacles[i].y,
                                 obs_w, obs_h, DEBUG_COLOR_OBSTACLE);
        }
    }

    // Goal area (Green)
    fb_draw_rect_outline(g_current_map->goal_x, g_current_map->goal_y,
                         g_current_map->goal_width, g_current_map->goal_height,
                         DEBUG_COLOR_GOAL);
}

void draw_game(void) {
    if (!g_current_map) return;

    // Draw background map
    fb_draw_bitmap(0, 0, g_current_map->map_bitmap);

    // Draw all obstacles
    const obstacle_t* obstacles = g_current_map->obstacles;
    int count = g_current_map->obstacle_count;
    for (int i = 0; i < count; i++) {
        if (obstacles[i].active) {
            fb_draw_bitmap_rotated(obstacles[i].x, obstacles[i].y,
                                   &obstacle_75x75_bitmap, obstacles[i].angle, TRANSPARENT_COLOR);
        }
    }

    // Draw car
    int16_t car_cx = car_get_screen_x(&g_car);
    int16_t car_cy = car_get_screen_y(&g_car);
    fb_draw_bitmap_rotated(car_cx, car_cy, &car_100x100_bitmap,
                           g_car.angle, TRANSPARENT_COLOR);

    // Draw handle
    fb_draw_bitmap_rotated(HANDLE_X, HANDLE_Y, &handle_80x80_bitmap,
                           g_handle_angle, TRANSPARENT_COLOR);

#ifdef DEBUG
    // Debug: Draw hitbox outlines
    draw_debug_hitboxes(car_cx, car_cy);
#endif

    fb_flush();
}

// Return handle to center gradually
static void update_handle_return(void) {
    if (g_handle_angle > 0) {
        g_handle_angle -= HANDLE_ANGLE_RETURN_SPEED;
        if (g_handle_angle < 0) g_handle_angle = 0;
    } else if (g_handle_angle < 0) {
        g_handle_angle += HANDLE_ANGLE_RETURN_SPEED;
        if (g_handle_angle > 0) g_handle_angle = 0;
    }
}

void process_input(void) {
    joystick_state_t joy = joystick_read_state();

    // Acceleration
    if (button_read_raw(BTN_A) == BUTTON_PRESSED) {
        car_apply_acceleration(&g_car, &default_car_params, true);
    } else if (button_read_raw(BTN_B) == BUTTON_PRESSED) {
        car_apply_acceleration(&g_car, &default_car_params, false);
    }

    // Brake
    if (joy.down) {
        car_apply_brake(&g_car, &default_car_params);
    }

    // Steering
    if (joy.left) {
        car_apply_turn(&g_car, &default_car_params, -1);
        g_handle_angle = -HANDLE_ANGLE_MAX;
    } else if (joy.right) {
        car_apply_turn(&g_car, &default_car_params, +1);
        g_handle_angle = HANDLE_ANGLE_MAX;
    } else {
        update_handle_return();
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

// State handler: INTRO
static void handle_state_intro(void) {
    printf("\n=== RaspberryParking ===\n");
    printf("Press A for Easy Map, B for Hard Map\n");
    show_intro_screen();

    map_type_t selected_map = wait_for_map_selection();
    if (!g_running) return;

    set_current_map(selected_map);

    printf("\n=== Game Controls ===\n");
    printf("A button: Accelerate forward\n");
    printf("B button: Accelerate backward (reverse)\n");
    printf("Joystick left/right: Steer\n");
    printf("Joystick down: Brake\n");
    printf("Press Ctrl+C to exit\n\n");

    car_physics_init(&g_car, g_current_map->start_x, g_current_map->start_y, 0);
    g_handle_angle = 0;

    g_game_state = GAME_STATE_PLAYING;
    draw_game();
}

// State handler: PLAYING
static void handle_state_playing(void) {
    update_game();
    bcm2835_delay(FRAME_DELAY_MS);
}

// State handler: GAMEOVER
static void handle_state_gameover(void) {
    show_game_over_screen();

    while (g_running && !wait_for_any_key()) {
        bcm2835_delay(KEY_WAIT_DELAY_MS);
    }

    if (g_running) {
        restart_game();
    }
}

// State handler: GOAL_SUCCESS
static void handle_state_goal_success(void) {
    draw_game();

    bool is_easy = (g_current_map == get_easy_map_config());
    if (is_easy) {
        printf("Switching to Hard Map in 5 seconds...\n");
        bcm2835_delay(GOAL_SUCCESS_DELAY);

        set_current_map(MAP_HARD);
        car_physics_init(&g_car, g_current_map->start_x, g_current_map->start_y, 0);
        g_handle_angle = 0;

        g_game_state = GAME_STATE_PLAYING;
        draw_game();
    } else {
        printf("SUCCESS! Returning to intro in 5 seconds...\n");
        fb_draw_bitmap(0, 0, &complete_240x240_bitmap);
        fb_flush();
        bcm2835_delay(GOAL_SUCCESS_DELAY);

        g_game_state = GAME_STATE_INTRO;
        g_current_map = NULL;
    }
}

void run_interactive_demo(void) {
    while (g_running) {
        switch (g_game_state) {
            case GAME_STATE_INTRO:
                handle_state_intro();
                break;
            case GAME_STATE_PLAYING:
                handle_state_playing();
                break;
            case GAME_STATE_GAMEOVER:
                handle_state_gameover();
                break;
            case GAME_STATE_GOAL_SUCCESS:
                handle_state_goal_success();
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
