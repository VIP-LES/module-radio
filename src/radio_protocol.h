#ifndef RADIO_PROTOCOL_H
#define RADIO_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RADIO_PROTOCOL_SYNC_BYTE 0xA5
#define RADIO_PROTOCOL_VERSION 1

#define RADIO_MSG_SENSOR_GPS 0x01
#define RADIO_MSG_EFM 0x02
#define RADIO_MSG_COMMAND 0x03 // cutdown

#define RADIO_MAX_PAYLOAD_SIZE 131U
#define RADIO_MAX_COMMAND_ARGS 16  // unsure about this num

#define RADIO_SENSOR_GPS_PAYLOAD_SIZE 131U
#define RADIO_EFM_PAYLOAD_SIZE 41U

/*
 * On-wire scalar encoding rules for all RF payloads serialized by
 * radio_protocol.c:
 *   - All multi-byte integers are little-endian.
 *   - bool is encoded as one byte: 0x00 for false, 0x01 for true.
 *   - float is encoded as IEEE-754 binary32 little-endian.
 *   - double is encoded as IEEE-754 binary64 little-endian.
 *
 * The outer RF frame layout is:
 *   [sync:1][version:1][message_type:1][sequence:1][payload_len:1]
 *   [payload:payload_len][crc16_ccitt_false:2 little-endian]
 *
 * sensor_gps payload layout (131 bytes total):
 *   [t_pkt_us:8]
 *   [bme688.humidity:4][bme688.pressure:4][bme688.temperature:4]
 *   [bme688.altitude:4][bme688.gas_resistance:4][bme688_valid:1]
 *   [tsl2591.light_lux:4][tsl2591.raw_visible:2]
 *   [tsl2591.raw_infrared:2][tsl2591.raw_full_spectrum:4]
 *   [tsl2591_valid:1]
 *   [ltr390.uvs:4][ltr390_valid:1]
 *   [pmsa003i.pm10_env:4][pmsa003i.pm25_env:4]
 *   [pmsa003i.pm100_env:4][pmsa003i.aqi_pm25_us:4]
 *   [pmsa003i.aqi_pm100_us:4][pmsa003i.particles_03um:4]
 *   [pmsa003i.particles_05um:4][pmsa003i.particles_10um:4]
 *   [pmsa003i.particles_25um:4][pmsa003i.particles_50um:4]
 *   [pmsa003i.particles_100um:4][pmsa003i_valid:1]
 *   [gps.fix_ok:1][gps.lat:8][gps.lon:8][gps.alt_m:4]
 *   [gps.speed_mps:4][gps.track_deg:4][gps.sats_used:1]
 *   [gps.sats_visible:1][gps.gps_utc_us:8]
 *
 * efm payload layout (41 bytes total):
 *   [t_pkt_us:8]
 *   [efm_valid:1]
 *   [adc1_ch1_diff:4][adc1_ch2_sensing:4][adc1_ch3_reference:4]
 *   [adc1_ch4_breakbeam:4]
 *   [adc2_ch1_diff:4][adc2_ch2_sensing:4][adc2_ch3_reference:4]
 *   [adc2_ch4_breakbeam:4]
 */
typedef struct
{
    float humidity;
    float pressure;
    float temperature;
    float altitude;
    float gas_resistance;
} bme688_radio_frame_t;

typedef struct
{
    float light_lux;
    uint16_t raw_visible;
    uint16_t raw_infrared;
    uint32_t raw_full_spectrum;
} tsl2591_radio_frame_t;

typedef struct
{
    uint32_t uvs;
} ltr390_radio_frame_t;

typedef struct
{
    uint32_t pm10_env;
    uint32_t pm25_env;
    uint32_t pm100_env;
    uint32_t aqi_pm25_us;
    uint32_t aqi_pm100_us;
    uint32_t particles_03um;
    uint32_t particles_05um;
    uint32_t particles_10um;
    uint32_t particles_25um;
    uint32_t particles_50um;
    uint32_t particles_100um;
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

typedef struct
{
    uint64_t t_pkt_us;
    bool valid;
    float adc1_ch1_diff;
    float adc1_ch2_sensing;
    float adc1_ch3_reference;
    float adc1_ch4_breakbeam;
    float adc2_ch1_diff;
    float adc2_ch2_sensing;
    float adc2_ch3_reference;
    float adc2_ch4_breakbeam;
} efm_radio_frame_t;

// typedef struct {
//  cutdown placeholder for now
// } command_radio_frame_t;

typedef struct
{
    uint8_t command_id;
    uint8_t command_arg_len;
    uint8_t command_args[RADIO_MAX_COMMAND_ARGS];
} command_radio_frame_t;

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
