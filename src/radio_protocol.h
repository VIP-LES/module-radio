#ifndef RADIO_PROTOCOL_H
#define RADIO_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RADIO_PROTOCOL_SYNC_BYTE 0xA5
#define RADIO_PROTOCOL_VERSION 1

#define RADIO_MSG_SENSOR_GPS 0x01
#define RADIO_MSG_EFM 0x02
#define RADIO_MSG_COMMAND 0x03 //cutdown


#define RADIO_MAX_PAYLOAD_SIZE 128 // unsure about this num
#define RADIO_MAX_COMMAND_ARGS 16 // unsure about this num


typedef struct
{
    uint32_t board_ms;
    float humidity;
    float pressure;
    float temperature;
    float altitude;
    float gas_resistance;
} bme688_radio_frame_t;

typedef struct
{
    uint32_t board_ms;
    float light_lux;
} tsl2591_radio_frame_t;

typedef struct
{
    uint32_t board_ms;
    uint16_t uvi;
} ltr390_radio_frame_t;

typedef struct
{
    uint32_t board_ms;
    uint32_t pm10_env;
    uint32_t pm25_env;
    uint32_t pm100_env;
    uint32_t aqi_pm25_us;
    uint32_t aqi_pm100_us;
} pmsa003i_radio_frame_t;

typedef struct
{
    bool fix_ok;
    double lat;
    double lon;
    float alt_m;
    float speed_mps;
    float track_deg;
    uint8_t sats_used;
    uint8_t sats_visible;
    uint64_t gps_utc_us;
} gps_radio_frame_t;

typedef struct
{
    uint64_t t_pkt_us;

    bme688_radio_frame_t bme688;
    bool bme688_valid;

    tsl2591_radio_frame_t tsl2591;
    bool tsl2591_valid;

    ltr390_radio_frame_t ltr390;
    bool ltr390_valid;

    pmsa003i_radio_frame_t pmsa003i;
    bool pmsa003i_valid;

    gps_radio_frame_t gps_data;
} sensor_gps_radio_frame_t;

typedef struct {
    uint32_t board_ms;
    int16_t raw[4];
    float volts[4];
} efm_radio_frame_t;

//typedef struct {
// cutdown placeholder for now
//} command_radio_frame_t;



size_t radio_protocol_pack_sensor_gps_frame(
    const sensor_gps_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size);

size_t radio_protocol_pack_efm_frame(
    const efm_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size);

bool radio_protocol_unpack_command_frame(
    const uint8_t *buf,
    size_t len,
    command_radio_frame_t *out_frame);

uint16_t radio_protocol_crc16_ccitt(
    const uint8_t *buf,
    size_t len);


#endif
