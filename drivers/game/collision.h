/**
 * @file collision.h
 * @brief OBB/AABB collision detection system
 *
 * Provides oriented bounding box (OBB) collision detection using
 * the Separating Axis Theorem (SAT) algorithm.
 */

#ifndef COLLISION_H
#define COLLISION_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 2D vector (fixed-point)
 */
typedef struct {
    int32_t x;
    int32_t y;
} vec2_fp_t;

/**
 * @brief Oriented Bounding Box (rotatable rectangle)
 */
typedef struct {
    int16_t cx;      // Center X (screen pixels)
    int16_t cy;      // Center Y (screen pixels)
    int16_t half_w;  // Half width
    int16_t half_h;  // Half height
    int16_t angle;   // Rotation angle (0-359 degrees)
} obb_t;

/**
 * @brief Axis-Aligned Bounding Box (non-rotated rectangle)
 */
typedef struct {
    int16_t cx;      // Center X
    int16_t cy;      // Center Y
    int16_t half_w;  // Half width
    int16_t half_h;  // Half height
} aabb_t;

/**
 * @brief Calculate 4 vertices of an OBB
 * @param obb OBB structure
 * @param vertices Output: 4 vertices array (fixed-point coordinates)
 */
void obb_get_vertices(const obb_t* obb, vec2_fp_t vertices[4]);

/**
 * @brief Check collision between OBB and AABB using SAT algorithm
 * @param obb Rotated bounding box (player)
 * @param aabb Axis-aligned bounding box (obstacle)
 * @return true if collision detected
 */
bool check_collision_obb_aabb(const obb_t* obb, const aabb_t* aabb);

#endif // COLLISION_H
