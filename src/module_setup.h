#pragma once

#include "leos/cyphal/node.h"
#include "leos/mcp251xfd.h"

int module_setup_init(MCP251XFD *dev, leos_cyphal_node_t *node);