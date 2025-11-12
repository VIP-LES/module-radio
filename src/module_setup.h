#pragma once
#include "leos/cyphal/node.h"
#include "leos/mcp251xfd.h"

int init_module(MCP251XFD* dev, leos_cyphal_node_t* node);

// -------- RADIO MODULE DEFINITIONS --------
#define RADIO_UART_ID uart0
// XBee default baud rate is 9600
#define RADIO_UART_BAUD_RATE 9600
#define RADIO_UART_TX_PIN 0
#define RADIO_UART_RX_PIN 1
// ------------------------------------------