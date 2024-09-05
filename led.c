#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <stdbool.h>

#include "led.h"

static bool led_0_state = false;
static bool led_1_state = false;
static bool led_2_state = false;

static bool leds_initialized = false;

void init_leds() {
    if (!leds_initialized) {
        // Init gpio pins
        gpio_init(LED_0_PIN);
        gpio_init(LED_1_PIN);
        gpio_init(LED_2_PIN);

        // Configure LED pins as outputs
        gpio_set_dir(LED_0_PIN, GPIO_OUT);
        gpio_set_dir(LED_1_PIN, GPIO_OUT);
        gpio_set_dir(LED_2_PIN, GPIO_OUT);

        // Pull LED pins down
        gpio_pull_up(LED_0_PIN);
        gpio_pull_up(LED_1_PIN);
        gpio_pull_up(LED_2_PIN);

        leds_initialized = true;
    } else {
        gpio_put(LED_0_PIN, false);
        gpio_put(LED_1_PIN, false);
        gpio_put(LED_2_PIN, false);

        led_0_state = false;
        led_1_state = false;
        led_2_state = false;
    }
}

void set_led_state(led_t led, bool state) {
    switch (led) {
    case LED_0:
        led_0_state = state;
        gpio_put(LED_0_PIN, led_0_state);
        break;

    case LED_1:
        led_1_state = state;
        gpio_put(LED_1_PIN, led_1_state);
        break;

    case LED_2:
        led_2_state = state;
        gpio_put(LED_0_PIN, led_2_state);
        break;
    }
}

void toggle_led_state(led_t led) {
    switch (led) {
    case LED_0:
        led_0_state = !led_0_state;
        gpio_put(LED_0_PIN, led_0_state);
        break;

    case LED_1:
        led_1_state = !led_1_state;
        gpio_put(LED_1_PIN, led_1_state);
        break;

    case LED_2:
        led_2_state = !led_2_state;
        gpio_put(LED_0_PIN, led_2_state);
        break;
    }
}