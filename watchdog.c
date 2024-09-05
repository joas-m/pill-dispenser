#include "watchdog.h"
#include "debug.h"

#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdio.h>

static bool watchdog_initialized = false;

void init_watchdog() {
    if (!watchdog_initialized) {
        if (watchdog_caused_reboot()) {
            printf("Rebooted by watchdog\n");

#ifdef ENABLE_DEBUG_PRINTS
            sleep_ms(5000);
            printf("Continuing\n");
#endif
        }

        watchdog_enable(WATCHDOG_TIMER_MS, true);

        watchdog_initialized = true;
    }
}

void feed_watchdog(watchdog_feed_reason_t reason) {
    DBG("[%lld]: ", time_us_64() / 1000);

    switch (reason) {
    case WATCHDOG_FEED_WAITING_FOR_INPUT:
        DBG("Fed watchdog while waiting for user input\n");
        break;

    case WATCHDOG_FEED_FED_IN_MAIN:
        DBG("Fed watchdog in main loop on a timer\n");
        break;

    case WATCHDOG_FEED_BLINKING:
        DBG("Fed watchdog while blinking\n");
        break;

    case WATCHDOG_FEED_CALIBRATING:
        DBG("Fed watchdog while calibrating the stepper motor\n");
        break;

    case WATCHDOG_FEED_ROTATING:
        DBG("Fed watchdog while rotating the stepper motor\n");
        break;

    case WATCHDOG_FEED_LORA:
        DBG("Fed watchdog while while communicating with the LoRa module\n");
        break;

    default:
        DBG("Watchdog fed\n");
        break;
    }

    watchdog_update();
}