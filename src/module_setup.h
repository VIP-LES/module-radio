#pragma once
#include "leos/mcp251xfd.h"
#include "leos/cyphal/node.h"

int init_module(MCP251XFD *dev, leos_cyphal_node_t *node);