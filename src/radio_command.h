#pragma once

#include <stddef.h>
#include <stdint.h>

#include "leos/cyphal/node.h"

/* Validate one received leos.Packet and queue an authorized cutdown request. */
bool radio_command_handle(
    leos_cyphal_node_t *node, const uint8_t *data, size_t size);
