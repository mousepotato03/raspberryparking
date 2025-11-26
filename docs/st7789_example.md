# ST7789 LCD 예제 코드 분석

BCM2835 라이브러리를 사용한 ST7789 LCD 제어 예제

---

## 1. 개요

- **디스플레이**: ST7789 240x240 TFT LCD
- **통신 방식**: SPI
- **색상 포맷**: RGB565 (16bit)
- **라이브러리**: bcm2835

---

## 2. 하드웨어 연결

### 핀 맵핑

| GPIO | 용도    | 설명                  |
| ---- | ------- | --------------------- |
| 8    | TFT_CS  | SPI Chip Select (CE0) |
| 25   | TFT_DC  | Data/Command 선택     |
| 24   | TFT_RST | 하드웨어 리셋         |
| 26   | BL      | 백라이트 제어         |
| 10   | MOSI    | SPI 데이터 출력       |
| 11   | SCLK    | SPI 클럭              |

---

## 3. BCM2835 라이브러리 사용법

### 3.1 초기화 및 종료

```c
// 라이브러리 초기화 (root 권한 필요)
if (!bcm2835_init()) {
    printf("bcm2835_init failed. Are you running as root?\n");
    return 1;
}

// SPI 종료 및 라이브러리 정리
bcm2835_spi_end();
bcm2835_close();
```

### 3.2 GPIO 설정

```c
// 핀 모드 설정 (출력)
bcm2835_gpio_fsel(TFT_DC, BCM2835_GPIO_FSEL_OUTP);
bcm2835_gpio_fsel(TFT_RST, BCM2835_GPIO_FSEL_OUTP);

// 핀 출력 제어
bcm2835_gpio_set(26);   // HIGH 출력
bcm2835_gpio_clr(26);   // LOW 출력
```

### 3.3 SPI 통신

```c
// SPI 초기화
bcm2835_spi_begin();

// 클럭 분주비 설정 (8분주 = 약 31.25MHz)
bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);

// 1바이트 전송
bcm2835_spi_transfer(data);
```

**클럭 분주비 옵션:**

- `BCM2835_SPI_CLOCK_DIVIDER_4` - 62.5 MHz
- `BCM2835_SPI_CLOCK_DIVIDER_8` - 31.25 MHz
- `BCM2835_SPI_CLOCK_DIVIDER_16` - 15.625 MHz

---

## 4. ST7789 드라이버 함수

### 4.1 명령어/데이터 전송

DC 핀으로 명령어와 데이터를 구분:

```c
// 명령어 전송 (DC = LOW)
void writeCommand(uint8_t cmd) {
    bcm2835_gpio_clr(TFT_DC);
    bcm2835_spi_transfer(cmd);
}

// 데이터 전송 (DC = HIGH)
void writeData(uint8_t data) {
    bcm2835_gpio_set(TFT_DC);
    bcm2835_spi_transfer(data);
}
```

### 4.2 초기화 시퀀스

```c
void st7789_init() {
    bcm2835_spi_begin();
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);

    // 하드웨어 리셋
    bcm2835_gpio_clr(TFT_RST);
    delay(100);
    bcm2835_gpio_set(TFT_RST);
    delay(100);

    // 소프트웨어 리셋
    writeCommand(0x01);  // SWRESET
    delay(150);

    // Sleep Out
    writeCommand(0x11);  // SLPOUT
    delay(500);

    // 색상 모드 설정 (16bit)
    writeCommand(0x3A);  // COLMOD
    writeData(0x55);
    delay(10);

    // 디스플레이 방향
    writeCommand(0x36);  // MADCTL
    writeData(0x00);

    // X 범위 설정
    writeCommand(0x2A);  // CASET
    writeData(0x00); writeData(0x00);        // XSTART
    writeData(0x00); writeData(0xEF);        // XEND (239)

    // Y 범위 설정
    writeCommand(0x2B);  // RASET
    writeData(0x00); writeData(0x00);        // YSTART
    writeData(0x00); writeData(0xEF);        // YEND (239)

    // 디스플레이 ON
    writeCommand(0x29);  // DISPON
    delay(100);
}
```

### 4.3 화면 채우기

```c
void st7789_fillScreen(uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    // 영역 설정
    writeCommand(0x2B);  // RASET
    writeData(0); writeData(0);
    writeData(0); writeData(239);

    writeCommand(0x2A);  // CASET
    writeData(0); writeData(0);
    writeData(0); writeData(239);

    // 메모리 쓰기 시작
    writeCommand(0x2C);  // RAMWR
    bcm2835_gpio_set(TFT_DC);  // 데이터 모드

    // 240 x 240 픽셀 전송
    for (int i = 0; i < 240; i++) {
        for (int j = 0; j < 240; j++) {
            bcm2835_spi_transfer(hi);
            bcm2835_spi_transfer(lo);
        }
    }
}
```

