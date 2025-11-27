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

// Helper function to draw 8 symmetric points of a circle
static void draw_circle_points(int16_t x0, int16_t y0, int16_t x, int16_t y, uint16_t color) {
    // Draw 8 symmetric points
    if (x0 + x >= 0 && x0 + x < ST7789_WIDTH && y0 + y >= 0 && y0 + y < ST7789_HEIGHT)
        framebuffer[y0 + y][x0 + x] = color;
    if (x0 - x >= 0 && x0 - x < ST7789_WIDTH && y0 + y >= 0 && y0 + y < ST7789_HEIGHT)
        framebuffer[y0 + y][x0 - x] = color;
    if (x0 + x >= 0 && x0 + x < ST7789_WIDTH && y0 - y >= 0 && y0 - y < ST7789_HEIGHT)
        framebuffer[y0 - y][x0 + x] = color;
    if (x0 - x >= 0 && x0 - x < ST7789_WIDTH && y0 - y >= 0 && y0 - y < ST7789_HEIGHT)
        framebuffer[y0 - y][x0 - x] = color;
    if (x0 + y >= 0 && x0 + y < ST7789_WIDTH && y0 + x >= 0 && y0 + x < ST7789_HEIGHT)
        framebuffer[y0 + x][x0 + y] = color;
    if (x0 - y >= 0 && x0 - y < ST7789_WIDTH && y0 + x >= 0 && y0 + x < ST7789_HEIGHT)
        framebuffer[y0 + x][x0 - y] = color;
    if (x0 + y >= 0 && x0 + y < ST7789_WIDTH && y0 - x >= 0 && y0 - x < ST7789_HEIGHT)
        framebuffer[y0 - x][x0 + y] = color;
    if (x0 - y >= 0 && x0 - y < ST7789_WIDTH && y0 - x >= 0 && y0 - x < ST7789_HEIGHT)
        framebuffer[y0 - x][x0 - y] = color;
}

void fb_draw_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    if (radius < 0) return;

    // Midpoint circle algorithm
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        draw_circle_points(x0, y0, x, y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }

        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fb_fill_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) {
    if (radius < 0) return;

    // Draw filled circle by drawing horizontal lines
    for (int16_t y = -radius; y <= radius; y++) {
        for (int16_t x = -radius; x <= radius; x++) {
            // Check if point is inside circle using distance formula
            if (x * x + y * y <= radius * radius) {
                int16_t px = x0 + x;
                int16_t py = y0 + y;

                // Boundary check
                if (px >= 0 && px < ST7789_WIDTH && py >= 0 && py < ST7789_HEIGHT) {
                    framebuffer[py][px] = color;
                }
            }
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
