#include "radio_protocol.h"

#include <string.h>

#include "cyphal_config.h"   /* RADIO_CALLSIGN */
#include "pb_encode.h"
#include "packet.pb.h"       /* generated from leos-protobufs (LEOS_PROTOBUF_PATH) */

static uint32_t s_seq;

static void fill_header(leos_Packet *p, uint64_t t_pkt_us)
{
    strncpy(p->callsign, RADIO_CALLSIGN, sizeof(p->callsign) - 1U);
    p->callsign[sizeof(p->callsign) - 1U] = '\0';
    p->sequence = s_seq++;
    p->timestamp_ms = t_pkt_us / 1000U;   /* uptime in ms, not UTC (per packet.proto) */
}

static size_t encode(const leos_Packet *p, uint8_t *out, size_t out_size)
{
    pb_ostream_t s = pb_ostream_from_buffer(out, out_size);
    if (!pb_encode(&s, leos_Packet_fields, p)) {
        return 0U;
    }
    return s.bytes_written;
}

size_t radio_protocol_pack_lowrate(const lowrate_radio_frame_t *f, uint8_t *out, size_t out_size)
{
    if (f == NULL || out == NULL) {
        return 0U;
    }

    leos_Packet p = leos_Packet_init_zero;
    fill_header(&p, f->t_pkt_us);
    p.which_payload = leos_Packet_telemetry_tag;
    p.payload.telemetry.which_data = leos_TelemetryPacket_low_rate_tag;

    leos_LowRateTelemetry *low = &p.payload.telemetry.data.low_rate;

    /* Purpleboard sensors (units already converted by the bridge). */
    low->has_sensors = true;
    low->sensors.temperature    = f->temperature_c;
    low->sensors.pressure       = f->pressure_pa;
    low->sensors.humidity       = f->humidity;
    low->sensors.uvi            = f->uvs;       /* raw counts; convert to UVI if needed */
    low->sensors.light_lux      = f->light_lux;
    low->sensors.gas_resistance = f->gas_resistance;

    /* Beacon / GPS. */
    low->has_beacon = true;
    low->beacon.has_gps = true;
    low->beacon.gps.lat         = f->lat;
    low->beacon.gps.lon         = f->lon;
    low->beacon.gps.alt_m       = f->alt_m;
    low->beacon.gps.satellites  = f->sats_used;
    low->beacon.gps.fix         = f->gps_fix_ok ? 3U : 1U;   /* 1=no fix, 3=3D; 2D not distinguishable */
    low->beacon.gps.climb_mps   = 0.0f;                      /* not in LowRate */
    low->beacon.gps.hdop        = 0.0f;                      /* not in LowRate */
    low->beacon.gps.utc_unix_ms = f->gps_utc_us / 1000U;
    /* beacon.health left unset: nothing in LowRate reports subsystem health. */

    return encode(&p, out, out_size);
}

size_t radio_protocol_pack_efm(const efm_radio_frame_t *f, uint8_t *out, size_t out_size)
{
    if (f == NULL || out == NULL) {
        return 0U;
    }

    leos_Packet p = leos_Packet_init_zero;
    fill_header(&p, f->t_pkt_us);
    p.which_payload = leos_Packet_telemetry_tag;
    p.payload.telemetry.which_data = leos_TelemetryPacket_efm_tag;

    leos_EfmReadings *efm = &p.payload.telemetry.data.efm;
    efm->adc_count = 2;

    /* ADC_0_2 only carries ch1_diff and ch4_breakbeam per ADC, so the schema's
     * ch2_phase1 / ch3_phase2 stay at 0 (not transmitted by proto3). */
    efm->adc[0].id            = 1;
    efm->adc[0].ch1_diff      = f->adc1_ch1_diff;
    efm->adc[0].ch4_breakbeam = f->adc1_ch4_breakbeam;

    efm->adc[1].id            = 2;
    efm->adc[1].ch1_diff      = f->adc2_ch1_diff;
    efm->adc[1].ch4_breakbeam = f->adc2_ch4_breakbeam;

    return encode(&p, out, out_size);
}