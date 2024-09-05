#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

#define US_IN_MS 1000
#define MS_IN_SECOND 1000

#define US_IN_SECOND (US_IN_MS * MS_IN_SECOND)

typedef struct {
    uint64_t freq;
    uint64_t next;
} recurring_timer_t;

/// Creates a new timer with the given frequency in microseconds
recurring_timer_t* new_timer(uint64_t freq_us);

/// Creates a new timer with the given frequency in seconds
recurring_timer_t* new_timer_seconds(uint64_t freq_us);

/// Destroys a timer and frees the used memory
void destroy_timer(recurring_timer_t* timer);

/// Checks if the timer's timout has passed and refreshes the timer if it has
bool timeout_passed(recurring_timer_t* timer);

#endif