---

## 5. ST7789 명령어 레지스터

| 명령어  | 값   | 설명                         |
| ------- | ---- | ---------------------------- |
| NOP     | 0x00 | No Operation                 |
| SWRESET | 0x01 | 소프트웨어 리셋              |
| SLPOUT  | 0x11 | Sleep Out                    |
| DISPON  | 0x29 | 디스플레이 ON                |
| CASET   | 0x2A | Column Address Set (X좌표)   |
| RASET   | 0x2B | Row Address Set (Y좌표)      |
| RAMWR   | 0x2C | 메모리 쓰기                  |
| MADCTL  | 0x36 | 메모리 접근 제어 (화면 방향) |
| COLMOD  | 0x3A | 색상 모드 설정               |

---

## 6. 색상 포맷 (RGB565)

16비트 색상 코드:

- Red: 5bit (15-11)
- Green: 6bit (10-5)
- Blue: 5bit (4-0)

### 기본 색상 코드

```c
#define RED     0xF800  // 1111 1000 0000 0000
#define GREEN   0x07E0  // 0000 0111 1110 0000
#define BLUE    0x001F  // 0000 0000 0001 1111
#define WHITE   0xFFFF
#define BLACK   0x0000
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
```

### RGB를 RGB565로 변환

```c
uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
```

# st7789.h

```c
#pragma once

#include <bcm2835.h>
#include <stdio.h>

// Pin definitions (Adjust these for your specific hardware)
#define TFT_CS      8
#define TFT_DC      25
#define TFT_RST     24

#define ST7789_TFTWIDTH  240
#define ST7789_TFTHEIGHT 240

// ST7789 commands
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_COLMOD  0x3A

void writeCommand(uint8_t cmd);

void writeData(uint8_t data);

void st7789_init();

void st7789_fillScreen(uint16_t color);
```

#st7789.c

```c
#include <bcm2835.h>
#include <stdio.h>

#include "st7789.h"

void writeCommand(uint8_t cmd) {
    bcm2835_gpio_clr(TFT_DC);
    bcm2835_spi_transfer(cmd);
}

void writeData(uint8_t data) {
    bcm2835_gpio_set(TFT_DC);
    bcm2835_spi_transfer(data);
}

void st7789_init() {
    bcm2835_spi_begin();
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);

    // Hardware reset
    bcm2835_gpio_clr(TFT_RST);
    delay(100);
    bcm2835_gpio_set(TFT_RST);
    delay(100);

    writeCommand(ST7789_SWRESET); // Software reset
    delay(150);

    writeCommand(ST7789_SLPOUT);  // Sleep out
    delay(500);

    writeCommand(ST7789_COLMOD);  // Set color mode
    writeData(0x55);              // 16-bit color
    delay(10);

    writeCommand(ST7789_MADCTL);
    writeData(0x00);              // Normal display

    writeCommand(ST7789_CASET);
    writeData(0x00); writeData(0x00); // XSTART = 0
    writeData(ST7789_TFTWIDTH >> 8); writeData(ST7789_TFTWIDTH & 0xFF); // XEND

    writeCommand(ST7789_RASET);
    writeData(0x00); writeData(0x00); // YSTART = 0
    writeData(ST7789_TFTHEIGHT >> 8); writeData(ST7789_TFTHEIGHT & 0xFF); // YEND

    writeCommand(ST7789_DISPON);  // Display on
    delay(100);
}

void st7789_fillScreen(uint16_t color) {
    uint8_t hi = color >> 8, lo = color & 0xFF;
    writeCommand(ST7789_RASET);
    writeData(0); writeData(0);
    writeData(ST7789_TFTHEIGHT >> 8); writeData(ST7789_TFTHEIGHT & 0xFF);

    writeCommand(ST7789_CASET);
    writeData(0); writeData(0);
    writeData(ST7789_TFTWIDTH >> 8); writeData(ST7789_TFTWIDTH & 0xFF);

    writeCommand(ST7789_RAMWR);
    bcm2835_gpio_set(TFT_DC);

    int i, j;
    for (i = 0; i < ST7789_TFTHEIGHT; i++) {
        for (j = 0; j < ST7789_TFTWIDTH; j++) {
            bcm2835_spi_transfer(hi);
            bcm2835_spi_transfer(lo);
        }
    }
}
```
