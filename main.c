#define TIMER_EXACT

#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "button.h"
#include "debug.h"
#include "led.h"
#include "lora.h"
#include "stepper.h"
#include "timer.h"
#include "watchdog.h"

#define MAIN_LOOP_SLEEP 10

#define SECONDS_PER_PILL 30
#define WATCHDOG_FEED_DELAY_US (750 * US_IN_MS)
#define BLINK_FREQ_MS 500
#define BLINK_FREQ_US (BLINK_FREQ_MS * US_IN_MS)

#define NUM_PILLS 7

#define BLINK_TIMES_WHEN_EMPTY 5

static bool first_run = true;

/// Tries to drop a single pill. Blinks a LED and tries to report to the LoRa
/// receiver on failure
static void drop_pill(void);

static void drop_pill() {
    lora_send_message("Dropping pill");

    if (step()) {
        lora_send_message("Pill dropped successfully");
    } else {
        lora_send_message("No pills dropped");

        feed_watchdog(WATCHDOG_FEED_BLINKING);
        for (uint8_t i = 0; i < BLINK_TIMES_WHEN_EMPTY; ++i) {
            set_led_state(LED_0, true);
            sleep_ms(BLINK_FREQ_MS / 2);
            feed_watchdog(WATCHDOG_FEED_BLINKING);

            set_led_state(LED_0, false);
            sleep_ms(BLINK_FREQ_MS / 2);
            feed_watchdog(WATCHDOG_FEED_BLINKING);
        }
    }
}

int main(void) {
    recurring_timer_t* feeder;
    recurring_timer_t* blinker;
    recurring_timer_t* rotator;
    uint8_t pills_dropped;

    stdio_init_all();
    printf("Serial port initialized\n");

    while (true) {
        init_watchdog();

        init_lora();

        init_buttons();
        init_leds();
        init_stepper();

        lora_connect();

        if (first_run) {
            first_run = false;
            lora_send_message("Pill dispenser turned on");
        }

        // Wait for button 0 to be pressed
        feeder = new_timer(WATCHDOG_FEED_DELAY_US);
        blinker = new_timer(BLINK_FREQ_US / 2);
        while (!btn_pressed(BTN_0)) {
            if (timeout_passed(feeder)) {
                feed_watchdog(WATCHDOG_FEED_WAITING_FOR_INPUT);
            }

            if (timeout_passed(blinker)) {
                toggle_led_state(LED_0);
            }

            sleep_ms(MAIN_LOOP_SLEEP);
        }
        set_led_state(LED_0, false);
        destroy_timer(blinker);

        DBG("Starting calibration\n");
        lora_send_message("Starting pill dispenser calibration");

        calibrate(true);

        lora_send_message("Pill dispenser calibrated");

        DBG("%d steps/rotation\n", steps_per_rotation());

        set_led_state(LED_0, true);
        while (!btn_pressed(BTN_0)) {
            if (timeout_passed(feeder)) {
                feed_watchdog(WATCHDOG_FEED_WAITING_FOR_INPUT);
            }

            sleep_ms(MAIN_LOOP_SLEEP);
        }
        set_led_state(LED_0, false);
        rotator = new_timer_seconds(SECONDS_PER_PILL);

        feed_watchdog(WATCHDOG_FEED_OTHER);
        // Drop the first pill instantly
        drop_pill();
        pills_dropped = 1;

        while (pills_dropped < NUM_PILLS) {
            if (timeout_passed(feeder)) {
                feed_watchdog(WATCHDOG_FEED_FED_IN_MAIN);
            }

            if (timeout_passed(rotator)) {
                drop_pill();
                ++pills_dropped;
            }

            if (timeout_passed(feeder)) {
                feed_watchdog(WATCHDOG_FEED_FED_IN_MAIN);
            }

            sleep_ms(MAIN_LOOP_SLEEP);
        }

        lora_send_message("All pills dispensed, starting over");

        feed_watchdog(WATCHDOG_FEED_OTHER);

        /// Free timers before looping and recreating them to prevent leaking
        /// memory
        destroy_timer(rotator);
        destroy_timer(feeder);
    }
}