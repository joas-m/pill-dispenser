#include "eeprom.h"
#include "debug.h"

#include <stdbool.h>
#include <stdint.h>

static bool eeprom_initialized = false;

void init_eeprom() {
    if (!eeprom_initialized) {
        i2c_init(EEPROM_I2C, EEPROM_BAUD_RATE);
        gpio_set_function(EEPROM_I2C_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(EEPROM_I2C_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(EEPROM_I2C_SDA_PIN);
        gpio_pull_up(EEPROM_I2C_SCL_PIN);

        eeprom_initialized = true;
    }
}

int16_t eeprom_read_byte(uint16_t addr) {
    uint8_t response;
    uint8_t msg[2] = {0};
    msg[0] = (addr >> 8) & 0xff;
    msg[1] = addr & 0xff;

    if (i2c_write_blocking(EEPROM_I2C, EEPROM_DEVICE_ADDR, msg, 2, true) ==
        PICO_ERROR_GENERIC) {
        DBG("Encountered an error while sending a message to EEPROM\n");
        return -1;
    }

    i2c_read_blocking(EEPROM_I2C, EEPROM_DEVICE_ADDR, &response, 1, false);

    return response;
}

int64_t eeprom_read_long(uint16_t addr) {
    int16_t tmp;
    uint32_t res = 0;
    uint8_t bytes[4] __attribute__((aligned(4))) = {0};

    for (uint8_t i = 0; i < 4; ++i) {
        tmp = eeprom_read_byte(addr + i);
        if (tmp == -1) {
            return -1;
        }
        bytes[i] = (uint8_t)tmp;
    }

    // Treat 4 element byte array as pointer to 4 byte int and deref
    res = *((uint32_t*)bytes);

    return res;
}

bool eeprom_write_byte(uint16_t addr, uint8_t byte) {
    uint8_t msg[3] = {0};
    msg[0] = (addr >> 8) & 0xff;
    msg[1] = addr & 0xff;
    msg[2] = byte;

    DBG("Writing byte 0x%02x\n", byte);
    if (i2c_write_blocking(EEPROM_I2C, EEPROM_DEVICE_ADDR, msg, 3, false) ==
        PICO_ERROR_GENERIC) {
        DBG("Encountered an error while writing to EEPROM\n");
        return false;
    }
    sleep_ms(EEPROM_WRITE_SLEEP_MS);
}

void eeprom_write_long(uint16_t addr, uint32_t unsigned_int) {
    uint8_t* bytes = (uint8_t*)(&unsigned_int);

    eeprom_write_byte(addr, bytes[0]);
    eeprom_write_byte(addr + 1, bytes[1]);
    eeprom_write_byte(addr + 2, bytes[2]);
    eeprom_write_byte(addr + 3, bytes[3]);
}
