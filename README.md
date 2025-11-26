# 라즈베리파이 LCD + 입력 장치 제어 프로젝트

라즈베리파이에서 ST7789 LCD 디스플레이와 조이스틱, 버튼을 제어하는 임베디드 프로젝트입니다.

## 🚀 빠른 시작

### 1. BCM2835 라이브러리 설치 (최초 1회)

```bash
make install-bcm2835
```

### 2. 프로젝트 빌드

```bash
make
```

### 3. 실행 (root 권한 필요)

```bash
make run
```

## 📦 구성 요소

### 하드웨어
- **LCD**: ST7789 240x240 TFT (SPI)
- **조이스틱**: 5방향 디지털 (UP/DOWN/LEFT/RIGHT/CENTER)
- **버튼**: 2개 (A, B)

### 소프트웨어
- **드라이버**:
  - `drivers/lcd/st7789` - LCD 제어
  - `drivers/input/button` - 버튼 입력 (디바운싱)
  - `drivers/input/joystick` - 조이스틱 입력
  - `drivers/common/gpio_init` - GPIO 초기화

## 🔌 핀 배치

### LCD
| GPIO | 용도 |
|------|------|
| 8    | CS   |
| 10   | MOSI |
| 11   | SCLK |
| 24   | RST  |
| 25   | DC   |
| 26   | BL   |

### 조이스틱
| GPIO | 방향 |
|------|------|
| 17   | UP    |
| 22   | DOWN  |
| 27   | LEFT  |
| 23   | RIGHT |
| 4    | CENTER|

### 버튼
| GPIO | 버튼 |
|------|------|
| 5    | A    |
| 6    | B    |

## 📚 문서

전체 API 레퍼런스 및 사용 예시는 다음 문서를 참고하세요:

- **[드라이버 문서](docs/driver_documentation.md)** - 전체 API 레퍼런스
- **[ST7789 예제](docs/st7789_example.md)** - LCD 제어 가이드
- **[코딩 컨벤션](docs/convention.md)** - 프로젝트 규칙

## 💻 사용 예시

```c
#include "common/gpio_init.h"
#include "lcd/st7789.h"
#include "input/button.h"
#include "input/joystick.h"

int main(void) {
    // 초기화
    gpio_init_all();
    st7789_init();

    // 빨간 화면 표시
    st7789_fill_screen(COLOR_RED);

    // 버튼 A 입력 대기
    if (button_is_pressed(BTN_A)) {
        st7789_fill_screen(COLOR_GREEN);
    }

    // 조이스틱 방향 읽기
    joystick_dir_t dir = joystick_get_direction();
    if (dir == JOY_DIR_UP) {
        // 위로 이동
    }

    // 정리
    gpio_cleanup();
    return 0;
}
```

## 🛠️ 빌드 명령어

| 명령어 | 설명 |
|--------|------|
| `make` | 프로젝트 빌드 |
| `make clean` | 빌드 결과물 삭제 |
| `make run` | 빌드 후 실행 (sudo) |
| `make help` | 도움말 표시 |

## ⚠️ 주의사항

1. **root 권한 필수**: GPIO 접근을 위해 `sudo`로 실행해야 합니다
2. **SPI 활성화**: `sudo raspi-config`에서 SPI를 활성화해야 합니다
3. **풀업 저항**: 모든 입력 핀은 풀업 저항 설정 (누르면 LOW)

## 🔧 트러블슈팅

### "bcm2835_init failed" 오류
```bash
sudo ./bin/main  # root 권한으로 실행
```

### SPI 통신 오류
```bash
sudo raspi-config
# Interface Options > SPI > Enable
sudo reboot
```

자세한 트러블슈팅은 [driver_documentation.md](docs/driver_documentation.md#트러블슈팅)를 참고하세요.

## 📁 프로젝트 구조

```
프로젝트/
├── drivers/           # 드라이버 소스
│   ├── common/       # GPIO 초기화
│   ├── lcd/          # LCD 드라이버
│   └── input/        # 입력 장치 드라이버
├── src/              # 메인 소스
│   └── main.c        # 데모 프로그램
├── lib/              # 외부 라이브러리
│   └── bcm2835-1.75/ # BCM2835 라이브러리
├── docs/             # 문서
├── assets/           # 리소스 (이미지 등)
├── Makefile          # 빌드 설정
└── README.md         # 이 파일
```

