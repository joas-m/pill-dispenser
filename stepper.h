#ifndef STEPPER_H
#define STEPPER_H

#include <stdbool.h>
#include <stdint.h>

#define STEPPER_A_PIN 2
#define STEPPER_B_PIN 3
#define STEPPER_C_PIN 6
#define STEPPER_D_PIN 13

#define OPTO_FORK_PIN 28
#define PIEZO_SENSOR_PIN 27

#define NUM_SLOTS 8

/// Initializes the stepper motor and related components
void init_stepper(void);

/// Moves a single slot forward
/// Returns whether a pill was detected
bool step(void);

/// Calibrates the dispenser, returns number of steps/rotation
void calibrate(bool force);

/// Checks if the stepper motor has been calibrated
bool is_calibrated(void);

/// Gets the number of steps it takes for the stepper motor to complete a full
/// revolution
uint32_t steps_per_rotation(void);

/// Gets the number of steps required for the stepper motor to rotate one slot
uint32_t steps_per_slot(void);

#endif