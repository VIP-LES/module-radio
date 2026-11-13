#include "radio_protocol.h"

#include <string.h>

#include "cyphal_config.h"   /* RADIO_CALLSIGN */
#include "pb_encode.h"
#include "packet.pb.h"       /* generated from leos-protobufs (LEOS_PROTOBUF_PATH) */

_Static_assert(leos_Packet_size <= RADIO_RF_MAX_PACKET_SIZE,
               "leos.Packet can exceed one LoRa packet (255 bytes)");

static uint32_t s_seq;

size_t radio_protocol_pack_lowrate(const lowrate_radio_frame_t *f, uint8_t *out, size_t out_size)
{
    if (f == NULL || out == NULL) {
        return 0U;
    }

    leos_Packet p = leos_Packet_init_zero;
    strncpy(p.callsign, RADIO_CALLSIGN, sizeof(p.callsign) - 1U);
    p.callsign[sizeof(p.callsign) - 1U] = '\0';
    p.sequence     = s_seq++;
    p.timestamp_ms = f->t_pkt_us / 1000U;      /* uptime in ms, not UTC (per packet.proto) */
    p.which_payload = leos_Packet_telemetry_tag;

    leos_TelemetryPacket *t = &p.payload.telemetry;

    /* Purpleboard sensors plus air quality. */
    t->has_purple = true;
    t->purple.temperature    = f->temperature_c;
    t->purple.pressure       = f->pressure_pa;
    t->purple.humidity       = f->humidity;
    t->purple.uvi            = f->uvs;           /* raw counts; convert to UVI if needed */
    t->purple.light_lux      = f->light_lux;
    t->purple.gas_resistance = f->gas_resistance;
    t->purple.has_air = true;
    t->purple.air.pm10_env     = f->pm10_env;
    t->purple.air.pm25_env     = f->pm25_env;
    t->purple.air.pm100_env    = f->pm100_env;
    t->purple.air.aqi_pm25_us  = f->aqi_pm25_us;
    t->purple.air.aqi_pm100_us = f->aqi_pm100_us;

    /* Beacon / GPS. */
    t->has_beacon = true;
    t->beacon.has_gps = true;
    t->beacon.gps.lat         = f->lat;
    t->beacon.gps.lon         = f->lon;
    t->beacon.gps.alt_m       = f->alt_m;
    t->beacon.gps.satellites  = f->sats_used;
    t->beacon.gps.fix         = f->gps_fix_ok ? 3U : 1U;   /* 1=no fix, 3=3D; 2D not distinguishable */
    t->beacon.gps.climb_mps   = 0.0f;                      /* not in LowRate */
    t->beacon.gps.hdop        = 0.0f;                      /* not in LowRate */
    t->beacon.gps.utc_unix_ms = f->gps_utc_us / 1000U;
    /* beacon.health left unset: nothing in LowRate reports subsystem health. */

    /* Electric field mills. */
    if (f->efm[0].present) {
        t->has_efm0 = true;
        t->efm0.motor_frequency = f->efm[0].motor_frequency;
    }
    if (f->efm[1].present) {
        t->has_efm1 = true;
        t->efm1.motor_frequency = f->efm[1].motor_frequency;
    }

    /* Geiger counter. */
    t->has_geiger = true;
    t->geiger.hits_in_interval = f->geiger_hits;

    pb_ostream_t s = pb_ostream_from_buffer(out, out_size);
    if (!pb_encode(&s, leos_Packet_fields, &p)) {
        return 0U;
    }
    return s.bytes_written;
}
