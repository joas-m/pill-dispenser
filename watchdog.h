#ifndef WATCHDOG_H
#define WATCHDOG_H

#define WATCHDOG_TIMER_MS 3000

typedef enum {
    WATCHDOG_FEED_OTHER,
    WATCHDOG_FEED_WAITING_FOR_INPUT,
    WATCHDOG_FEED_FED_IN_MAIN,
    WATCHDOG_FEED_BLINKING,
    WATCHDOG_FEED_CALIBRATING,
    WATCHDOG_FEED_ROTATING,
    WATCHDOG_FEED_LORA,
} watchdog_feed_reason_t;

/// Initializes Pico watchdog
void init_watchdog(void);

/// Tells the watchdog that all is fine
void feed_watchdog(watchdog_feed_reason_t reason);

#endif