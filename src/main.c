#include "pico/stdlib.h"

#include "config.h"
#include "cyphal_bridge.h"
#include "module_setup.h"
#include "radio.h"
#include "tethered_test.h"

#include "canard.h"
#include "leos/cyphal/node.h"
#include "leos/mcp251xfd.h"

int main(void)
{
    stdio_init_all();

#if !defined(LEOS_FAKE_TELEMETRY_MODE)
#define LEOS_FAKE_TELEMETRY_MODE 0
#endif

#if !defined(LEOS_ENABLE_SX1268)
#define LEOS_ENABLE_SX1268 1
#endif

#if LEOS_FAKE_TELEMETRY_MODE
    const radio_init_options_t radio_options = {
        .sx1262_enabled = true,
        .sx1268_enabled = false,
    };

    const int rc = radio_init_with_options(&radio_options);
    if (rc != 0)
    {
        while (true)
        {
            tight_loop_contents();
        }
    }

    tethered_test_init();
#else
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

#if LEOS_ENABLE_SX1268
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
#endif
#endif

    /* Main loop — keep this simple and fast. */
    while (true)
    {
#if LEOS_FAKE_TELEMETRY_MODE
        radio_service_irqs();
        tethered_test_service();
#else
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);
        radio_service_irqs();

        if (radio_sx1262_packet_available())
        {
            cyphal_bridge_publish_sx1262_rx(&node);
        }
#endif

        tight_loop_contents();
    }

    return 0;
}
