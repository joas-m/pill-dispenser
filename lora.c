#include "lora.h"
#include "debug.h"
#include "watchdog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"

// #define LORA_TRACE_RESPONSE

/// Sends a command with optional data to the LoRa module
static void lora_send_command(uart_inst_t* uart, char* cmd, char* data);

/// Tries to listen to a response from the LoRa module. Returns true if the
/// response matches and false if the response differed or timed out
static bool lora_expect_response(uart_inst_t* uart, char* expected);

/// Checks if the LoRa module is connected
static bool lora_check_presence(uart_inst_t* uart);

static bool lora_initialized = false;
static bool lora_present = false;
static bool lora_connected = false;

static void lora_send_command(uart_inst_t* uart, char* cmd, char* data) {
    size_t base_len;
    size_t cmd_len;
    size_t data_len;
    size_t data_sep_len;
    char* msg;

    if (cmd != NULL) {
        base_len = strlen(LORA_COMMAND_BASE);
        cmd_len = strlen(cmd);

        if (data == NULL) {
            data_len = 0;
            data_sep_len = 0;
            msg = malloc(LORA_MIN_COMMAND_BYTES + cmd_len);
        } else {
            data_len = strlen(data);
            data_sep_len = strlen(LORA_DATA_SEPARATOR);
            msg = malloc(LORA_MIN_COMMAND_BYTES + LORA_MIN_DATA_BYTES +
                         cmd_len + data_len);
        }
        if (msg == NULL) {
            DBG("Failed to allocate memory for message\n");
            return;
        }

        memcpy(msg, LORA_COMMAND_BASE, base_len);
        memcpy(msg + base_len, data, data_len);
        memcpy(msg + base_len + cmd_len, LORA_DATA_SEPARATOR, data_sep_len);
        if (data != NULL) {
            memcpy(msg + base_len + cmd_len + data_sep_len, data, data_len);
        }
        memcpy(msg + base_len + cmd_len + data_sep_len + data_len,
               LORA_COMMAND_SEPARATOR "\0", strlen(LORA_COMMAND_SEPARATOR) + 1);

        uart_puts(uart, msg);

        free(msg);

        DBG("Sent command: '%s'", cmd);
        if (data == NULL) {
            DBG("\n");
        } else {
            DBG(" with data: '%s'\n", data);
        }
    }
}

static bool lora_expect_response(uart_inst_t* uart, char* expected) {
    bool match;
    char current;

    match = true;

    for (size_t i = 0; expected[i] != '\0'; ++i) {
        if (!uart_is_readable_within_us(uart, LORA_TIMEOUT_US)) {
            return false;
        }
        feed_watchdog(WATCHDOG_FEED_LORA);

        current = uart_getc(uart);
#ifdef LORA_TRACE_RESPONSE
        DBG("Received: '%c'\n", current);
#endif

        if (current != expected[i]) {
            match = false;
        }

        if (current == '\0') {
            return expected[i] == '\0';
        } else if (expected[i] == '\0') {
            while (uart_is_readable_within_us(uart, LORA_TIMEOUT_US)) {
                current = uart_getc(uart);
#ifdef LORA_TRACE_RESPONSE
                DBG("Received: '%c'\n", current);
#endif
            }
        }
    }

    match = match && current != '\0';

    return match;
}

static bool lora_check_presence(uart_inst_t* uart) {
    if (lora_present && lora_initialized) {
        return true;
    }

    uart_puts(uart, LORA_BASIC_COMMAND LORA_COMMAND_SEPARATOR);

    return lora_expect_response(uart, LORA_RESPONSE_START
                                "AT" LORA_RESPONSE_DATA_SEPARATOR
                                "OK" LORA_RESPONSE_END);
}

void init_lora(void) {
    init_watchdog();

    if (!lora_initialized) {
        uart_init(LORA_UART_ID, LORA_BAUD_RATE);

        gpio_set_function(LORA_UART_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(LORA_UART_RX_PIN, GPIO_FUNC_UART);
        uart_set_format(LORA_UART_ID, LORA_DATA_BITS, LORA_STOP_BITS,
                        LORA_PARITY);

        lora_initialized = true;

        // Check LoRa module presence
        lora_present = lora_check_presence(LORA_UART_ID);

        if (lora_present) {
            DBG("LoRa module present\n");
        } else {
            DBG("LoRa module not present\n");
        }
    }
}

bool lora_connect() {
    init_watchdog();

    if (lora_connected && lora_present && lora_initialized) {
        return true;
    }

    lora_send_command(LORA_UART_ID, LORA_COMMAND_MODE, LORA_MODE_DATA);
    lora_expect_response(LORA_UART_ID, " "); // Discard any response

    lora_send_command(LORA_UART_ID, LORA_COMMAND_APPKEY, LORA_APPKEY_DATA);
    lora_expect_response(LORA_UART_ID, " "); // Discard any response

    lora_send_command(LORA_UART_ID, LORA_COMMAND_CLASS, "A");
    lora_expect_response(LORA_UART_ID, " "); // Discard any response

    lora_send_command(LORA_UART_ID, LORA_COMMAND_PORT, "8");
    lora_expect_response(LORA_UART_ID, " "); // Discard any response

    lora_send_command(LORA_UART_ID, LORA_COMMAND_JOIN, NULL);
    lora_expect_response(LORA_UART_ID, " "); // Discard any response

    return true;
}

void lora_send_message(char* msg) {
    char* quoted_msg;
    size_t msg_len;

    // if (!(lora_connected || lora_present || lora_initialized) || msg == NULL)
    // {
    //     return;
    // }

    msg_len = strlen(msg);

    quoted_msg = malloc(msg_len + 3);
    if (quoted_msg == NULL) {
        return;
    }

    quoted_msg[0] = '"';
    for (size_t i = 0; i < msg_len; ++i) {
        if (msg[i] == '\0' || msg[i] == '\n' || msg[i] == '\r' ||
            msg[i] == '"') {
            quoted_msg[i + 1] = '?';
        } else {
            quoted_msg[i + 1] = msg[i];
        }
    }
    quoted_msg[msg_len + 1] = '"';
    quoted_msg[msg_len + 2] = '\0';

    DBG("Sending message: '%s' to LoRa receiver\n", quoted_msg);

    lora_send_command(LORA_UART_ID, LORA_COMMAND_MSG, quoted_msg);
}

#ifdef LORA_TRACE_RESPONSE
#undef LORA_TRACE_RESPONSE
#endif