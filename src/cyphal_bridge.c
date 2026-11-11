#include "cyphal_bridge.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "leos/log.h"
#include "pico/time.h"
#include "radio.h"
#include "radio_protocol.h"

/* Last good readings. The flight computer's aggregate can arrive with a sensor block zeroed
 * (stale or missing sensor). Forwarding those zeros makes dashboards drop to 0 (and temperature to
 * -273 C), so a block that is not valid keeps its previous good value instead. */
static struct {
    bool     bme_ok;                       /* a good BME688 reading has been seen */
    float    temperature_c, pressure_pa, humidity, gas_resistance;
    float    light_lux;
    uint32_t uvs;
    uint32_t pm10_env, pm25_env, pm100_env, aqi_pm25_us, aqi_pm100_us;
    efm_interval_t efm[2];
} s_hold;
static uint32_t s_bme_stale, s_tsl_stale, s_ltr_stale, s_pm_stale, s_efm_stale;   /* counts of held (not updated) blocks */

/* Field mapping from the generated DSDL structs to the protobuf-ready frames.
 * Only fields that leos.Packet can carry are kept.
 * Returns false if there is no usable BME688 reading yet, in which case nothing is sent. */
static bool lowrate_to_frame(const flight_lowrate_dsdl_t *m, lowrate_radio_frame_t *o)
{
    /* BME688: 0 K or 0 Pa cannot be a real reading, so treat it as missing. */
    if (m->bme688.temperature.kelvin > 1.0f && m->bme688.pressure.pascal > 0.0f) {
        s_hold.bme_ok         = true;
        s_hold.temperature_c  = m->bme688.temperature.kelvin - 273.15f;   /* schema uses degrees C */
        s_hold.pressure_pa    = m->bme688.pressure.pascal;
        s_hold.humidity       = m->bme688.humidity;
        s_hold.gas_resistance = m->bme688.gas_resistance;
    } else {
        s_bme_stale++;
    }
    /* TSL2591 and LTR390 can legitimately read 0, so trust their own valid flags. */
    if (m->tsl2591.valid) {
        s_hold.light_lux = m->tsl2591.light_lux;
    } else {
        s_tsl_stale++;
    }
    if (m->ltr390.valid) {
        s_hold.uvs = m->ltr390.uvs;
    } else {
        s_ltr_stale++;
    }
    /* PMSA003I (air quality): trust its valid flag; 0 ug/m3 can be real. */
    if (m->pmsa003i.valid) {
        s_hold.pm10_env     = m->pmsa003i.pm10_env;
        s_hold.pm25_env     = m->pmsa003i.pm25_env;
        s_hold.pm100_env    = m->pmsa003i.pm100_env;
        s_hold.aqi_pm25_us  = m->pmsa003i.aqi_pm25_us;
        s_hold.aqi_pm100_us = m->pmsa003i.aqi_pm100_us;
    } else {
        s_pm_stale++;
    }
    /* Electric field mills: array of two, index 0 -> efm0, index 1 -> efm1. */
    for (int i = 0; i < 2; i++) {
        const leos_aggregate_EFMAggregate_0_1 *e = &m->efm_aggregate[i];
        if (e->valid) {
            s_hold.efm[i].present                 = true;
            s_hold.efm[i].avg_sample              = e->avg_sample;
            s_hold.efm[i].min_sample              = e->min_sample;
            s_hold.efm[i].max_sample              = e->max_sample;
            s_hold.efm[i].hall_events_in_interval = e->hall_events_in_interval;
        } else {
            s_efm_stale++;
        }
    }
    if (!s_hold.bme_ok) {
        return false;
    }

    memset(o, 0, sizeof(*o));
    o->t_pkt_us       = (uint64_t)to_us_since_boot(get_absolute_time());
    o->temperature_c  = s_hold.temperature_c;
    o->pressure_pa    = s_hold.pressure_pa;
    o->humidity       = s_hold.humidity;
    o->gas_resistance = s_hold.gas_resistance;
    o->light_lux      = s_hold.light_lux;
    o->uvs            = s_hold.uvs;
    o->pm10_env       = s_hold.pm10_env;
    o->pm25_env       = s_hold.pm25_env;
    o->pm100_env      = s_hold.pm100_env;
    o->aqi_pm25_us    = s_hold.aqi_pm25_us;
    o->aqi_pm100_us   = s_hold.aqi_pm100_us;
    o->efm[0]         = s_hold.efm[0];
    o->efm[1]         = s_hold.efm[1];
    o->geiger_hits    = m->geiger_aggregate.counts_in_interval;   /* no valid flag in the type */

    o->gps_fix_ok     = m->gps_data.fix_ok;
    o->lat            = m->gps_data.lat;
    o->lon            = m->gps_data.lon;
    o->alt_m          = m->gps_data.alt_m;
    o->sats_used      = m->gps_data.sats_used;
    o->gps_utc_us     = m->gps_data.gps_utc.microsecond;
    return true;
}

/* Latest decoded frames, handed from the callbacks to cyphal_bridge_service(). */
static lowrate_radio_frame_t s_low;
static volatile bool         s_low_pending;
static uint32_t              s_low_overwritten;

/* Debug counters, printed by cyphal_bridge_log_stats(). */
static struct {
    uint32_t rx, dec_fail, tx_ok, tx_fail;
    uint32_t last_rx_ms;           /* 0 = never */
} s_ls;
static uint32_t s_pack_fail;

static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }

