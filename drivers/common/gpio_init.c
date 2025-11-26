#include "gpio_init.h"
#include <stdio.h>

int32_t gpio_init_all(void) {
    // Initialize BCM2835 library
    if (!bcm2835_init()) {
        printf("bcm2835_init failed. Are you running as root?\n");
        return 0;
    }

    // Configure LCD pins
    bcm2835_gpio_fsel(TFT_DC, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TFT_BL, BCM2835_GPIO_FSEL_OUTP);

    // Turn on backlight by default
    bcm2835_gpio_set(TFT_BL);

    // Configure joystick pins (input with pull-up)
    bcm2835_gpio_fsel(JOY_UP, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOY_UP, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(JOY_DOWN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOY_DOWN, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(JOY_LEFT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOY_LEFT, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(JOY_RIGHT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOY_RIGHT, BCM2835_GPIO_PUD_UP);

    // Configure button pins (input with pull-up)
    bcm2835_gpio_fsel(BUTTON_A, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(BUTTON_A, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(BUTTON_B, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(BUTTON_B, BCM2835_GPIO_PUD_UP);

    printf("GPIO initialization completed\n");
    return 1;
}

void gpio_cleanup(void) {
    // Turn off backlight
    bcm2835_gpio_clr(TFT_BL);

    // Close BCM2835 library
    bcm2835_close();
    printf("GPIO cleanup completed\n");
}
