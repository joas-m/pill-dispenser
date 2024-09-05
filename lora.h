#ifndef LORA_H
#define LORA_H

#include "hardware/uart.h"

#include <stdbool.h>

#define LORA_TIMEOUT_US (10 * 1000)

#define LORA_DATA_BITS 8
#define LORA_STOP_BITS 1
#define LORA_PARITY UART_PARITY_NONE

#define LORA_UART_ID uart1
#define LORA_UART_IRQ UART1_IRQ

#define LORA_BAUD_RATE 9600

#define LORA_UART0_TX_PIN 0
#define LORA_UART0_RX_PIN 1

#define LORA_UART1_TX_PIN 4
#define LORA_UART1_RX_PIN 5

#define LORA_UART_TX_PIN LORA_UART1_TX_PIN
#define LORA_UART_RX_PIN LORA_UART1_RX_PIN

#define LORA_APPKEY "8979adfccaf0fb5e4e087ecb2f00157e"

/// 'AT+' + '\n' + '\0'
#define LORA_MIN_COMMAND_BYTES (3 + 1 + 1)
#define LORA_MIN_DATA_BYTES 1

#define LORA_COMMAND_SEPARATOR "\n"
#define LORA_DATA_SEPARATOR "="

#define LORA_BASIC_COMMAND "AT"

#define LORA_COMMAND_BASE "AT+"
#define LORA_COMMAND_VERSION "VER"
#define LORA_COMMAND_MODE "MODE"
#define LORA_COMMAND_APPKEY "KEY"
#define LORA_COMMAND_CLASS "CLASS" // A
#define LORA_COMMAND_PORT "PORT" // 8
#define LORA_COMMAND_JOIN "JOIN"
#define LORA_COMMAND_MSG "MSG"

#define LORA_MODE_DATA "LWOTAA"
#define LORA_APPKEY_DATA "APPKEY,\"" LORA_APPKEY "\""

#define LORA_RESPONSE_START "+"
#define LORA_RESPONSE_DATA_SEPARATOR ": "
#define LORA_RESPONSE_END "\r\n"

/// Initializes the LoRa module
void init_lora(void);

/// Tries to connect to a network with the LoRa module. Return true on success
bool lora_connect(void);

/// Sends a message to the LoRa receiver
void lora_send_message(char* msg);

#endif