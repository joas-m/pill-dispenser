#include "stepper.h"
#include "debug.h"
#include "eeprom.h"
#include "watchdog.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define STEP_SLEEP_MS 10
#define APPROX_STEPS_PER_ROTATION 2084

#define PIEZO_IRQ_EVENT_MASK GPIO_IRQ_LEVEL_LOW

#define STEPPER_TRANSACTION_MASK (1 << 31)

#define WATCHDOG_FEED_FREQ 100

/// Only supported backend. Uses nonpersistend somewhat working(?) solution if
/// not defined
#define PERSISTENCE_BACKEND_EEPROM
// #define SAVE_STEP_TO_EEPROM
// #define SAVE_SLOT_TO_EEPROM

/// Turns the stepper motor by a single step
static void step_single(void);

/// Starts motor transaction
static void start_transaction(uint32_t steps);

/// Ends motor transaction
static void clear_transaction(void);

/// Decrements the transaction step counter
///
/// Important: Do not call outside of transactions, or things *WILL* break
static bool decrement_transaction(void);

/// Gets the number of steps remaining in the transaction
static uint32_t get_transaction_remaining_steps(void);

/// Checks whether a transaction is active at the moment
static bool is_in_transaction(void);

/// Tries to complete the current transaction
static void continue_transaction(void);

/// Tries to get the saved number of steps per rotation from a previous
/// calibration
static uint32_t get_saved_calibration(void);

/// Saves the number of steps per rotation for future calibrations
static void save_calibration(uint32_t calibrated_steps_per_rotation);

static bool coil_a = true;
static bool coil_b = false;
static bool coil_c = false;
static bool coil_d = false;

static bool stepper_initialized = false;

static bool calibrated = false;
static uint32_t num_steps_per_rotation = APPROX_STEPS_PER_ROTATION;

static uint8_t current_slot = 0;

volatile static bool detected_pill = false;

#ifndef PERSISTENCE_BACKEND_EEPROM
volatile static uint32_t __scratch_x("stepper_transaction") stepper_transaction;
volatile static uint32_t __scratch_y("last_calibration") last_calibration;
#else
static uint8_t stepper_transaction;
static uint32_t transaction_steps;
static uint32_t last_calibration;
#endif

/// Marks the start of a transaction and saves how many steps should still be
/// traversed
static void start_transaction(uint32_t steps) {
#ifdef PERSISTENCE_BACKEND_EEPROM
    init_eeprom();
#endif

#ifndef PERSISTENCE_BACKEND_EEPROM
    stepper_transaction = STEPPER_TRANSACTION_MASK;
    stepper_transaction += steps;
#else
    stepper_transaction = 1;
    transaction_steps = steps;

    eeprom_write_byte(EEPROM_STEPPER_TRANSACTION_ENABLED_ADDRESS,
                      stepper_transaction);
#ifdef SAVE_STEP_TO_EEPROM
    eeprom_write_byte(EEPROM_STEPPER_TRANSACTION_REMAINING_STEPS_ADDRESS,
                      transaction_steps);
#endif
#endif
}

/// Clears the transaction scratch register
static void clear_transaction() {
    stepper_transaction = 0;
#ifdef PERSISTENCE_BACKEND_EEPROM
    eeprom_write_byte(EEPROM_STEPPER_TRANSACTION_ENABLED_ADDRESS,
                      stepper_transaction);
#ifdef SAVE_STEP_TO_EEPROM
    eeprom_write_byte(EEPROM_STEPPER_TRANSACTION_REMAINING_STEPS_ADDRESS,
                      stepper_transaction);
#endif
#endif
}

