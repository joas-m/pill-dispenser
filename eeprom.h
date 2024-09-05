#ifndef EEPROM_H
#define EEPROM_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define EEPROM_WRITE_SLEEP_MS 10
#define EEPROM_PAGE_SIZE 64
#define EEPROM_NUM_PAGES 512

#define EEPROM_DEVICE_ADDR 0x50

/// Is long, and therefore uses addresses 0x42, 0x43, 0x44 & 0x45
#define EEPROM_STEPPER_TRANSACTION_REMAINING_STEPS_ADDRESS 0x42

/// Is long, and therefore uses addresses 0x46, 0x47, 0x48 & 0x49
#define EEPROM_STEPPER_CACHED_STEPS_PER_REVOLUTION 0x46

#define EEPROM_STEPPER_TRANSACTION_ENABLED_ADDRESS 0x4a
#define EEPROM_STEPPER_CURRENT_SLOT_ADDRESS 0x4b

#define EEPROM_I2C i2c0

#define EEPROM_BAUD_RATE (100 * 1000)

#define EEPROM_I2C_SDA_PIN 16
#define EEPROM_I2C_SCL_PIN 17

/// Initializes EEPROM
void init_eeprom(void);

/// Reads a byte from the EEPROM
int16_t eeprom_read_byte(uint16_t addr);

/// Reads a long from the EEPROM
int64_t eeprom_read_long(uint16_t addr);

/// Writes a byte to the EEPROM
bool eeprom_write_byte(uint16_t addr, uint8_t byte);

/// Writes a long to the EEPROM
void eeprom_write_long(uint16_t addr, uint32_t unsigned_int);

#endif