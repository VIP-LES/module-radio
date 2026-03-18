#include "radio.h"
#include "config.h"

#include "leos/sx126x.h"
#include "leos/log.h"

int radio_init(void)
{
    leos_radio_config_t sx1262_cfg;
    leos_radio_config_t sx1268_cfg;

    config_build_sx1262(&sx1262_cfg);
    config_build_sx1268(&sx1268_cfg);

    int r1 = leos_sx126x_init(LEOS_RADIO_SX1262, &sx1262_cfg);
    if (r1 != 0)
    {
        LOG_ERROR("SX1262 init failed");
        return -1;
    }

    int r2 = leos_sx126x_init(LEOS_RADIO_SX1268, &sx1268_cfg);
    if (r2 != 0)
    {
        LOG_ERROR("SX1268 init failed");
        return -2;
    }

    LOG_INFO("Radios initialized");
    return 0;
}

void radio_handle_dio1_irq_sx1262(void)
{
    leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1262);
}

void radio_handle_dio1_irq_sx1268(void)
{
    leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1268);
}

void radio_service_irqs(void)
{
    leos_sx126x_process_irq(LEOS_RADIO_SX1262);
    leos_sx126x_process_irq(LEOS_RADIO_SX1268);
}

int radio_send_sx1262(const uint8_t *buf, size_t len)
{
    return leos_sx126x_send(LEOS_RADIO_SX1262, buf, len);
}

int radio_send_sx1268(const uint8_t *buf, size_t len)
{
    return leos_sx126x_send(LEOS_RADIO_SX1268, buf, len);
}

bool radio_sx1262_packet_available(void)
{
    return leos_sx126x_packet_available(LEOS_RADIO_SX1262);
}

int radio_read_sx1262(
    uint8_t *buf,
    size_t buf_size,
    size_t *out_len,
    leos_radio_packet_info_t *info)
{
    return leos_sx126x_read(
        LEOS_RADIO_SX1262,
        buf,
        buf_size,
        out_len,
        info);
}

int radio_enter_command_rx_mode(void)
{
    return leos_sx126x_start_rx(LEOS_RADIO_SX1262);
}