#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Max bytes per LoRa packet (SX126x payload limit). */
#define RADIO_RF_MAX_PACKET_SIZE 255u

/* In-memory frames the bridge fills. They hold ONLY what the team's
 * leos.Packet schema (leos-protobufs: packet/telemetry/purpleboard/gps/efm)
 * can carry, in the units that schema uses. radio_protocol.c encodes them. */

typedef struct {
    uint64_t t_pkt_us;          /* radio uptime when forwarded */
    float    temperature_c;     /* degrees C */
    float    pressure_pa;       /* Pa */
    float    humidity;
    float    gas_resistance;
    float    light_lux;
    uint32_t uvs;               /* raw LTR390 counts -> schema field "uvi" */
    bool     gps_fix_ok;
    double   lat;
    double   lon;
    float    alt_m;
    uint8_t  sats_used;
    uint64_t gps_utc_us;
} lowrate_radio_frame_t;

typedef struct {
    uint64_t t_pkt_us;
    float adc1_ch1_diff;
    float adc1_ch4_breakbeam;
    float adc2_ch1_diff;
    float adc2_ch4_breakbeam;
} efm_radio_frame_t;

/* Return encoded protobuf length, or 0 on failure / buffer too small. */
size_t radio_protocol_pack_lowrate(const lowrate_radio_frame_t *f, uint8_t *out, size_t out_size);
size_t radio_protocol_pack_efm(const efm_radio_frame_t *f, uint8_t *out, size_t out_size);