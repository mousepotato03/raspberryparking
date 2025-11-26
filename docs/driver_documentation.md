# 라즈베리파이 LCD + 입력 장치 드라이버 문서

## 목차

1. [프로젝트 개요](#프로젝트-개요)
2. [하드웨어 핀 배치](#하드웨어-핀-배치)
3. [드라이버 구조](#드라이버-구조)
4. [API 레퍼런스](#api-레퍼런스)
5. [사용 예시](#사용-예시)
6. [빌드 및 실행](#빌드-및-실행)

---

## 프로젝트 개요

이 프로젝트는 라즈베리파이에서 ST7789 LCD 디스플레이와 디지털 조이스틱, 버튼을 제어하기 위한 드라이버 라이브러리입니다.

### 주요 구성 요소

- **LCD**: ST7789 240x240 TFT LCD (SPI 통신)
- **조이스틱**: 5방향 디지털 조이스틱 (UP/DOWN/LEFT/RIGHT/CENTER)
- **버튼**: 2개의 푸시 버튼 (A, B)
- **라이브러리**: BCM2835 (GPIO/SPI 제어)

### 주요 기능

- ST7789 LCD 초기화 및 그래픽 출력
- RGB565 색상 지원
- 디바운싱이 적용된 버튼 입력
- 5방향 조이스틱 입력 감지
- 실시간 입력 상태 시각화

---

## 하드웨어 핀 배치

### LCD (ST7789) - SPI 통신

| GPIO 번호 | 핀 이름  | 용도                | 설명                    |
|-----------|----------|---------------------|-------------------------|
| 8         | TFT_CS   | SPI Chip Select     | SPI CE0                 |
| 10        | MOSI     | SPI Data Out        | SPI 데이터 전송         |
| 11        | SCLK     | SPI Clock           | SPI 클럭                |
| 24        | TFT_RST  | Reset               | 하드웨어 리셋           |
| 25        | TFT_DC   | Data/Command        | 데이터/명령 선택        |
| 26        | BL       | Backlight           | 백라이트 제어           |

**연결 방식**: SPI0 (CE0) 사용

### 조이스틱 (5방향 디지털)

| GPIO 번호 | 핀 이름    | 용도        | 풀업/다운 |
|-----------|------------|-------------|-----------|
| 17        | JOY_UP     | 상 방향     | 풀업      |
| 22        | JOY_DOWN   | 하 방향     | 풀업      |
| 27        | JOY_LEFT   | 좌 방향     | 풀업      |
| 23        | JOY_RIGHT  | 우 방향     | 풀업      |
| 4         | JOY_CENTER | 센터 버튼   | 풀업      |

**동작 방식**: 버튼을 누르면 LOW(0) 신호 발생

### 버튼

| GPIO 번호 | 핀 이름   | 용도     | 풀업/다운 |
|-----------|-----------|----------|-----------|
| 5         | BUTTON_A  | 버튼 A   | 풀업      |
| 6         | BUTTON_B  | 버튼 B   | 풀업      |

**동작 방식**: 버튼을 누르면 LOW(0) 신호 발생

### 핀 배치도

```
라즈베리파이 GPIO
┌─────────────────────────────────────┐
│  3.3V  5V  5V  GND                  │
│  GPIO2 ─                            │
│  GPIO3 ─                            │
│  GPIO4 ───────── JOY_CENTER         │
│  GND                                │
│  GPIO5 ───────── BUTTON_A           │
│  GPIO6 ───────── BUTTON_B           │
│  ...                                │
│  GPIO8 ───────── TFT_CS (SPI CE0)   │
│  GPIO10 ──────── MOSI               │
│  GPIO11 ──────── SCLK               │
│  ...                                │
│  GPIO17 ──────── JOY_UP             │
│  ...                                │
│  GPIO22 ──────── JOY_DOWN           │
│  GPIO23 ──────── JOY_RIGHT          │
│  GPIO24 ──────── TFT_RST            │
│  GPIO25 ──────── TFT_DC             │
│  GPIO26 ──────── BL                 │
│  GPIO27 ──────── JOY_LEFT           │
└─────────────────────────────────────┘
```

---

## 드라이버 구조

### 파일 구조

```
drivers/
├── common/
│   ├── gpio_init.h       # GPIO 핀 정의 및 초기화 함수 선언
│   └── gpio_init.c       # BCM2835 라이브러리 초기화 구현
├── lcd/
│   ├── st7789.h          # ST7789 LCD 드라이버 헤더
│   └── st7789.c          # ST7789 LCD 드라이버 구현
└── input/
    ├── button.h          # 버튼 드라이버 헤더
    ├── button.c          # 버튼 드라이버 구현 (디바운싱 포함)
    ├── joystick.h        # 조이스틱 드라이버 헤더
    └── joystick.c        # 조이스틱 드라이버 구현
```

### 모듈별 역할

#### 1. `common/gpio_init` - GPIO 초기화 모듈

**역할**:
- BCM2835 라이브러리 초기화
- 모든 GPIO 핀 설정을 한 곳에서 관리
- 프로그램 종료 시 정리 작업

**주요 함수**:
- `gpio_init_all()` - 전체 GPIO 초기화
- `gpio_cleanup()` - 정리 및 종료

#### 2. `lcd/st7789` - LCD 드라이버

**역할**:
- ST7789 LCD 초기화 및 제어
- SPI 통신을 통한 명령어/데이터 전송
- 기본 그래픽 출력 함수 제공

**주요 함수**:
- `st7789_init()` - LCD 초기화
- `st7789_fill_screen()` - 전체 화면 채우기
- `st7789_draw_pixel()` - 픽셀 그리기
- `st7789_draw_rect()` - 사각형 그리기

#### 3. `input/button` - 버튼 드라이버

**역할**:
- 버튼 입력 읽기
- 소프트웨어 디바운싱 (20ms)
- 블로킹/논블로킹 입력 지원

**주요 함수**:
- `button_read()` - 디바운싱된 버튼 상태 읽기
- `button_is_pressed()` - 버튼 눌림 확인
- `button_wait_press()` - 버튼 누름 대기

#### 4. `input/joystick` - 조이스틱 드라이버

**역할**:
- 5방향 디지털 조이스틱 입력 읽기
- 방향 감지 및 우선순위 처리
- 디바운싱 적용 (20ms)

**주요 함수**:
- `joystick_read_state()` - 전체 상태 읽기
- `joystick_get_direction()` - 현재 방향 감지
- `joystick_is_up/down/left/right/center()` - 개별 방향 확인

---

## API 레퍼런스

### GPIO 초기화 모듈 (`common/gpio_init.h`)

#### `int32_t gpio_init_all(void)`

BCM2835 라이브러리를 초기화하고 모든 GPIO 핀을 설정합니다.

**반환값**:
- `1`: 성공
- `0`: 실패 (root 권한 필요)

**사용 예시**:
```c
if (!gpio_init_all()) {
    printf("GPIO 초기화 실패\n");
    return 1;
}
```

#### `void gpio_cleanup(void)`

GPIO를 정리하고 BCM2835 라이브러리를 종료합니다.

**사용 예시**:
```c
gpio_cleanup();
```

---

### LCD 드라이버 (`lcd/st7789.h`)

#### `void st7789_init(void)`

ST7789 LCD를 초기화합니다.

**동작**:
- SPI 통신 시작
- 하드웨어/소프트웨어 리셋
- 색상 모드 설정 (RGB565)
- 디스플레이 활성화

**사용 예시**:
```c
st7789_init();
```

#### `void st7789_fill_screen(uint16_t color)`

전체 화면을 지정된 색상으로 채웁니다.

**매개변수**:
- `color`: RGB565 형식의 색상 코드

**사용 예시**:
```c
st7789_fill_screen(COLOR_RED);    // 빨간색으로 채우기
st7789_fill_screen(COLOR_BLUE);   // 파란색으로 채우기
st7789_fill_screen(0x0000);       // 검은색으로 채우기
```

#### `void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color)`

지정된 위치에 픽셀을 그립니다.

**매개변수**:
- `x`: X 좌표 (0-239)
- `y`: Y 좌표 (0-239)
- `color`: RGB565 색상

**사용 예시**:
```c
st7789_draw_pixel(120, 120, COLOR_WHITE);
```

#### `void st7789_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)`

채워진 사각형을 그립니다.

**매개변수**:
- `x`: 시작 X 좌표
- `y`: 시작 Y 좌표
- `w`: 너비
- `h`: 높이
- `color`: RGB565 색상

**사용 예시**:
```c
st7789_draw_rect(10, 10, 50, 30, COLOR_GREEN);
```

#### `uint16_t st7789_rgb_to_565(uint8_t r, uint8_t g, uint8_t b)`

RGB888 색상을 RGB565로 변환합니다.

**매개변수**:
- `r`: 빨강 (0-255)
- `g`: 녹색 (0-255)
- `b`: 파랑 (0-255)

**반환값**: RGB565 형식의 16비트 색상

**사용 예시**:
```c
uint16_t orange = st7789_rgb_to_565(255, 165, 0);
st7789_fill_screen(orange);
```

#### 색상 상수

| 상수명          | 값     | 색상    |
|----------------|--------|---------|
| COLOR_BLACK    | 0x0000 | 검정    |
| COLOR_WHITE    | 0xFFFF | 흰색    |
| COLOR_RED      | 0xF800 | 빨강    |
| COLOR_GREEN    | 0x07E0 | 녹색    |
| COLOR_BLUE     | 0x001F | 파랑    |
| COLOR_YELLOW   | 0xFFE0 | 노랑    |
| COLOR_CYAN     | 0x07FF | 청록    |
| COLOR_MAGENTA  | 0xF81F | 자홍    |

---

### 버튼 드라이버 (`input/button.h`)

#### 버튼 ID

```c
typedef enum {
    BTN_A = 0,
    BTN_B = 1
} button_id_t;
```

#### `uint8_t button_read(button_id_t btn)`

디바운싱이 적용된 버튼 상태를 읽습니다.

**매개변수**:
- `btn`: 버튼 ID (BTN_A 또는 BTN_B)

**반환값**:
- `BUTTON_PRESSED` (1): 눌림
- `BUTTON_RELEASED` (0): 안눌림

**사용 예시**:
```c
if (button_read(BTN_A) == BUTTON_PRESSED) {
    printf("버튼 A가 눌렸습니다\n");
}
```

#### `uint8_t button_is_pressed(button_id_t btn)`

버튼이 눌렸는지 확인합니다.

**반환값**:
- `1`: 눌림
- `0`: 안눌림

**사용 예시**:
```c
if (button_is_pressed(BTN_B)) {
    // 버튼 B 처리
}
```

#### `void button_wait_press(button_id_t btn)`

버튼이 눌릴 때까지 대기합니다 (블로킹).

**사용 예시**:
```c
printf("버튼 A를 눌러주세요...\n");
button_wait_press(BTN_A);
printf("눌렸습니다!\n");
```

#### `void button_wait_release(button_id_t btn)`

버튼이 떼어질 때까지 대기합니다 (블로킹).

**사용 예시**:
```c
button_wait_release(BTN_A);
```

---

### 조이스틱 드라이버 (`input/joystick.h`)

#### 방향 열거형

```c
typedef enum {
    JOY_DIR_NONE = 0,     // 입력 없음
    JOY_DIR_UP = 1,       // 위
    JOY_DIR_DOWN = 2,     // 아래
    JOY_DIR_LEFT = 3,     // 왼쪽
    JOY_DIR_RIGHT = 4,    // 오른쪽
    JOY_DIR_CENTER = 5    // 센터
} joystick_dir_t;
```

#### 상태 구조체

```c
typedef struct {
    uint8_t up;      // 1: 눌림, 0: 안눌림
    uint8_t down;
    uint8_t left;
    uint8_t right;
    uint8_t center;
} joystick_state_t;
```

#### `joystick_state_t joystick_read_state(void)`

조이스틱의 모든 방향 상태를 읽습니다.

**반환값**: 모든 방향의 상태를 담은 구조체

**사용 예시**:
```c
joystick_state_t state = joystick_read_state();
if (state.up) {
    printf("위 방향 눌림\n");
}
if (state.center) {
    printf("센터 버튼 눌림\n");
}
```

#### `joystick_dir_t joystick_get_direction(void)`

현재 눌린 방향을 감지합니다.

**우선순위**: UP > DOWN > LEFT > RIGHT > CENTER

**반환값**: 현재 방향 (JOY_DIR_*)

**사용 예시**:
```c
joystick_dir_t dir = joystick_get_direction();
switch (dir) {
    case JOY_DIR_UP:
        printf("위\n");
        break;
    case JOY_DIR_DOWN:
        printf("아래\n");
        break;
    case JOY_DIR_NONE:
        printf("입력 없음\n");
        break;
}
```

#### `uint8_t joystick_is_up(void)`
#### `uint8_t joystick_is_down(void)`
#### `uint8_t joystick_is_left(void)`
#### `uint8_t joystick_is_right(void)`
#### `uint8_t joystick_is_center(void)`

특정 방향이 눌렸는지 확인합니다 (디바운싱 적용).

**반환값**:
- `1`: 눌림
- `0`: 안눌림

**사용 예시**:
```c
if (joystick_is_up()) {
    player_y -= 1;  // 플레이어 위로 이동
}
```

#### `joystick_dir_t joystick_wait_any(void)`

조이스틱 입력을 기다립니다 (블로킹).

**반환값**: 눌린 방향

**사용 예시**:
```c
printf("조이스틱을 움직여주세요...\n");
joystick_dir_t dir = joystick_wait_any();
printf("방향: %d\n", dir);
```

---

## 사용 예시

### 예시 1: 간단한 LCD 테스트

```c
#include "common/gpio_init.h"
#include "lcd/st7789.h"

int main(void) {
    // 초기화
    gpio_init_all();
    st7789_init();

    // 빨간 화면
    st7789_fill_screen(COLOR_RED);
    bcm2835_delay(1000);

    // 녹색 화면
    st7789_fill_screen(COLOR_GREEN);
    bcm2835_delay(1000);

    // 사각형 그리기
    st7789_fill_screen(COLOR_BLACK);
    st7789_draw_rect(50, 50, 100, 80, COLOR_CYAN);

    // 정리
    bcm2835_delay(2000);
    bcm2835_spi_end();
    gpio_cleanup();

    return 0;
}
```

### 예시 2: 버튼으로 화면 색상 변경

```c
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "input/button.h"

int main(void) {
    gpio_init_all();
    st7789_init();

    st7789_fill_screen(COLOR_BLACK);

    while (1) {
        if (button_is_pressed(BTN_A)) {
            st7789_fill_screen(COLOR_RED);
        }

        if (button_is_pressed(BTN_B)) {
            st7789_fill_screen(COLOR_BLUE);
        }

        bcm2835_delay(50);
    }

    return 0;
}
```

### 예시 3: 조이스틱으로 사각형 이동

```c
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "input/joystick.h"

int main(void) {
    gpio_init_all();
    st7789_init();

    int16_t x = 100, y = 100;
    int16_t speed = 5;

    while (1) {
        // 이전 위치 지우기
        st7789_fill_screen(COLOR_BLACK);

        // 조이스틱 입력 처리
        if (joystick_is_up() && y > 0) {
            y -= speed;
        }
        if (joystick_is_down() && y < ST7789_HEIGHT - 20) {
            y += speed;
        }
        if (joystick_is_left() && x > 0) {
            x -= speed;
        }
        if (joystick_is_right() && x < ST7789_WIDTH - 20) {
            x += speed;
        }

        // 새 위치에 사각형 그리기
        st7789_draw_rect(x, y, 20, 20, COLOR_GREEN);

        bcm2835_delay(50);
    }

    return 0;
}
```

### 예시 4: 메뉴 시스템

```c
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "input/button.h"
#include "input/joystick.h"

void draw_menu(int selected) {
    st7789_fill_screen(COLOR_BLACK);

    // 메뉴 항목 그리기
    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == selected) ? COLOR_GREEN : COLOR_WHITE;
        st7789_draw_rect(20, 50 + i * 60, 200, 40, color);
    }
}

int main(void) {
    gpio_init_all();
    st7789_init();

    int selected = 0;

    draw_menu(selected);

    while (1) {
        // 위로 이동
        if (joystick_is_up()) {
            selected = (selected > 0) ? selected - 1 : 0;
            draw_menu(selected);
            bcm2835_delay(200);
        }

        // 아래로 이동
        if (joystick_is_down()) {
            selected = (selected < 2) ? selected + 1 : 2;
            draw_menu(selected);
            bcm2835_delay(200);
        }

        // 선택
        if (button_is_pressed(BTN_A)) {
            // 선택된 항목 처리
            st7789_fill_screen(COLOR_YELLOW);
            bcm2835_delay(500);
            draw_menu(selected);
        }

        bcm2835_delay(50);
    }

    return 0;
}
```

---

## 빌드 및 실행

### 사전 요구사항

1. **라즈베리파이** (3B+, 4 권장)
2. **Raspberry Pi OS** (Raspbian)
3. **root 권한** (GPIO 접근용)
4. **GCC 컴파일러** (기본 설치됨)

### BCM2835 라이브러리 설치

프로젝트 최초 실행 시 한 번만 설치하면 됩니다.

```bash
cd ~/프로젝트경로
make install-bcm2835
```

또는 수동 설치:

```bash
cd lib/bcm2835-1.75
./configure
make
sudo make install
```

### 프로젝트 빌드

```bash
make
```

빌드 결과물은 `bin/main`에 생성됩니다.

### 실행

**반드시 root 권한으로 실행해야 합니다.**

```bash
make run
```

또는

```bash
sudo ./bin/main
```

### 빌드 정리

```bash
make clean
```

### Makefile 타겟 목록

| 타겟 | 설명 |
|------|------|
| `make` 또는 `make all` | 프로젝트 빌드 |
| `make clean` | 빌드 결과물 삭제 |
| `make run` | 빌드 후 실행 (sudo) |
| `make install-bcm2835` | BCM2835 라이브러리 설치 |
| `make help` | 도움말 표시 |

---

## 트러블슈팅

### 1. "bcm2835_init failed" 오류

**원인**: root 권한 없이 실행

**해결**:
```bash
sudo ./bin/main
```

### 2. SPI 통신이 안됨

**원인**: SPI가 비활성화됨

**해결**:
```bash
sudo raspi-config
# Interface Options > SPI > Enable 선택
```

재부팅 후 다시 실행:
```bash
sudo reboot
```

### 3. 컴파일 오류 - bcm2835.h 찾을 수 없음

**원인**: BCM2835 라이브러리 미설치

**해결**:
```bash
make install-bcm2835
```

### 4. 버튼/조이스틱 입력이 반대로 동작

**원인**: 하드웨어가 풀다운 저항으로 구성됨

**해결**: `drivers/common/gpio_init.c`에서 풀업을 풀다운으로 변경:

```c
// 변경 전
bcm2835_gpio_set_pud(JOY_UP, BCM2835_GPIO_PUD_UP);

// 변경 후
bcm2835_gpio_set_pud(JOY_UP, BCM2835_GPIO_PUD_DOWN);
```

그리고 `drivers/input/button.c`와 `joystick.c`에서 LOW/HIGH 로직을 반전:

```c
// 변경 전
uint8_t level = bcm2835_gpio_lev(s_button_pins[btn]);
return (level == LOW) ? BUTTON_PRESSED : BUTTON_RELEASED;

// 변경 후
uint8_t level = bcm2835_gpio_lev(s_button_pins[btn]);
return (level == HIGH) ? BUTTON_PRESSED : BUTTON_RELEASED;
```

### 5. LCD 화면이 나오지 않음

**확인 사항**:
1. SPI 연결 확인 (MOSI, SCLK, CS)
2. 전원 연결 확인 (3.3V, GND)
3. GPIO 핀 번호 확인
4. 백라이트 전원 확인

**디버그 명령**:
```bash
# SPI 활성화 확인
ls /dev/spi*
# 출력: /dev/spidev0.0  /dev/spidev0.1

# GPIO 상태 확인
gpio readall
```

---

## 코딩 컨벤션

이 프로젝트는 `docs/convention.md`의 규칙을 따릅니다:

- **함수 복잡도**: 중첩 3단계 이내
- **함수 길이**: 50줄 이하
- **메모리 관리**: `malloc` 금지, static/stack 사용
- **네이밍**: `snake_case` 사용
  - 전역 변수: `g_` 접두사
  - 정적 변수: `s_` 접두사
  - 상수: `UPPER_CASE`
- **타입**: `<stdint.h>` 고정 폭 정수 사용

---

## 라이선스 및 참고

- **BCM2835 라이브러리**: Mike McCauley (GPL v3)
- **ST7789 예제**: docs/st7789_example.md 기반

---

## 버전 이력

- **v1.0** (2025-11-21): 초기 드라이버 구현
  - ST7789 LCD 드라이버
  - 5방향 디지털 조이스틱 드라이버
  - 2버튼 입력 드라이버
  - 디바운싱 적용
  - 실시간 입력 시각화 데모
