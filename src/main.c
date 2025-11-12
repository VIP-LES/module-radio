#include "config.h"
#include "leos/cyphal/node.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "module_setup.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "radio_transport.h"
#include "string.h"

void check_radio_uart_rx(void)
{
    while (uart_is_readable(RADIO_UART_ID)) {
        // Read one byte
        uint8_t ch = uart_getc(RADIO_UART_ID);

        LOG_INFO("[RADIO_RX]: 0x%02X\n", ch);
    }
}

void main()
{
    // --- INITIALIZE MODULE ---
    leos_log_init_console(ULOG_INFO_LEVEL);
    MCP251XFD dev;
    leos_cyphal_node_t node;
    if (init_module(&dev, &node) < 0) {
        LOG_ERROR("A critical communications error has occurred. This node is offline.");
        return;
    }

    // DEBUG

    absolute_time_t last_debug_time = get_absolute_time();
    const uint32_t debug_interval_ms = 1000;

    // END DEBUG

    leos_cyphal_result_t sub_result = leos_cyphal_subscribe(
        &node,
        CanardTransferKindMessage, // Or CanardTransferKindRequest
        CYPHAL_PORT_ID_TO_FORWARD,
        CYPHAL_MESSAGE_EXTENT,
        radio_cyphal_rx_callback, // This function forwards to the radio
        NULL // No user reference needed
    );

    if (sub_result != LEOS_CYPHAL_OK) {
        LOG_ERROR("Failed to subscribe to Port ID %d. Error: %d", CYPHAL_PORT_ID_TO_FORWARD, sub_result);
    } else {
        LOG_INFO("Subscribed to Cyphal Port ID %d for XBee forwarding", CYPHAL_PORT_ID_TO_FORWARD);
    }

    // After finishing initialization, set our mode to operational
    node.mode.value = uavcan_node_Mode_1_0_OPERATIONAL;
    // Turn on board LED to indicate setup success.
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);

        absolute_time_t now = get_absolute_time();

        if (absolute_time_diff_us(last_debug_time, now) / 1000 > debug_interval_ms) {
            last_debug_time = now;

            LOG_INFO("DEBUG: Main loop running @ %llu\n", time_us_64());

            const char* msg = "DEBUG: Main loop running\r\n";
            radio_transmit((const uint8_t*)msg, strlen(msg));
        }

        // --- 2. Check for data *from* the radio ---
        check_radio_uart_rx();
    }
}