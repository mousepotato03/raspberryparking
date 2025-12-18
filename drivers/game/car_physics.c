/**
 * car_physics.c
 * 자동차 물리 시뮬레이션 구현
 */

#include "car_physics.h"
#include "sin_table.h"

// 기본 물리 파라미터
const car_physics_params_t default_car_params = {
    .max_speed_forward = 850,    // 약 3.3 픽셀/프레임 (768→850, 약 10% 증가)
    .max_speed_reverse = 425,    // 약 1.7 픽셀/프레임 (384→425, 비례 증가)
    .acceleration_rate = 16,     // 0→최대: 약 53프레임 (0.53초)
    .brake_deceleration = 48,    // 빠른 정지
    .friction = 16,              // 자연 감속 (8→16, 버튼 안 누르면 빨리 정지)
    .turn_rate = 3,              // 3도/프레임
    .min_speed_to_turn = 64      // 회전 가능 최소 속도
};

void car_physics_init(car_state_t* car, int16_t start_x, int16_t start_y, int16_t start_angle) {
    car->pos_x = (int32_t)start_x << CAR_FP_SHIFT;
    car->pos_y = (int32_t)start_y << CAR_FP_SHIFT;
    car->speed = 0;
    car->angle = start_angle;
    car->is_accelerating = false;
    car->is_braking = false;
}

void car_apply_acceleration(car_state_t* car, const car_physics_params_t* params, bool forward) {
    car->is_accelerating = true;

    if (forward) {
        // 전진 가속
        car->speed += params->acceleration_rate;
        if (car->speed > params->max_speed_forward) {
            car->speed = params->max_speed_forward;
        }
    } else {
        // 후진 가속
        car->speed -= params->acceleration_rate;
        if (car->speed < -params->max_speed_reverse) {
            car->speed = -params->max_speed_reverse;
        }
    }
}

void car_apply_brake(car_state_t* car, const car_physics_params_t* params) {
    car->is_braking = true;

    if (car->speed > 0) {
        car->speed -= params->brake_deceleration;
        if (car->speed < 0) {
            car->speed = 0;
        }
    } else if (car->speed < 0) {
        car->speed += params->brake_deceleration;
        if (car->speed > 0) {
            car->speed = 0;
        }
    }
}

// 내부 함수: 마찰 적용
static void apply_friction(car_state_t* car, const car_physics_params_t* params) {
    if (car->is_accelerating || car->is_braking) {
        return;  // 가속 또는 브레이크 중에는 마찰 무시
    }

    if (car->speed > 0) {
        car->speed -= params->friction;
        if (car->speed < 0) {
            car->speed = 0;
        }
    } else if (car->speed < 0) {
        car->speed += params->friction;
        if (car->speed > 0) {
            car->speed = 0;
        }
    }
}

void car_apply_turn(car_state_t* car, const car_physics_params_t* params, int8_t direction) {
    // 속도의 절댓값 계산
    int32_t abs_speed = car->speed;
    if (abs_speed < 0) {
        abs_speed = -abs_speed;
    }

    // 속도가 너무 낮으면 회전 불가
    if (abs_speed < params->min_speed_to_turn) {
        return;
    }

    // 후진 중에는 핸들 방향 반전
    int8_t effective_direction = direction;
    if (car->speed < 0) {
        effective_direction = -direction;
    }

    // 각도 업데이트
    car->angle += effective_direction * params->turn_rate;

    // 각도 정규화 (0-359)
    while (car->angle < 0) {
        car->angle += 360;
    }
    while (car->angle >= 360) {
        car->angle -= 360;
    }
}

// 내부 함수: 위치 업데이트
static void update_position(car_state_t* car) {
    if (car->speed == 0) {
        return;
    }

    // sin/cos 값 가져오기 (FP_SCALE = 1024)
    int16_t sin_val = get_sin(car->angle);
    int16_t cos_val = get_cos(car->angle);

    // 속도 성분 계산
    // velocity_x = speed * sin(angle) (오른쪽이 +x)
    // velocity_y = -speed * cos(angle) (아래쪽이 +y, 화면 좌표계)
    //
    // 스케일 조정: speed는 CAR_FP_SCALE(256), sin/cos는 FP_SCALE(1024)
    // 결과를 다시 CAR_FP_SCALE로 맞추려면 FP_SHIFT(10)만큼 우측 시프트
    int32_t velocity_x = ((int32_t)car->speed * sin_val) >> FP_SHIFT;
    int32_t velocity_y = -((int32_t)car->speed * cos_val) >> FP_SHIFT;

    // 위치 업데이트
    car->pos_x += velocity_x;
    car->pos_y += velocity_y;
}

void car_physics_update(car_state_t* car, const car_physics_params_t* params) {
    // 마찰 적용
    apply_friction(car, params);

    // 위치 업데이트
    update_position(car);

    // 플래그 리셋 (다음 프레임을 위해)
    car->is_accelerating = false;
    car->is_braking = false;
}

void car_clamp_to_screen(car_state_t* car, uint16_t screen_width, uint16_t screen_height,
                         uint16_t car_width, uint16_t car_height) {
    // 자동차 중심 기준으로 경계 계산
    int32_t half_w = ((int32_t)car_width / 2) << CAR_FP_SHIFT;
    int32_t half_h = ((int32_t)car_height / 2) << CAR_FP_SHIFT;

    int32_t min_x = half_w;
    int32_t min_y = half_h;
    int32_t max_x = ((int32_t)screen_width << CAR_FP_SHIFT) - half_w;
    int32_t max_y = ((int32_t)screen_height << CAR_FP_SHIFT) - half_h;

    bool hit_boundary = false;

    if (car->pos_x < min_x) {
        car->pos_x = min_x;
        hit_boundary = true;
    }
    if (car->pos_x > max_x) {
        car->pos_x = max_x;
        hit_boundary = true;
    }
    if (car->pos_y < min_y) {
        car->pos_y = min_y;
        hit_boundary = true;
    }
    if (car->pos_y > max_y) {
        car->pos_y = max_y;
        hit_boundary = true;
    }

    // 경계에 부딪히면 속도 감소
    if (hit_boundary) {
        car->speed = car->speed / 2;
    }
}

int16_t car_get_screen_x(const car_state_t* car) {
    return (int16_t)(car->pos_x >> CAR_FP_SHIFT);
}

int16_t car_get_screen_y(const car_state_t* car) {
    return (int16_t)(car->pos_y >> CAR_FP_SHIFT);
}

bool car_is_moving(const car_state_t* car) {
    return car->speed != 0;
}

bool car_is_reversing(const car_state_t* car) {
    return car->speed < 0;
}
