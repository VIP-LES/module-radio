#include "config.h"
#include "leos/cyphal/node.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "module_setup.h"
#include "pico/stdlib.h"
#include "radio_transport.h"

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

        // subscription already handled in callback

        // Your looping code goes here
    }
}