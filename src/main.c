#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#include "cyphal_bridge.h"
#include "cyphal_config.h"
#include "leos/cyphal/node.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "module_setup.h"
#include "radio.h"
#include "radio_command.h"
#include "radio_protocol.h"

static void subscribe(leos_cyphal_node_t *node, uint16_t id, size_t extent,
                      void (*cb)(struct CanardRxTransfer *, void *), const char *name)
{
    leos_cyphal_result_t rc = leos_cyphal_subscribe(
        node, CanardTransferKindMessage, id, extent, cb, NULL);
    if (rc != LEOS_CYPHAL_OK) {
        LOG_ERROR("subscribe %s (subject %u) failed: %d", name, (unsigned)id, (int)rc);
    } else {
        LOG_INFO("subscribed %s on subject %u", name, (unsigned)id);
    }
}

static void fatal_blink(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    while (true) {
        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
        sleep_ms(150);
    }
}

int main(void)
{
    stdio_init_all();
    leos_log_init_console(ULOG_INFO_LEVEL);   /* enables LOG_INFO / LOG_ERROR output */

    /* Wait up to 5 s for a terminal so the startup lines are not missed. */
    for (int i = 0; i < 50 && !stdio_usb_connected(); i++) {
        sleep_ms(100);
    }
    LOG_INFO("=== module-radio Cyphal bridge starting ===");
    LOG_INFO("node id %u | subject: LowRate=%u (sensors, GPS, EFM x2, geiger)",
             (unsigned)CYPHAL_NODE_ID, (unsigned)CYPHAL_SUB_LOWRATE_ID);

    MCP251XFD dev;
    leos_cyphal_node_t node;

    int rc = module_setup_init(&dev, &node);
    if (rc != 0) {
        LOG_ERROR("module_setup_init failed: %d", rc);
        fatal_blink();
    }

    LOG_INFO("CAN + Cyphal node up");

    rc = radio_init();
    if (rc != 0) {
        LOG_ERROR("radio_init failed: %d", rc);
        fatal_blink();
    }
    LOG_INFO("radio ready");

    /* The LED means the complete module, not just the Pico runtime, is ready. */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

    subscribe(&node, CYPHAL_SUB_LOWRATE_ID, CYPHAL_SUB_LOWRATE_EXTENT,
              cyphal_bridge_on_lowrate, "LowRate");

    uint32_t next_stats_ms = to_ms_since_boot(get_absolute_time()) + 10000U;
    while (true) {
        const uint32_t t = to_ms_since_boot(get_absolute_time());
        if ((int32_t)(t - next_stats_ms) >= 0) {
            cyphal_bridge_log_stats();
            next_stats_ms = t + 10000U;
        }
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);
        cyphal_bridge_service();

        uint8_t packet[RADIO_RF_MAX_PACKET_SIZE];
        const int received = radio_receive(packet, sizeof(packet));
        if (received > 0) {
            (void)radio_command_handle(&node, packet, (size_t)received);
        }
        tight_loop_contents();
    }
}
