#pragma once

#include "leos/mcp251xfd/config.h"

/* CAN controller wiring and CAN FD settings. Same values as leos-firmware
 * common/platform/src/config.c (SPI0 pins 2-6, 40 MHz crystal, 1M/4M). */
extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;