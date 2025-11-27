/**
 * car_physics.h
 * 자동차 물리 시뮬레이션 시스템
 */

#ifndef CAR_PHYSICS_H
#define CAR_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>

// 고정소수점 스케일 (8비트 소수점)
#define CAR_FP_SHIFT 8
#define CAR_FP_SCALE (1 << CAR_FP_SHIFT)  // 256

/**
 * 자동차 상태 구조체
 */
typedef struct {
    // 위치 (고정소수점: 실제값 = 저장값 >> CAR_FP_SHIFT)
    int32_t pos_x;
    int32_t pos_y;

    // 속도 (고정소수점, 양수=전진, 음수=후진)
    int32_t speed;

    // 방향 (0-359도, 0=위쪽, 시계방향 증가)
    int16_t angle;

    // 상태 플래그 (프레임별 리셋)
    bool is_accelerating;
    bool is_braking;
} car_state_t;

/**
 * 물리 파라미터 구조체 (튜닝 가능)
 */
typedef struct {
    int32_t max_speed_forward;    // 최대 전진 속도
    int32_t max_speed_reverse;    // 최대 후진 속도
    int32_t acceleration_rate;    // 가속도 (프레임당 속도 증가량)
    int32_t brake_deceleration;   // 브레이크 감속도
    int32_t friction;             // 마찰 계수 (자연 감속)
    int16_t turn_rate;            // 회전 속도 (도/프레임)
    int32_t min_speed_to_turn;    // 회전 가능한 최소 속도
} car_physics_params_t;

// 기본 물리 파라미터 (외부에서 사용 가능)
extern const car_physics_params_t default_car_params;

/**
 * 자동차 초기화
 * @param car 자동차 상태 포인터
 * @param start_x 시작 X 좌표 (화면 픽셀)
 * @param start_y 시작 Y 좌표 (화면 픽셀)
 * @param start_angle 시작 각도 (0-359)
 */
void car_physics_init(car_state_t* car, int16_t start_x, int16_t start_y, int16_t start_angle);

/**
 * 가속 적용 (A버튼=전진, B버튼=후진)
 * @param car 자동차 상태 포인터
 * @param params 물리 파라미터
 * @param forward true=전진, false=후진
 */
void car_apply_acceleration(car_state_t* car, const car_physics_params_t* params, bool forward);

/**
 * 브레이크 적용 (조이스틱 아래)
 * @param car 자동차 상태 포인터
 * @param params 물리 파라미터
 */
void car_apply_brake(car_state_t* car, const car_physics_params_t* params);

/**
 * 회전 적용 (조이스틱 좌/우)
 * - 후진 시 방향 자동 반전
 * - 정지 시 회전 불가
 * @param car 자동차 상태 포인터
 * @param params 물리 파라미터
 * @param direction -1=좌회전, +1=우회전
 */
void car_apply_turn(car_state_t* car, const car_physics_params_t* params, int8_t direction);

/**
 * 전체 물리 업데이트 (매 프레임 호출)
 * - 마찰 적용
 * - 위치 업데이트
 * - 플래그 리셋
 * @param car 자동차 상태 포인터
 * @param params 물리 파라미터
 */
void car_physics_update(car_state_t* car, const car_physics_params_t* params);

/**
 * 화면 경계 처리
 * @param car 자동차 상태 포인터
 * @param screen_width 화면 너비
 * @param screen_height 화면 높이
 * @param car_width 자동차 비트맵 너비
 * @param car_height 자동차 비트맵 높이
 */
void car_clamp_to_screen(car_state_t* car, uint16_t screen_width, uint16_t screen_height,
                         uint16_t car_width, uint16_t car_height);

/**
 * 화면 좌표 얻기 (고정소수점 -> 정수 변환)
 */
int16_t car_get_screen_x(const car_state_t* car);
int16_t car_get_screen_y(const car_state_t* car);

/**
 * 현재 이동 중인지 확인
 */
bool car_is_moving(const car_state_t* car);

/**
 * 후진 중인지 확인
 */
bool car_is_reversing(const car_state_t* car);

#endif // CAR_PHYSICS_H
