#pragma once

#include "canard.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Transmits a raw data buffer over the XBee UART.
 *
 * Assumes the XBee module is in Transparent Mode (AP=0).
 *
 * @param data Pointer to the data buffer to transmit.
 * @param len  Number of bytes to transmit.
 */
void radio_transmit(const uint8_t *data, size_t len);

/**
 * @brief Cyphal subscription callback that forwards received payloads to the XBee.
 *
 * This function is called by the Cyphal stack when a message on the
 * subscribed port ID is received.
 *
 * @param transfer The received Cyphal transfer object, containing the payload.
 * @param user_reference A user-defined pointer (unused in this example).
 */
void radio_cyphal_rx_callback(struct CanardRxTransfer *transfer, void *user_reference);
int radio_init(void);