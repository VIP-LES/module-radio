#include "module_setup.h"

#include "board_can.h"   /* can_hw_config, can_config: copy from dev's config.c/h */
#include "cyphal_config.h"
#include "leos/cyphal/transport/mcp251xfd.h"
#include "leos/log.h"
#include "leos/mcp251xfd/debug.h"

static void mcp_read_pending_cb(MCP251XFD *dev, void *node_ref)
{
    (void)dev;
    leos_cyphal_rx_process((leos_cyphal_node_t *)node_ref);
}

int module_setup_init(MCP251XFD *dev, leos_cyphal_node_t *node)
{
    eERRORRESULT err = leos_mcp251xfd_init(dev, &can_hw_config, &can_config, true);
    if (err != ERR_OK) {
        LOG_ERROR("MCP251XFD init failed: %s", mcp251xfd_debug_error_reason(err));
        return -1;
    }

    leos_cyphal_transport_t transport = leos_cyphal_transport_mcp251xfd(dev);
    leos_cyphal_result_t rc = leos_cyphal_init(node, transport, CYPHAL_NODE_ID);
    if (rc != LEOS_CYPHAL_OK) {
        LOG_ERROR("Cyphal init failed: %d", (int)rc);
        return -2;
    }

    /* Drain Cyphal RX whenever the CAN controller has frames pending. */
    leos_mcp251xfd_set_rx_handler(dev, mcp_read_pending_cb, node);
    return 0;
}
