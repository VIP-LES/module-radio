#include "leos/log.h"
#include "config.h"
#include "leos/mcp251xfd.h"
#include "leos/cyphal/node.h"
#include "module_setup.h"
#include "pico/stdlib.h"

void main() {
    // --- INITIALIZE MODULE ---
    leos_log_init_console(ULOG_INFO_LEVEL);
    MCP251XFD dev;
    leos_cyphal_node_t node;
    if (init_module(&dev, &node) < 0) {
        LOG_ERROR("A critical communications error has occured. This node is offline.");
        return;
    }

    // Your setup code goes here


    // After finishing initialization, set our mode to operational
    node.mode.value = uavcan_node_Mode_1_0_OPERATIONAL;
    // Turn on board LED to indicate setup success.
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);

        // Your looping code goes here
    }
}