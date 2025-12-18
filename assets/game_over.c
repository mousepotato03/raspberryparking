// PLACEHOLDER - 라즈베리파이에서 png2c.py로 다시 생성해야 함
// python3 png2c.py image/game_over_240x240.png
#include <stdint.h>
#include "game_over.h"

// 240x240, RGB565 format (placeholder - 빈 이미지)
static const uint16_t game_over_240x240_pixels[57600] = {0};

const bitmap game_over_240x240_bitmap = { 240, 240, (uint16_t*)game_over_240x240_pixels };
