#include "easy_map.h"
#include "../../assets/easy_map.h"

#define EASY_START_X 198
#define EASY_START_Y 185
#define EASY_GOAL_X  40
#define EASY_GOAL_Y  50
#define EASY_GOAL_TOLERANCE 10

static const map_config_t s_easy_map = {
    .start_x = EASY_START_X,
    .start_y = EASY_START_Y,
    .goal_x = EASY_GOAL_X,
    .goal_y = EASY_GOAL_Y,
    .goal_width = EASY_GOAL_TOLERANCE * 2,
    .goal_height = EASY_GOAL_TOLERANCE * 2,
    .map_bitmap = &easy_map_240x240_bitmap,
    .obstacles = {
        {85, 55, 0, true},
        {125, 65, 0, true},
        {165, 55, 0, true},
        {205, 55, 0, true},
        {45, 165, 0, true},
        {85, 165, 0, true},
        {125, 165, 0, true},
        {165, 155, 0, true}
    },
    .obstacle_count = 8
};

const map_config_t* get_easy_map_config(void) {
    return &s_easy_map;
}
