#pragma once
#include "leos/mcp251xfd/config.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

// -------- RADIO MODULE DEFINITIONS --------
#define RADIO_UART_ID uart0
// XBee default baud rate is 9600
#define RADIO_UART_BAUD_RATE 9600
#define RADIO_UART_TX_PIN 0
#define RADIO_UART_RX_PIN 1
// ------------------------------------------

// !!!!!!!!!!! IMPORTANT !!!!!!!!!!!
// !! We must replace these values with the
// !! Port ID and max size (extent) of your message.
//
// The Port ID for the message you want to subscribe to.
#define CYPHAL_PORT_ID_TO_FORWARD 7509
// The maximum size (extent) of that message in bytes.
#define CYPHAL_MESSAGE_EXTENT 64
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!