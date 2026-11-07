#pragma once

#include "leos/cyphal/node.h"
#include "leos/mcp251xfd.h"

/* Init CAN controller + Cyphal node. 0 on success, negative on failure. */
int module_setup_init(MCP251XFD *dev, leos_cyphal_node_t *node);
