// PLACEHOLDER - 라즈베리파이에서 png2c.py로 다시 생성해야 함
// python3 png2c.py image/obstacle_100x100.png
#include <stdint.h>
#include "obstacle.h"

// 100x100, RGB565 format (placeholder - 빈 이미지)
static const uint16_t obstacle_100x100_pixels[10000] = {0};

const bitmap obstacle_100x100_bitmap = { 100, 100, (uint16_t*)obstacle_100x100_pixels };