/// Decrements the remaining steps in the transaction and returns whether it
/// should still continue
static bool decrement_transaction() {
#ifndef PERSISTENCE_BACKEND_EEPROM
    --stepper_transaction;

    if (stepper_transaction == STEPPER_TRANSACTION_MASK) {
#else
    --transaction_steps;
    if (transaction_steps == 0) {
#endif
        clear_transaction();
        return false;
    }
#ifdef SAVE_STATE_TO_EEPROM
    else {
        eeprom_write_byte(EEPROM_STEPPER_TRANSACTION_REMAINING_STEPS_ADDRESS,
                          transaction_steps);
    }
#endif
    return true;
}

static uint32_t get_transaction_remaining_steps() {
#ifndef PERSISTENCE_BACKEND_EEPROM
    return stepper_transaction & (~STEPPER_TRANSACTION_MASK);
#else
    return transaction_steps;
#endif
}

/// Checks whether a transaction is underway
static bool is_in_transaction() {
#ifndef PERSISTENCE_BACKEND_EEPROM
    return stepper_transaction & (1 << 31);
#else
    return stepper_transaction;
#endif
}

static void continue_transaction() {
    init_watchdog();

    while (decrement_transaction()) {
        if (get_transaction_remaining_steps() % WATCHDOG_FEED_FREQ == 0) {
            feed_watchdog(WATCHDOG_FEED_ROTATING);
        }

        step_single();
        sleep_ms(STEP_SLEEP_MS);
    }
}

/// Returns the number of steps per rotation calculated in an earlier
/// calibration, or 0, if a calibration was not stored.
static uint32_t get_saved_calibration() {
#ifdef PERSISTENCE_BACKEND_EEPROM
    int64_t tmp;
    tmp = eeprom_read_long(EEPROM_STEPPER_CACHED_STEPS_PER_REVOLUTION);
    if (tmp != -1) {
        last_calibration = (uint32_t)last_calibration;
    }
#endif

    DBG("Loaded calibration data: %d\n", last_calibration);
    return last_calibration;
}

/// Saves a calibration into the Pi Pico's scratch registers
static void save_calibration(uint32_t calibrated_steps_per_rotation) {
    DBG("Saved calibration data (%d)\n", calibrated_steps_per_rotation);
    last_calibration = calibrated_steps_per_rotation;

#ifdef PERSISTENCE_BACKEND_EEPROM
    eeprom_write_long(EEPROM_STEPPER_CACHED_STEPS_PER_REVOLUTION,
                      last_calibration);
#endif
}

static void piezo_sensor_callback(uint gpio, uint32_t event_mask) {
    DBG("Piezo sensor detected dropped pill\n");
    detected_pill = true;
}

void init_stepper() {
    current_slot = 0;

    if (!stepper_initialized) {
        // Init gpio pins
        gpio_init(STEPPER_A_PIN);
        gpio_init(STEPPER_B_PIN);
        gpio_init(STEPPER_C_PIN);
        gpio_init(STEPPER_D_PIN);

        gpio_init(OPTO_FORK_PIN);
        gpio_init(PIEZO_SENSOR_PIN);

        // Configure stepper pins as outputs
        gpio_set_dir(STEPPER_A_PIN, GPIO_OUT);
        gpio_set_dir(STEPPER_B_PIN, GPIO_OUT);
        gpio_set_dir(STEPPER_C_PIN, GPIO_OUT);
        gpio_set_dir(STEPPER_D_PIN, GPIO_OUT);

        // Configure sensor pins as inputs
        gpio_set_dir(OPTO_FORK_PIN, GPIO_IN);
        gpio_set_dir(PIEZO_SENSOR_PIN, GPIO_IN);

        // Pull stepper pins down
        gpio_pull_down(STEPPER_A_PIN);
        gpio_pull_down(STEPPER_B_PIN);
        gpio_pull_down(STEPPER_C_PIN);
        gpio_pull_down(STEPPER_D_PIN);

        // Pull sensor pins up
        gpio_pull_up(OPTO_FORK_PIN);
        gpio_pull_up(PIEZO_SENSOR_PIN);

        // Set piezo sensor callback
        gpio_set_irq_enabled_with_callback(PIEZO_SENSOR_PIN,
                                           PIEZO_IRQ_EVENT_MASK, true,
                                           piezo_sensor_callback);

        stepper_initialized = true;
    }
}

static void step_single() {
    bool tmp = coil_d;

    coil_d = coil_c; // D = C
    coil_c = coil_b; // C = B
    coil_b = coil_a; // B = A
    coil_a = tmp;    // A = D

    gpio_put(STEPPER_A_PIN, coil_a);
    gpio_put(STEPPER_B_PIN, coil_b);
    gpio_put(STEPPER_C_PIN, coil_c);
    gpio_put(STEPPER_D_PIN, coil_d);
}

bool step() {
    uint32_t slot_steps;

    ++current_slot;

    slot_steps = num_steps_per_rotation / NUM_SLOTS;
    if (current_slot % NUM_SLOTS == 0) {
        DBG("Stepping remainder steps\n");
        slot_steps += num_steps_per_rotation % NUM_SLOTS;
        current_slot = 0;
    }

#ifdef SAVE_SLOT_TO_EEPROM
    eeprom_write_byte(EEPROM_STEPPER_CURRENT_SLOT_ADDRESS, current_slot);
#endif

    detected_pill = false;

    start_transaction(slot_steps);
    continue_transaction();
    clear_transaction();

    if (detected_pill) {
        detected_pill = false;
        return true;
    } else {
        return false;
    }
}

void calibrate(bool force) {
    uint32_t steps;
    uint32_t quarter_slot;
    uint32_t saved;
    uint32_t watchog_feeding_timer;

#ifdef SAVE_SLOT_TO_EEPROM
    int16_t tmp;
#endif

    init_watchdog();

    watchog_feeding_timer = 0;

    saved = get_saved_calibration();

    if (saved != 0 && !force) {
        DBG("Found calibration data\n");

#ifdef SAVE_SLOT_TO_EEPROM
        tmp = eeprom_read_byte(EEPROM_STEPPER_CURRENT_SLOT_ADDRESS);
        if (tmp == -1) {
            current_slot = 0;
        } else {
            current_slot = (uint8_t)tmp;
        }
#else
        current_slot = 0;
#endif

        num_steps_per_rotation = saved;
        calibrated = true;

        if (is_in_transaction()) {
            DBG("Found transaction with %d steps left\n",
                get_transaction_remaining_steps());
            continue_transaction();
            clear_transaction();
        }

        return;
    } else {
        DBG("No calibration data found\n");
        // Clear saved transaction, just in case
        save_calibration(0);
        clear_transaction();
    }

    // Step until light is sensed
    while (gpio_get(OPTO_FORK_PIN) != 0) {
        step_single();
        ++watchog_feeding_timer;
        if (watchog_feeding_timer % WATCHDOG_FEED_FREQ == 0) {
            feed_watchdog(WATCHDOG_FEED_CALIBRATING);
        }
        sleep_ms(STEP_SLEEP_MS);
    }

    // Count steps until calibration slot is over. This produces slightly
    // incorrect results
    DBG("Counting steps\n");
    steps = 0;
    while (gpio_get(OPTO_FORK_PIN) == 0) {
        step_single();
        ++steps;
        ++watchog_feeding_timer;
        if (watchog_feeding_timer % WATCHDOG_FEED_FREQ == 0) {
            feed_watchdog(WATCHDOG_FEED_CALIBRATING);
        }
        sleep_ms(STEP_SLEEP_MS);
    }

    // Calculate num of steps to correct with later
    quarter_slot = steps / 4;

    // Continue step counting
    while (gpio_get(OPTO_FORK_PIN) != 0) {
        step_single();
        ++steps;
        ++watchog_feeding_timer;
        if (watchog_feeding_timer % WATCHDOG_FEED_FREQ == 0) {
            feed_watchdog(WATCHDOG_FEED_CALIBRATING);
        }
        sleep_ms(STEP_SLEEP_MS);
    }

    // Correct for mistakes
    for (uint32_t i = 0; i < quarter_slot * 2; ++i) {
        step_single();
        ++watchog_feeding_timer;
        if (watchog_feeding_timer % WATCHDOG_FEED_FREQ == 0) {
            feed_watchdog(WATCHDOG_FEED_CALIBRATING);
        }
        sleep_ms(STEP_SLEEP_MS);
    }

    feed_watchdog(WATCHDOG_FEED_CALIBRATING);

    steps += quarter_slot;
    // steps -= steps / 20;

    save_calibration(steps);
    get_saved_calibration();
    calibrated = true;
    num_steps_per_rotation = steps;

    current_slot = 0;
#ifdef SAVE_SLOT_TO_EEPROM
    eeprom_write_byte(EEPROM_STEPPER_CURRENT_SLOT_ADDRESS, current_slot);
#endif
}

bool is_calibrated() { return calibrated; }

uint32_t steps_per_rotation() {
    if (calibrated) {
        return num_steps_per_rotation;
    } else {
        return 0;
    }
}

uint32_t steps_per_slot() {
    if (calibrated) {
        return num_steps_per_rotation / NUM_SLOTS;
    } else {
        return 0;
    }
}

#undef STEP_SLEEP_MS
#undef APPROX_STEPS_PER_ROTATION
#undef STEPPER_TRANSACTION_MASK
#undef WATCHDOG_FEED_FREQ