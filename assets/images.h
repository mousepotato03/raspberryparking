#pragma once

#include <stdint.h>

typedef struct bitmap {
    uint16_t width;
    uint16_t height;
    uint16_t* bitmap;
} bitmap;

extern const bitmap clr_circle_bitmap;
extern const bitmap cannon_bitmap;
