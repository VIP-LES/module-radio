#include "pico/stdlib.h"

#include "config.h"
#include "cyphal_bridge.h"
#include "module_setup.h"
#include "radio.h"

#include "canard.h"
#include "leos/cyphal/node.h"
#include "leos/mcp251xfd.h"

int main(void)
{
    stdio_init_all();

    MCP251XFD dev;
    leos_cyphal_node_t node;

    int rc = module_setup_init(&dev, &node);
    if (rc != 0)
    {
        while (true)
        {
            tight_loop_contents();
        }
    }

    /* Register Cyphal subscriptions. All callback bodies live in
     * cyphal_bridge.c — main.c is the single place where the
     * application's message-level behavior is visible. */

    leos_cyphal_result_t sub_rc;

    sub_rc = leos_cyphal_subscribe(
        &node,
        CanardTransferKindMessage,
        CYPHAL_SUB_SENSOR_GPS_ID,
        CYPHAL_SUB_SENSOR_GPS_EXTENT,
        cyphal_bridge_on_sensor_gps,
        NULL);

    if (sub_rc != LEOS_CYPHAL_OK)
    {
        /* Non-fatal: log and continue. The board can still forward EFM
         * traffic even if this subscription fails. */
        /* LOG_ERROR("Failed to subscribe sensor_gps: %d", sub_rc); */
    }

    sub_rc = leos_cyphal_subscribe(
        &node,
        CanardTransferKindMessage,
        CYPHAL_SUB_EFM_ID,
        CYPHAL_SUB_EFM_EXTENT,
        cyphal_bridge_on_efm,
        NULL);

    if (sub_rc != LEOS_CYPHAL_OK)
    {
        /* LOG_ERROR("Failed to subscribe efm: %d", sub_rc); */
    }

    /* Main loop — keep this simple and fast. */
    while (true)
    {
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);
        radio_service_irqs();

        if (radio_sx1262_packet_available())
        {
            cyphal_bridge_publish_sx1262_rx(&node);
        }

        tight_loop_contents();
    }

    return 0;
}