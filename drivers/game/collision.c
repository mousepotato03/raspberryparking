/**
 * @file collision.c
 * @brief OBB/AABB collision detection implementation using SAT algorithm
 */

#include "collision.h"
#include "sin_table.h"

// Fixed-point scale (same as sin_table.h)
#define COLLISION_FP_SHIFT 10
#define COLLISION_FP_SCALE (1 << COLLISION_FP_SHIFT)  // 1024

/**
 * @brief Fixed-point dot product
 */
static int32_t vec2_dot(const vec2_fp_t* a, const vec2_fp_t* b) {
    int64_t result = (int64_t)a->x * b->x + (int64_t)a->y * b->y;
    return (int32_t)(result >> COLLISION_FP_SHIFT);
}

/**
 * @brief Project vertices onto an axis and get min/max range
 */
static void project_vertices(const vec2_fp_t vertices[4],
                             const vec2_fp_t* axis,
                             int32_t* out_min, int32_t* out_max) {
    int32_t projection = vec2_dot(&vertices[0], axis);
    *out_min = projection;
    *out_max = projection;

    for (int i = 1; i < 4; i++) {
        projection = vec2_dot(&vertices[i], axis);
        if (projection < *out_min) *out_min = projection;
        if (projection > *out_max) *out_max = projection;
    }
}

/**
 * @brief Check if two ranges overlap
 */
static bool ranges_overlap(int32_t min1, int32_t max1,
                           int32_t min2, int32_t max2) {
    return !(max1 < min2 || max2 < min1);
}

void obb_get_vertices(const obb_t* obb, vec2_fp_t vertices[4]) {
    int16_t sin_a = get_sin(obb->angle);
    int16_t cos_a = get_cos(obb->angle);

    // Local coordinates (relative to center)
    // Order: top-left, top-right, bottom-right, bottom-left
    int16_t local_x[4] = {-obb->half_w, +obb->half_w, +obb->half_w, -obb->half_w};
    int16_t local_y[4] = {-obb->half_h, -obb->half_h, +obb->half_h, +obb->half_h};

    for (int i = 0; i < 4; i++) {
        // Rotation transform:
        // x' = x*cos - y*sin
        // y' = x*sin + y*cos
        int32_t rot_x = ((int32_t)local_x[i] * cos_a - (int32_t)local_y[i] * sin_a);
        int32_t rot_y = ((int32_t)local_x[i] * sin_a + (int32_t)local_y[i] * cos_a);

        // Add center coordinates (apply FP_SCALE)
        vertices[i].x = ((int32_t)obb->cx << COLLISION_FP_SHIFT) + rot_x;
        vertices[i].y = ((int32_t)obb->cy << COLLISION_FP_SHIFT) + rot_y;
    }
}

bool check_collision_obb_aabb(const obb_t* obb, const aabb_t* aabb) {
    // 1. Calculate OBB vertices
    vec2_fp_t obb_verts[4];
    obb_get_vertices(obb, obb_verts);

    // 2. Calculate AABB vertices (no rotation)
    vec2_fp_t aabb_verts[4];
    int32_t aabb_cx_fp = (int32_t)aabb->cx << COLLISION_FP_SHIFT;
    int32_t aabb_cy_fp = (int32_t)aabb->cy << COLLISION_FP_SHIFT;
    int32_t aabb_hw_fp = (int32_t)aabb->half_w << COLLISION_FP_SHIFT;
    int32_t aabb_hh_fp = (int32_t)aabb->half_h << COLLISION_FP_SHIFT;

    aabb_verts[0] = (vec2_fp_t){aabb_cx_fp - aabb_hw_fp, aabb_cy_fp - aabb_hh_fp};
    aabb_verts[1] = (vec2_fp_t){aabb_cx_fp + aabb_hw_fp, aabb_cy_fp - aabb_hh_fp};
    aabb_verts[2] = (vec2_fp_t){aabb_cx_fp + aabb_hw_fp, aabb_cy_fp + aabb_hh_fp};
    aabb_verts[3] = (vec2_fp_t){aabb_cx_fp - aabb_hw_fp, aabb_cy_fp + aabb_hh_fp};

    // 3. SAT: Test separation on 4 axes
    // Axes 1,2: AABB axes (X, Y)
    // Axes 3,4: OBB axes (rotated X, Y)

    vec2_fp_t axes[4];

    // AABB axes (fixed)
    axes[0] = (vec2_fp_t){COLLISION_FP_SCALE, 0};  // X axis
    axes[1] = (vec2_fp_t){0, COLLISION_FP_SCALE};  // Y axis

    // OBB axes (rotated)
    int16_t sin_a = get_sin(obb->angle);
    int16_t cos_a = get_cos(obb->angle);
    axes[2] = (vec2_fp_t){cos_a, sin_a};   // Rotated X axis
    axes[3] = (vec2_fp_t){-sin_a, cos_a};  // Rotated Y axis

    // Test each axis for separation
    for (int i = 0; i < 4; i++) {
        int32_t obb_min, obb_max;
        int32_t aabb_min, aabb_max;

        project_vertices(obb_verts, &axes[i], &obb_min, &obb_max);
        project_vertices(aabb_verts, &axes[i], &aabb_min, &aabb_max);

        // If separation found -> no collision
        if (!ranges_overlap(obb_min, obb_max, aabb_min, aabb_max)) {
            return false;
        }
    }

    // All axes overlap -> collision
    return true;
}

bool check_collision_aabb(int16_t x1, int16_t y1, int16_t w1, int16_t h1,
                          int16_t x2, int16_t y2, int16_t w2, int16_t h2) {
    int16_t half_w1 = w1 / 2, half_h1 = h1 / 2;
    int16_t half_w2 = w2 / 2, half_h2 = h2 / 2;
    return (x1 - half_w1 < x2 + half_w2 && x1 + half_w1 > x2 - half_w2 &&
            y1 - half_h1 < y2 + half_h2 && y1 + half_h1 > y2 - half_h2);
}