void cyphal_bridge_on_lowrate(struct CanardRxTransfer *transfer, void *user_reference)
{
    (void)user_reference;
    if (transfer == NULL) {
        return;
    }
    s_ls.rx++;
    s_ls.last_rx_ms = now_ms() | 1u;
    LOG_INFO("rx LowRate #%u: %u bytes (extent %u)", (unsigned)s_ls.rx,
             (unsigned)transfer->payload.size, (unsigned)CYPHAL_SUB_LOWRATE_EXTENT);

    flight_lowrate_dsdl_t msg;
    memset(&msg, 0, sizeof(msg));
    size_t size = transfer->payload.size;
    const int8_t rc = leos_aggregate_LowRate_0_1_deserialize_(
        &msg, (const uint8_t *)transfer->payload.data, &size);
    if (rc < 0) {
        s_ls.dec_fail++;
        LOG_ERROR("LowRate deserialize failed: %d (payload %u bytes)", (int)rc,
                  (unsigned)transfer->payload.size);
        return;
    }
    LOG_INFO("LowRate valid flags: bme=%d tsl=%d ltr=%d pm=%d | raw T=%dK P=%uPa",
             (int)msg.bme688.valid, (int)msg.tsl2591.valid, (int)msg.ltr390.valid,
             (int)msg.pmsa003i.valid,
             (int)msg.bme688.temperature.kelvin, (unsigned)msg.bme688.pressure.pascal);
    lowrate_radio_frame_t frame;
    if (!lowrate_to_frame(&msg, &frame)) {
        LOG_ERROR("LowRate has no valid BME688 reading yet; not sending");
        return;
    }
    if (s_low_pending) {
        s_low_overwritten++;
    }
    s_low = frame;
    s_low_pending = true;
}

static void send_frame(const uint8_t *buf, size_t len)
{
    const int tx = radio_send(buf, len);
    if (tx != 0) {
        s_ls.tx_fail++;
        LOG_ERROR("radio_send failed: %d", tx);
    } else {
        s_ls.tx_ok++;
        LOG_INFO("tx %u bytes OK", (unsigned)len);
    }
}

void cyphal_bridge_service(void)
{
    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];

    if (s_low_pending) {
        lowrate_radio_frame_t frame = s_low;
        s_low_pending = false;
        const size_t len = radio_protocol_pack_lowrate(&frame, buf, sizeof(buf));
        if (len == 0U) {
            s_pack_fail++;
            LOG_ERROR("pack lowrate failed");
        } else {
            /* Integer formatting only, so this works without float printf. */
            LOG_INFO("LowRate: T=%dC P=%uPa RH=%d%% lux=%d | GPS fix=%u sats=%u lat=%d.%04u lon=%d alt=%dm | PM1=%u PM2.5=%u PM10=%u AQI25=%u",
                     (int)frame.temperature_c, (unsigned)frame.pressure_pa, (int)frame.humidity,
                     (int)frame.light_lux, frame.gps_fix_ok ? 3U : 1U, (unsigned)frame.sats_used,
                     (int)frame.lat, (unsigned)((frame.lat - (int)frame.lat) * (frame.lat < 0 ? -10000 : 10000)),
                     (int)frame.lon, (int)frame.alt_m,
                     (unsigned)frame.pm10_env, (unsigned)frame.pm25_env, (unsigned)frame.pm100_env,
                     (unsigned)frame.aqi_pm25_us);
            LOG_INFO("EFM0 avg=%d min=%d max=%d hall=%u | EFM1 avg=%d min=%d max=%d hall=%u | geiger=%u",
                     (int)frame.efm[0].avg_sample, (int)frame.efm[0].min_sample, (int)frame.efm[0].max_sample,
                     (unsigned)frame.efm[0].hall_events_in_interval,
                     (int)frame.efm[1].avg_sample, (int)frame.efm[1].min_sample, (int)frame.efm[1].max_sample,
                     (unsigned)frame.efm[1].hall_events_in_interval, (unsigned)frame.geiger_hits);
            send_frame(buf, len);
        }
        return;   /* one packet per call keeps the main loop responsive */
    }
}

static void age(char *out, size_t n, uint32_t last_ms)
{
    if (last_ms == 0U) {
        snprintf(out, n, "never");
    } else {
        snprintf(out, n, "%us ago", (unsigned)((now_ms() - (last_ms & ~1u)) / 1000U));
    }
}

void cyphal_bridge_log_stats(void)
{
    char a[16];
    age(a, sizeof(a), s_ls.last_rx_ms);
    LOG_INFO("STATS up=%us | LowRate rx=%u decfail=%u tx=%u txfail=%u skipped=%u last=%s",
             (unsigned)(now_ms() / 1000U), (unsigned)s_ls.rx, (unsigned)s_ls.dec_fail,
             (unsigned)s_ls.tx_ok, (unsigned)s_ls.tx_fail, (unsigned)s_low_overwritten, a);
    LOG_INFO("STATS      | held stale blocks: bme=%u tsl=%u ltr=%u pm=%u efm=%u",
             (unsigned)s_bme_stale, (unsigned)s_tsl_stale, (unsigned)s_ltr_stale, (unsigned)s_pm_stale,
             (unsigned)s_efm_stale);
    LOG_INFO("STATS      | packfail=%u", (unsigned)s_pack_fail);
    if (s_ls.rx == 0U) {
        LOG_ERROR("STATS: no LowRate received yet (check CAN wiring, bitrate, fc-lowrate-aggregate on the FC)");
    }
}