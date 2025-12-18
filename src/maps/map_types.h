#ifndef MAP_TYPES_H
#define MAP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "lcd/framebuffer.h"

typedef enum {
    MAP_EASY,
    MAP_HARD
} map_type_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t angle;
    bool active;
} obstacle_t;

#define MAX_OBSTACLES 8

typedef struct {
    int16_t start_x;
    int16_t start_y;
    int16_t goal_x;
    int16_t goal_y;
    int16_t goal_width;
    int16_t goal_height;
    const bitmap* map_bitmap;
    obstacle_t obstacles[MAX_OBSTACLES];
    int obstacle_count;
} map_config_t;

#endif
