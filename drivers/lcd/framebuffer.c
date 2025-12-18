/**
 * @file framebuffer.c
 * @brief Frame buffer implementation
 */

#include "framebuffer.h"
#include <string.h>
#include "../game/sin_table.h"

// Frame buffer: 240x240 pixels, RGB565 format
// Size: 240 * 240 * 2 bytes = 115,200 bytes (~112.5 KB)
static uint16_t framebuffer[ST7789_HEIGHT][ST7789_WIDTH];

void fb_init(void) {
    // Initialize frame buffer to black
    fb_clear(0x0000);
}

void fb_clear(uint16_t color) {
    for (int y = 0; y < ST7789_HEIGHT; y++) {
        for (int x = 0; x < ST7789_WIDTH; x++) {
            framebuffer[y][x] = color;
        }
    }
}

void fb_set_pixel(uint16_t x, uint16_t y, uint16_t color) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }
    framebuffer[y][x] = color;
}

uint16_t fb_get_pixel(uint16_t x, uint16_t y) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return 0x0000;
    }
    return framebuffer[y][x];
}

void fb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // Boundary check
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) {
        return;
    }

    // Clip rectangle to screen boundaries
    uint16_t x1 = x + w;
    uint16_t y1 = y + h;

    if (x1 > ST7789_WIDTH) {
        x1 = ST7789_WIDTH;
    }
    if (y1 > ST7789_HEIGHT) {
        y1 = ST7789_HEIGHT;
    }

    // Draw rectangle to frame buffer
    for (uint16_t py = y; py < y1; py++) {
        for (uint16_t px = x; px < x1; px++) {
            framebuffer[py][px] = color;
        }
    }
}

void fb_draw_rect_outline(int16_t cx, int16_t cy, int16_t w, int16_t h, uint16_t color) {
    // Calculate corners from center
    int16_t x1 = cx - w / 2;
    int16_t y1 = cy - h / 2;
    int16_t x2 = cx + w / 2;
    int16_t y2 = cy + h / 2;

    // Draw horizontal lines (top and bottom)
    for (int16_t x = x1; x <= x2; x++) {
        if (x >= 0 && x < ST7789_WIDTH) {
            if (y1 >= 0 && y1 < ST7789_HEIGHT) {
                framebuffer[y1][x] = color;
            }
            if (y2 >= 0 && y2 < ST7789_HEIGHT) {
                framebuffer[y2][x] = color;
            }
        }
    }

    // Draw vertical lines (left and right)
    for (int16_t y = y1; y <= y2; y++) {
        if (y >= 0 && y < ST7789_HEIGHT) {
            if (x1 >= 0 && x1 < ST7789_WIDTH) {
                framebuffer[y][x1] = color;
            }
            if (x2 >= 0 && x2 < ST7789_WIDTH) {
                framebuffer[y][x2] = color;
            }
        }
    }
}

void fb_draw_bitmap(uint16_t x, uint16_t y, const bitmap* bmp) {
    if (bmp == NULL || bmp->bitmap == NULL) {
        return;
    }

    // Draw bitmap with boundary checking and clipping
    for (uint16_t by = 0; by < bmp->height; by++) {
        for (uint16_t bx = 0; bx < bmp->width; bx++) {
            uint16_t screen_x = x + bx;
            uint16_t screen_y = y + by;

            // Skip pixels outside screen boundaries
            if (screen_x >= ST7789_WIDTH || screen_y >= ST7789_HEIGHT) {
                continue;
            }

            // Copy pixel from bitmap to frame buffer
            uint16_t pixel_color = bmp->bitmap[by * bmp->width + bx];
            framebuffer[screen_y][screen_x] = pixel_color;
        }
    }
}

void fb_flush(void) {
    // Send entire frame buffer to LCD
    // This function will use the st7789_write_framebuffer() function
    st7789_write_framebuffer((uint16_t*)framebuffer, ST7789_WIDTH * ST7789_HEIGHT);
}

uint16_t* fb_get_buffer(void) {
    return (uint16_t*)framebuffer;
}

