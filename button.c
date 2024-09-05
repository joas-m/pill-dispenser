#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "button.h"

static bool buttons_initialized = false;

void init_buttons() {
    if (!buttons_initialized) {
        // Init gpio pins
        gpio_init(BTN_0_PIN);
        gpio_init(BTN_1_PIN);
        gpio_init(BTN_2_PIN);

        // Configure button pins as inputs
        gpio_set_dir(BTN_0_PIN, GPIO_IN);
        gpio_set_dir(BTN_1_PIN, GPIO_IN);
        gpio_set_dir(BTN_2_PIN, GPIO_IN);

        // Pull button pins up
        gpio_pull_up(BTN_0_PIN);
        gpio_pull_up(BTN_1_PIN);
        gpio_pull_up(BTN_2_PIN);

        buttons_initialized = true;
    }
}

bool btn_pressed(btn_t btn) {
    switch (btn) {
    case BTN_0:
        return gpio_get(BTN_0_PIN) == 0;

    case BTN_1:
        return gpio_get(BTN_1_PIN) == 0;

    case BTN_2:
        return gpio_get(BTN_2_PIN) == 0;

    default:
        return false;
    }
}