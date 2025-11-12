#include "module_setup.h"
#include "config.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "leos/cyphal/node.h"
#include "leos/cyphal/transport/mcp251xfd.h"
#include "leos/log.h"
#include "leos/mcp251xfd.h"
#include "leos/mcp251xfd/debug.h"
#include "pico/stdlib.h"

void mcp_read_pending_cb(MCP251XFD* dev, void* node_ref)
{
    leos_cyphal_node_t* node = (leos_cyphal_node_t*)node_ref;
    leos_cyphal_rx_process(node);
}

bool read_uart_response(const char* expected_response, uint timeout_ms)
{
    char buffer[32]; // Small buffer for "OK\r" or "ERROR\r"
    int i = 0;

    absolute_time_t end_time = make_timeout_time_ms(timeout_ms);

    while (absolute_time_diff_us(get_absolute_time(), end_time) / 1000 > 0) {
        if (uart_is_readable(RADIO_UART_ID)) {
            char c = uart_getc(RADIO_UART_ID);
            if (c == '\r') {
                buffer[i] = '\0'; // Null-terminate the string
                LOG_INFO("XBee responded: \"%s\"\n", buffer);
                // Check if the buffer starts with the expected response
                return (strncmp(buffer, expected_response, strlen(expected_response)) == 0);
            } else if (i < (sizeof(buffer) - 1)) {
                buffer[i++] = c;
            }
        }
    }

    LOG_INFO("XBee response timed out.\n");
    return false; // Timeout
}

bool enter_command_mode()
{
    LOG_INFO("Entering AT Command Mode...\n");

    // Guard time
    sleep_ms(1100);

    // Send command sequence
    uart_puts(RADIO_UART_ID, "+++");

    sleep_ms(1100);

    // Wait for "OK\r"
    bool success = read_uart_response("OK\n", 1100);

    if (!success) {
        LOG_CRITICAL("Failed to enter AT Command Mode. Is XBee connected and powered?\n");
        // Clear UART buffer in case "+++" was echoed
        while (uart_is_readable_within_us(RADIO_UART_ID, 100)) {
            (void)uart_getc(RADIO_UART_ID);
        }
    }

    return success;
}

// Sends an AT command and waits for "OK"
bool send_at_command(const char* command, const char* param)
{
    char full_cmd[64];
    if (param) {
        snprintf(full_cmd, sizeof(full_cmd), "%s%s\r", command, param);
        LOG_INFO("Sending: %s%s\n", command, param);
    } else {
        snprintf(full_cmd, sizeof(full_cmd), "%s\r", command);
        LOG_INFO("Sending: %s\n", command);
    }

    uart_puts(RADIO_UART_ID, full_cmd);

    bool success = read_uart_response("OK", 1000);
    if (!success) {
        LOG_CRITICAL("Command AT%s failed.\n", command);
    }
    return success;
}

int init_module(MCP251XFD* dev, leos_cyphal_node_t* node)
{
    // Setup CANBus Communication
    eERRORRESULT err;
    err = leos_mcp251xfd_init(dev, &can_hw_config, &can_config, true);
    if (err != ERR_OK) {
        LOG_ERROR("Failed to init MCP251XFD: %s", mcp251xfd_debug_error_reason(err));
        return -1;
    }
    leos_cyphal_transport_t transport = leos_cyphal_transport_mcp251xfd(dev);
    leos_cyphal_result_t can_result;
    can_result = leos_cyphal_init(node, transport, 12);
    if (can_result != LEOS_CYPHAL_OK) {
        LOG_ERROR("Failed to initialize Cyphal/Libcanard: %d", can_result);
        return -2;
    }
    // Attach CANBus receive handler
    leos_mcp251xfd_set_rx_handler(dev, mcp_read_pending_cb, node);

    uart_init(RADIO_UART_ID, RADIO_UART_BAUD_RATE);
    gpio_set_function(RADIO_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RADIO_UART_RX_PIN, GPIO_FUNC_UART);

    // radio modules default to no flow control.
    uart_set_hw_flow(RADIO_UART_ID, false, false);
    LOG_INFO("RADIO UART initialized at %d baud", RADIO_UART_BAUD_RATE);

    if (enter_command_mode()) {
        LOG_INFO("Successfully entered AT Command Mode.\n");

        // Set Network ID
        send_at_command("ATID ", RADIO_NETWORK_ID);

        // Set Destination Address High
        send_at_command("ATDH ", RADIO_DEST_ADDR_H);

        // Set Destination Address Low
        send_at_command("ATDL ", RADIO_DEST_ADDR_L);

        // Write settings to non-volatile memory
        LOG_INFO("Saving settings to XBee flash (ATWR)...\n");
        send_at_command("ATWR", NULL);

        // Exit Command Mode
        send_at_command("ATCN", NULL);

        LOG_INFO("Configuration complete and saved.\n");

    } else {
        LOG_CRITICAL("Could not configure XBee. Will try to send on existing settings.\n");
    }

    // Setup board LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}