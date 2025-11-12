#include "radio_transport.h"
#include "config.h"
#include "hardware/uart.h"
#include "leos/log.h"

void radio_transmit(const uint8_t* data, size_t len)
{
    if (data == NULL || len == 0) {
        return;
    }

    // In XBee Transparent Mode, we just write the raw payload to the UART.
    // The XBee module handles queueing and RF transmission.
    uart_write_blocking(RADIO_UART_ID, data, len);
}

void radio_cyphal_rx_callback(struct CanardRxTransfer* transfer, void* user_reference)
{
    // user_reference is unused
    (void)user_reference;

    LOG_INFO("Cyphal RX: PortID %d, %d bytes. Forwarding to XBee.",
        transfer->metadata.port_id,
        transfer->payload.size);

    const uint8_t* payload_bytes = (const uint8_t*)transfer->payload.data;

    for (size_t i = 0; i < transfer->payload.size; i++) {
        uint8_t ch = payload_bytes[i];
        LOG_INFO("[CYPHAL_RX]: 0x%02X\n", ch);
    }

    // Transmit the raw Cyphal payload over the radio
    radio_transmit((const uint8_t*)transfer->payload.data, transfer->payload.size);
}