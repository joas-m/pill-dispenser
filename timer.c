#include "timer.h"

#include "hardware/timer.h"
#include "pico/stdlib.h"

#include <stdint.h>
#include <stdlib.h>

recurring_timer_t* new_timer(uint64_t freq_us) {
    recurring_timer_t* timer = malloc(sizeof(recurring_timer_t));

    timer->freq = freq_us;
    timer->next = time_us_64() + freq_us;

    return timer;
}

recurring_timer_t* new_timer_seconds(uint64_t freq) {
    return new_timer(freq * US_IN_SECOND);
}

void destroy_timer(recurring_timer_t* timer) { free(timer); }

bool timeout_passed(recurring_timer_t* timer) {
    uint64_t now;

#ifdef TIMER_EXACT
    uint64_t num_passed;
#endif

    now = time_us_64();
    if (now >= timer->next) {
#ifdef TIMER_EXACT
        // How many times the timeout has passed
        num_passed = ((now - timer->next) / timer->freq) + 1;

        timer->next += timer->freq * num_passed;
#else
        timer->next = now + timer->freq;
#endif
        return true;
    } else {
        return false;
    }
}