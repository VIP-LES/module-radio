#include "leos/log.h"
#include "config.h"
#include "leos/mcp251xfd.h"
#include "leos/cyphal/transport/mcp251xfd.h"
#include "leos/mcp251xfd/debug.h"
#include "leos/cyphal/node.h"
#include "pico/stdlib.h"

void mcp_read_pending_cb(MCP251XFD* dev, void *node_ref) {
    leos_cyphal_node_t *node = (leos_cyphal_node_t *)node_ref;
    leos_cyphal_rx_process(node);
}

int init_module(MCP251XFD* dev, leos_cyphal_node_t *node) {
    // Setup CANBus Communication
    eERRORRESULT err;
    err = leos_mcp251xfd_init(dev, &can_hw_config, &can_config, true);
    if (err != ERR_OK)
    {
        LOG_ERROR("Failed to init MCP251XFD: %s", mcp251xfd_debug_error_reason(err));
        return -1;
    }
    leos_cyphal_transport_t transport = leos_cyphal_transport_mcp251xfd(dev);
    leos_cyphal_result_t can_result;
    can_result = leos_cyphal_init(node, transport, 12);
    if (can_result != LEOS_CYPHAL_OK)
    {
        LOG_ERROR("Failed to initialize Cyphal/Libcanard: %d", can_result);
        return -2;
    }
    // Attach CANBus receive handler
    leos_mcp251xfd_set_rx_handler(dev, mcp_read_pending_cb, node);

    // Setup board LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}