void fb_draw_bitmap_rotated(int16_t cx, int16_t cy, const bitmap* bmp,
                            int16_t angle, uint16_t transparent_color) {
    if (bmp == NULL || bmp->bitmap == NULL) {
        return;
    }

    // 각도 정규화 (0-359)
    angle = normalize_angle(angle);

    // 특수 각도 최적화: 0도일 때 일반 draw 사용
    if (angle == 0) {
        // 중심 좌표를 좌상단 좌표로 변환
        int16_t x = cx - bmp->width / 2;
        int16_t y = cy - bmp->height / 2;

        // 투명 색상 처리 포함 그리기
        for (uint16_t by = 0; by < bmp->height; by++) {
            for (uint16_t bx = 0; bx < bmp->width; bx++) {
                int16_t screen_x = x + bx;
                int16_t screen_y = y + by;

                if (screen_x < 0 || screen_x >= ST7789_WIDTH ||
                    screen_y < 0 || screen_y >= ST7789_HEIGHT) {
                    continue;
                }

                uint16_t color = bmp->bitmap[by * bmp->width + bx];
                if (color != transparent_color) {
                    framebuffer[screen_y][screen_x] = color;
                }
            }
        }
        return;
    }

    // 비트맵 중심점
    int16_t bmp_cx = bmp->width / 2;
    int16_t bmp_cy = bmp->height / 2;

    // sin/cos 값 가져오기 (고정소수점 스케일 1024)
    int16_t sin_a = get_sin(angle);
    int16_t cos_a = get_cos(angle);

    // 회전된 비트맵의 바운딩 박스 크기 계산
    // 최대 대각선 길이를 사용
    int16_t max_dim = (bmp->width > bmp->height) ? bmp->width : bmp->height;
    int16_t half_diag = (max_dim * 3) / 4 + 1;  // 약간의 여유

    // 목적지 픽셀 순회 (역변환 방식)
    for (int16_t dy = -half_diag; dy <= half_diag; dy++) {
        for (int16_t dx = -half_diag; dx <= half_diag; dx++) {
            // 화면 좌표
            int16_t screen_x = cx + dx;
            int16_t screen_y = cy + dy;

            // 화면 범위 체크
            if (screen_x < 0 || screen_x >= ST7789_WIDTH ||
                screen_y < 0 || screen_y >= ST7789_HEIGHT) {
                continue;
            }

            // 역회전 변환: 화면 좌표 -> 원본 비트맵 좌표
            // 공식: src = R^(-1) * dst
            // R^(-1)은 -angle 회전이므로 sin(-a) = -sin(a), cos(-a) = cos(a)
            int32_t src_x_fp = (int32_t)dx * cos_a + (int32_t)dy * sin_a;
            int32_t src_y_fp = -(int32_t)dx * sin_a + (int32_t)dy * cos_a;

            // 고정소수점 -> 정수 변환 + 비트맵 중심 오프셋
            int16_t src_x = (src_x_fp >> FP_SHIFT) + bmp_cx;
            int16_t src_y = (src_y_fp >> FP_SHIFT) + bmp_cy;

            // 원본 비트맵 범위 체크
            if (src_x < 0 || src_x >= bmp->width ||
                src_y < 0 || src_y >= bmp->height) {
                continue;
            }

            // 픽셀 색상 가져오기
            uint16_t color = bmp->bitmap[src_y * bmp->width + src_x];

            // 투명 색상이면 스킵
            if (color == transparent_color) {
                continue;
            }

            // 프레임버퍼에 그리기
            framebuffer[screen_y][screen_x] = color;
        }
    }
}

/**
 * @brief Draw a line using Bresenham's algorithm
 */
static void fb_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        if (x0 >= 0 && x0 < ST7789_WIDTH && y0 >= 0 && y0 < ST7789_HEIGHT) {
            framebuffer[y0][x0] = color;
        }

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void fb_draw_rotated_rect_outline(int16_t cx, int16_t cy,
                                   int16_t half_w, int16_t half_h,
                                   int16_t angle, uint16_t color) {
    // Get sin/cos values
    int16_t sin_a = get_sin(angle);
    int16_t cos_a = get_cos(angle);

    // Local coordinates for 4 corners
    int16_t local_x[4] = {-half_w, +half_w, +half_w, -half_w};
    int16_t local_y[4] = {-half_h, -half_h, +half_h, +half_h};

    // Calculate rotated screen coordinates
    int16_t screen_x[4], screen_y[4];
    for (int i = 0; i < 4; i++) {
        int32_t rot_x = ((int32_t)local_x[i] * cos_a - (int32_t)local_y[i] * sin_a) >> FP_SHIFT;
        int32_t rot_y = ((int32_t)local_x[i] * sin_a + (int32_t)local_y[i] * cos_a) >> FP_SHIFT;
        screen_x[i] = cx + (int16_t)rot_x;
        screen_y[i] = cy + (int16_t)rot_y;
    }

    // Draw 4 edges
    fb_draw_line(screen_x[0], screen_y[0], screen_x[1], screen_y[1], color);  // top
    fb_draw_line(screen_x[1], screen_y[1], screen_x[2], screen_y[2], color);  // right
    fb_draw_line(screen_x[2], screen_y[2], screen_x[3], screen_y[3], color);  // bottom
    fb_draw_line(screen_x[3], screen_y[3], screen_x[0], screen_y[0], color);  // left
}
