#pragma once
#include "leos/mcp251xfd/config.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

// !!!!!!!!!!! IMPORTANT !!!!!!!!!!!
// !! We must replace these values with the
// !! Port ID and max size (extent) of your message.
//
// The Port ID for the message you want to subscribe to.
#define CYPHAL_PORT_ID_TO_FORWARD 7509
// The maximum size (extent) of that message in bytes.
#define CYPHAL_MESSAGE_EXTENT 64
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!