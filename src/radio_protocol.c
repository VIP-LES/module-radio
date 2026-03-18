#include "radio_protocol.h"

#include <string.h>

static uint8_t g_sensor_gps_sequence = 0U;
static uint8_t g_efm_sequence = 0U;

uint16_t radio_protocol_crc16_ccitt(
    const uint8_t *buf,
    size_t len)
{
    if (buf == NULL)
    {
        return 0U;
    }

    uint16_t crc = 0xFFFFU;

    for (size_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)((uint16_t)buf[i] << 8);

        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

static size_t radio_protocol_pack_frame(
    uint8_t message_type,
    uint8_t sequence,
    const uint8_t *payload,
    size_t payload_len,
    uint8_t *out_buf,
    size_t out_buf_size)
{
    if ((payload == NULL) || (out_buf == NULL))
    {
        return 0U;
    }

    if (payload_len > RADIO_MAX_PAYLOAD_SIZE)
    {
        return 0U;
    }

    if (payload_len > 255U)
    {
        return 0U;
    }

    const size_t total_len = 5U + payload_len + 2U;

    if (out_buf_size < total_len)
    {
        return 0U;
    }

    out_buf[0] = RADIO_PROTOCOL_SYNC_BYTE;
    out_buf[1] = RADIO_PROTOCOL_VERSION;
    out_buf[2] = message_type;
    out_buf[3] = sequence;
    out_buf[4] = (uint8_t)payload_len;

    memcpy(&out_buf[5], payload, payload_len);

    const uint16_t crc = radio_protocol_crc16_ccitt(out_buf, 5U + payload_len);

    out_buf[5U + payload_len] = (uint8_t)((crc >> 8) & 0xFFU);
    out_buf[6U + payload_len] = (uint8_t)(crc & 0xFFU);

    return total_len;
}

size_t radio_protocol_pack_sensor_gps_frame(
    const sensor_gps_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size)
{
    if (frame == NULL)
    {
        return 0U;
    }

    return radio_protocol_pack_frame(
        RADIO_MSG_SENSOR_GPS,
        g_sensor_gps_sequence++,
        (const uint8_t *)frame,
        sizeof(sensor_gps_radio_frame_t),
        out_buf,
        out_buf_size);
}

size_t radio_protocol_pack_efm_frame(
    const efm_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size)
{
    if (frame == NULL)
    {
        return 0U;
    }

    return radio_protocol_pack_frame(
        RADIO_MSG_EFM,
        g_efm_sequence++,
        (const uint8_t *)frame,
        sizeof(efm_radio_frame_t),
        out_buf,
        out_buf_size);
}

bool radio_protocol_unpack_command_frame(
    const uint8_t *buf,
    size_t len,
    command_radio_frame_t *out_frame)
{
    if ((buf == NULL) || (out_frame == NULL))
    {
        return false;
    }

  
    if (len < 7U)
    {
        return false;
    }

    if (buf[0] != RADIO_PROTOCOL_SYNC_BYTE)
    {
        return false;
    }

    if (buf[1] != RADIO_PROTOCOL_VERSION)
    {
        return false;
    }

    if (buf[2] != RADIO_MSG_COMMAND)
    {
        return false;
    }

    const uint8_t payload_len = buf[4];
    const size_t expected_len = 5U + (size_t)payload_len + 2U;

    if (len != expected_len)
    {
        return false;
    }

    const uint16_t rx_crc =
        (uint16_t)(((uint16_t)buf[len - 2U] << 8) |
                   ((uint16_t)buf[len - 1U]));

    const uint16_t calc_crc = radio_protocol_crc16_ccitt(buf, len - 2U);

    if (rx_crc != calc_crc)
    {
        return false;
    }

    
    if (payload_len < 2U)
    {
        return false;
    }

    const uint8_t *payload = &buf[5];

    const uint8_t command_id = payload[0];
    const uint8_t command_arg_len = payload[1];

    if (command_arg_len > RADIO_MAX_COMMAND_ARGS)
    {
        return false;
    }

    
    if (payload_len != (size_t)(2U + command_arg_len))
    {
        return false;
    }

    memset(out_frame, 0, sizeof(*out_frame));

    out_frame->command_id = command_id;
    out_frame->command_arg_len = command_arg_len;

    if (command_arg_len > 0U)
    {
        memcpy(out_frame->command_args, &payload[2], command_arg_len);
    }

    return true;
}