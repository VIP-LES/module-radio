#include "pico/stdlib.h"

#include "cyphal_bridge.h"
#include "cyphal_config.h"
#include "leos/cyphal/node.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "module_setup.h"
#include "radio.h"

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

    MCP251XFD dev;
    leos_cyphal_node_t node;

    int rc = module_setup_init(&dev, &node);
    if (rc != 0) {
        LOG_ERROR("module_setup_init failed: %d", rc);
        fatal_blink();
    }

    rc = radio_init();
    if (rc != 0) {
        LOG_ERROR("radio_init failed: %d", rc);
        fatal_blink();
    }

    subscribe(&node, CYPHAL_SUB_LOWRATE_ID, CYPHAL_SUB_LOWRATE_EXTENT,
              cyphal_bridge_on_lowrate, "LowRate");
    subscribe(&node, CYPHAL_SUB_EFM_ID, CYPHAL_SUB_EFM_EXTENT,
              cyphal_bridge_on_efm, "EFM");

    while (true) {
        leos_mcp251xfd_task(&dev);
        leos_cyphal_task(&node);
        tight_loop_contents();
    }
}