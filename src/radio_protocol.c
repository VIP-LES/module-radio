#include "radio_protocol.h"

#include <string.h>

static uint8_t g_sensor_gps_sequence = 0U;
static uint8_t g_efm_sequence = 0U;

// ALL SERIALIZERS ARE LITTLE-ENDIAN

static bool radio_protocol_write_u8(
    uint8_t value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    if ((buf == NULL) || (offset == NULL) || (*offset >= buf_size))
    {
        return false;
    }

    buf[*offset] = value;
    *offset += 1U;
    return true;
}

static bool radio_protocol_write_bool(
    bool value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    return radio_protocol_write_u8(value ? 1U : 0U, buf, buf_size, offset);
}

static bool radio_protocol_write_u16_le(
    uint16_t value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    if ((buf == NULL) || (offset == NULL) || ((buf_size - *offset) < 2U))
    {
        return false;
    }

    buf[*offset + 0U] = (uint8_t)(value & 0xFFU);
    buf[*offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    *offset += 2U;
    return true;
}

static bool radio_protocol_write_u32_le(
    uint32_t value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    if ((buf == NULL) || (offset == NULL) || ((buf_size - *offset) < 4U))
    {
        return false;
    }

    buf[*offset + 0U] = (uint8_t)(value & 0xFFU);
    buf[*offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    buf[*offset + 2U] = (uint8_t)((value >> 16) & 0xFFU);
    buf[*offset + 3U] = (uint8_t)((value >> 24) & 0xFFU);
    *offset += 4U;
    return true;
}

static bool radio_protocol_write_u64_le(
    uint64_t value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    if ((buf == NULL) || (offset == NULL) || ((buf_size - *offset) < 8U))
    {
        return false;
    }

    buf[*offset + 0U] = (uint8_t)(value & 0xFFU);
    buf[*offset + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    buf[*offset + 2U] = (uint8_t)((value >> 16) & 0xFFU);
    buf[*offset + 3U] = (uint8_t)((value >> 24) & 0xFFU);
    buf[*offset + 4U] = (uint8_t)((value >> 32) & 0xFFU);
    buf[*offset + 5U] = (uint8_t)((value >> 40) & 0xFFU);
    buf[*offset + 6U] = (uint8_t)((value >> 48) & 0xFFU);
    buf[*offset + 7U] = (uint8_t)((value >> 56) & 0xFFU);
    *offset += 8U;
    return true;
}

static bool radio_protocol_write_float_le(
    float value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    uint32_t bits = 0U;
    memcpy(&bits, &value, sizeof(bits));
    return radio_protocol_write_u32_le(bits, buf, buf_size, offset);
}

static bool radio_protocol_write_double_le(
    double value,
    uint8_t *buf,
    size_t buf_size,
    size_t *offset)
{
    uint64_t bits = 0U;
    memcpy(&bits, &value, sizeof(bits));
    return radio_protocol_write_u64_le(bits, buf, buf_size, offset);
}

static bool radio_protocol_serialize_bme688(
    const bme688_radio_frame_t *frame,
    bool valid,
    uint8_t *payload_buf,
    size_t payload_buf_size,
    size_t *offset)
{
    if (frame == NULL)
    {
        return false;
    }

    return radio_protocol_write_float_le(frame->humidity, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->pressure, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->temperature, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->altitude, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->gas_resistance, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_bool(valid, payload_buf, payload_buf_size, offset);
}

static bool radio_protocol_serialize_tsl2591(
    const tsl2591_radio_frame_t *frame,
    bool valid,
    uint8_t *payload_buf,
    size_t payload_buf_size,
    size_t *offset)
{
    if (frame == NULL)
    {
        return false;
    }

    return radio_protocol_write_float_le(frame->light_lux, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u16_le(frame->raw_visible, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u16_le(frame->raw_infrared, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->raw_full_spectrum, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_bool(valid, payload_buf, payload_buf_size, offset);
}

static bool radio_protocol_serialize_ltr390(
    const ltr390_radio_frame_t *frame,
    bool valid,
    uint8_t *payload_buf,
    size_t payload_buf_size,
    size_t *offset)
{
    if (frame == NULL)
    {
        return false;
    }

    return radio_protocol_write_u32_le(frame->uvs, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_bool(valid, payload_buf, payload_buf_size, offset);
}

static bool radio_protocol_serialize_pmsa003i(
    const pmsa003i_radio_frame_t *frame,
    bool valid,
    uint8_t *payload_buf,
    size_t payload_buf_size,
    size_t *offset)
{
    if (frame == NULL)
    {
        return false;
    }

    return radio_protocol_write_u32_le(frame->pm10_env, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->pm25_env, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->pm100_env, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->aqi_pm25_us, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->aqi_pm100_us, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_03um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_05um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_10um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_25um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_50um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u32_le(frame->particles_100um, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_bool(valid, payload_buf, payload_buf_size, offset);
}

static bool radio_protocol_serialize_gps(
    const gps_radio_frame_t *frame,
    uint8_t *payload_buf,
    size_t payload_buf_size,
    size_t *offset)
{
    if (frame == NULL)
    {
        return false;
    }

    return radio_protocol_write_bool(frame->fix_ok, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_double_le(frame->lat, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_double_le(frame->lon, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->alt_m, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->speed_mps, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_float_le(frame->track_deg, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u8(frame->sats_used, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u8(frame->sats_visible, payload_buf, payload_buf_size, offset) &&
           radio_protocol_write_u64_le(frame->gps_utc_us, payload_buf, payload_buf_size, offset);
}

static size_t radio_protocol_serialize_sensor_gps_payload(
    const sensor_gps_radio_frame_t *frame,
    uint8_t *payload_buf,
    size_t payload_buf_size)
{
    size_t offset = 0U;

    if ((frame == NULL) || (payload_buf == NULL) || (payload_buf_size != RADIO_SENSOR_GPS_PAYLOAD_SIZE))
    {
        return 0U;
    }

    if (!radio_protocol_write_u64_le(frame->t_pkt_us, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_serialize_bme688(
            &frame->bme688,
            frame->bme688_valid,
            payload_buf,
            payload_buf_size,
            &offset) ||
        !radio_protocol_serialize_tsl2591(
            &frame->tsl2591,
            frame->tsl2591_valid,
            payload_buf,
            payload_buf_size,
            &offset) ||
        !radio_protocol_serialize_ltr390(
            &frame->ltr390,
            frame->ltr390_valid,
            payload_buf,
            payload_buf_size,
            &offset) ||
        !radio_protocol_serialize_pmsa003i(
            &frame->pmsa003i,
            frame->pmsa003i_valid,
            payload_buf,
            payload_buf_size,
            &offset) ||
        !radio_protocol_serialize_gps(
            &frame->gps_data,
            payload_buf,
            payload_buf_size,
            &offset))
    {
        return 0U;
    }

    if (offset != RADIO_SENSOR_GPS_PAYLOAD_SIZE)
    {
        return 0U;
    }

    return RADIO_SENSOR_GPS_PAYLOAD_SIZE;
}

static size_t radio_protocol_serialize_efm_payload(
    const efm_radio_frame_t *frame,
    uint8_t *payload_buf,
    size_t payload_buf_size)
{
    size_t offset = 0U;

    if ((frame == NULL) || (payload_buf == NULL) || (payload_buf_size != RADIO_EFM_PAYLOAD_SIZE))
    {
        return 0U;
    }

    if (!radio_protocol_write_u64_le(frame->t_pkt_us, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_bool(frame->valid, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc1_ch1_diff, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc1_ch2_sensing, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc1_ch3_reference, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc1_ch4_breakbeam, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc2_ch1_diff, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc2_ch2_sensing, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc2_ch3_reference, payload_buf, payload_buf_size, &offset) ||
        !radio_protocol_write_float_le(frame->adc2_ch4_breakbeam, payload_buf, payload_buf_size, &offset))
    {
        return 0U;
    }

    if (offset != RADIO_EFM_PAYLOAD_SIZE)
    {
        return 0U;
    }

    return RADIO_EFM_PAYLOAD_SIZE;
}

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

    out_buf[5U + payload_len] = (uint8_t)(crc & 0xFFU);
    out_buf[6U + payload_len] = (uint8_t)((crc >> 8) & 0xFFU);

    return total_len;
}

size_t radio_protocol_pack_sensor_gps_frame(
    const sensor_gps_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size)
{
    uint8_t payload_buf[RADIO_SENSOR_GPS_PAYLOAD_SIZE];
    const size_t serialized_len = radio_protocol_serialize_sensor_gps_payload(
        frame,
        payload_buf,
        sizeof(payload_buf));

    if (serialized_len != RADIO_SENSOR_GPS_PAYLOAD_SIZE)
    {
        return 0U;
    }

    return radio_protocol_pack_frame(
        RADIO_MSG_SENSOR_GPS,
        g_sensor_gps_sequence++,
        payload_buf,
        RADIO_SENSOR_GPS_PAYLOAD_SIZE,
        out_buf,
        out_buf_size);
}

size_t radio_protocol_pack_efm_frame(
    const efm_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size)
{
    uint8_t payload_buf[RADIO_EFM_PAYLOAD_SIZE];
    const size_t serialized_len = radio_protocol_serialize_efm_payload(
        frame,
        payload_buf,
        sizeof(payload_buf));

    if (serialized_len != RADIO_EFM_PAYLOAD_SIZE)
    {
        return 0U;
    }

    return radio_protocol_pack_frame(
        RADIO_MSG_EFM,
        g_efm_sequence++,
        payload_buf,
        RADIO_EFM_PAYLOAD_SIZE,
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
        (uint16_t)(((uint16_t)buf[len - 1U] << 8) |
                   ((uint16_t)buf[len - 2U]));

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
