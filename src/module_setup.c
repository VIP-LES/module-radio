#include "module_setup.h"

#include "config.h"
#include "radio.h"

#include "hardware/gpio.h"
#include "leos/cyphal/node.h"
#include "leos/cyphal/transport/mcp251xfd.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "leos/mcp251xfd/debug.h"
#include "pico/stdlib.h"

static void mcp_read_pending_cb(MCP251XFD *dev, void *node_ref)
{
    (void)dev;

    leos_cyphal_node_t *node = (leos_cyphal_node_t *)node_ref;
    leos_cyphal_rx_process(node);
}

static void radio_dio1_irq_cb(uint gpio, uint32_t events)
{
    (void)events;

    if (gpio == LEOS_SX1262_PIN_DIO1)
    {
        radio_handle_dio1_irq_sx1262();
    }
    else if (gpio == LEOS_SX1268_PIN_DIO1)
    {
        radio_handle_dio1_irq_sx1268();
    }
}

int module_setup_init(MCP251XFD *dev, leos_cyphal_node_t *node)
{
    eERRORRESULT err = leos_mcp251xfd_init(dev, &can_hw_config, &can_config, true);
    if (err != ERR_OK)
    {
        LOG_ERROR("Failed to init MCP251XFD: %s", mcp251xfd_debug_error_reason(err));
        return -1;
    }

    leos_mcp251xfd_set_rx_handler(dev, mcp_read_pending_cb, node);

    leos_cyphal_transport_t transport = leos_cyphal_transport_mcp251xfd(dev);
    leos_cyphal_result_t cyphal_result = leos_cyphal_init(node, transport, CYPHAL_NODE_ID);
    if (cyphal_result != LEOS_CYPHAL_OK)
    {
        LOG_ERROR("Failed to initialize Cyphal/Libcanard: %d", cyphal_result);
        return -2;
    }

    int radio_result = radio_init();
    if (radio_result != 0)
    {
        LOG_ERROR("Failed to initialize radios: %d", radio_result);
        return -3;
    }

    gpio_set_irq_enabled_with_callback(
        LEOS_SX1262_PIN_DIO1,
        GPIO_IRQ_EDGE_RISE,
        true,
        &radio_dio1_irq_cb);

    gpio_set_irq_enabled(
        LEOS_SX1268_PIN_DIO1,
        GPIO_IRQ_EDGE_RISE,
        true);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    LOG_INFO("Module setup complete");
    return 0;
}