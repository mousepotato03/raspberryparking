#include "hard_map.h"
#include "../../assets/hard_map.h"

#define HARD_START_X 35
#define HARD_START_Y 180
#define HARD_GOAL_X  200
#define HARD_GOAL_Y  30
#define HARD_GOAL_WIDTH  20
#define HARD_GOAL_HEIGHT 20

static const map_config_t s_hard_map = {
    .start_x = HARD_START_X,
    .start_y = HARD_START_Y,
    .goal_x = HARD_GOAL_X,
    .goal_y = HARD_GOAL_Y,
    .goal_width = HARD_GOAL_WIDTH,
    .goal_height = HARD_GOAL_HEIGHT,
    .map_bitmap = &hard_map_240x240_bitmap,
    .obstacles = {
        {30, 50, 0, true},
        {75, 50, 0, true},
        {75, 180, 0, true},
        {205, 75, 90, true},
        {145, 125, 60, true},
        {205, 155, 90, true},
        {205, 205, 90, true},
        {0, 0, 0, false}
    },
    .obstacle_count = 7
};

const map_config_t* get_hard_map_config(void) {
    return &s_hard_map;
}
