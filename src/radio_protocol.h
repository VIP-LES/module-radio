#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Max bytes per LoRa packet (SX126x payload limit). */
#define RADIO_RF_MAX_PACKET_SIZE 255u

/* One interval summary from an electric field mill (leos.EfmData). */
typedef struct {
    bool     present;           /* false until a valid reading has been seen */
    float    motor_frequency;
} efm_interval_t;

/* In-memory frame the bridge fills; radio_protocol.c encodes it into the team's leos.Packet
 * (leos-protobufs, luke branch: TelemetryPacket with purple + beacon). */
typedef struct {
    uint64_t t_pkt_us;          /* radio uptime when forwarded */

    /* Purpleboard (units the schema uses) */
    float    temperature_c;     /* degrees C */
    float    pressure_pa;       /* Pa */
    float    humidity;
    float    gas_resistance;
    float    light_lux;
    uint32_t uvs;               /* raw LTR390 counts -> schema field "uvi" */

    /* Air quality (PMSA003I). env = concentration in ug/m3; aqi values are the US AQI. */
    uint32_t pm10_env;          /* PM1.0 */
    uint32_t pm25_env;          /* PM2.5 */
    uint32_t pm100_env;         /* PM10 */
    uint32_t aqi_pm25_us;
    uint32_t aqi_pm100_us;

    /* Electric field mills: efm[0] -> TelemetryPacket.efm0, efm[1] -> efm1 */
    efm_interval_t efm[2];

    /* Geiger counter (leos.GeigerData.hits_in_interval) */
    uint32_t geiger_hits;

    /* GPS */
    bool     gps_fix_ok;
    double   lat;
    double   lon;
    float    alt_m;
    uint8_t  sats_used;
    uint64_t gps_utc_us;
} lowrate_radio_frame_t;

/* Return encoded protobuf length, or 0 on failure / buffer too small. */
size_t radio_protocol_pack_lowrate(const lowrate_radio_frame_t *f, uint8_t *out, size_t out_size);
