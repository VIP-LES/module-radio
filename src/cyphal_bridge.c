#include "cyphal_bridge.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "leos/log.h"
#include "pico/time.h"
#include "radio.h"
#include "radio_protocol.h"

static uint32_t s_bme_stale, s_tsl_stale, s_ltr_stale, s_pm_stale, s_efm_stale;

/* Field mapping from the generated DSDL structs to the protobuf-ready frames.
 * Invalid scalar sensor blocks remain zero because proto3 scalars have no null
 * representation. Invalid EFM messages are omitted via message presence. */
static void lowrate_to_frame(const flight_lowrate_dsdl_t *m, lowrate_radio_frame_t *o)
{
    memset(o, 0, sizeof(*o));
    o->t_pkt_us = (uint64_t)to_us_since_boot(get_absolute_time());

    if (m->bme688.valid) {
        o->temperature_c  = m->bme688.temperature.kelvin - 273.15f;
        o->pressure_pa    = m->bme688.pressure._pascal;
        o->humidity       = m->bme688.humidity;
        o->gas_resistance = m->bme688.gas_resistance;
    } else {
        s_bme_stale++;
    }
    if (m->tsl2591.valid) {
        o->light_lux = m->tsl2591.light_lux;
    } else {
        s_tsl_stale++;
    }
    if (m->ltr390.valid) {
        o->uvs = m->ltr390.uvs;
    } else {
        s_ltr_stale++;
    }
    if (m->pmsa003i.valid) {
        o->pm10_env     = m->pmsa003i.pm10_env;
        o->pm25_env     = m->pmsa003i.pm25_env;
        o->pm100_env    = m->pmsa003i.pm100_env;
        o->aqi_pm25_us  = m->pmsa003i.aqi_pm25_us;
        o->aqi_pm100_us = m->pmsa003i.aqi_pm100_us;
    } else {
        s_pm_stale++;
    }
    for (int i = 0; i < 2; i++) {
        const leos_aggregate_EFMAggregate_0_1 *e = &m->efm_aggregate[i];
        if (e->valid) {
            o->efm[i].present = true;
            o->efm[i].motor_frequency = e->motor_frequency;
        } else {
            s_efm_stale++;
        }
    }
    o->geiger_hits    = m->geiger_aggregate.counts_in_interval;   /* no valid flag in the type */

    o->gps_fix_ok     = m->gps_data.fix_ok;
    o->lat            = m->gps_data.lat;
    o->lon            = m->gps_data.lon;
    o->alt_m          = m->gps_data.alt_m;
    o->sats_used      = m->gps_data.sats_used;
    o->gps_utc_us     = m->gps_data.gps_utc.microsecond;

}

/* Latest decoded frame, handed from the callback to cyphal_bridge_service(). */
static lowrate_radio_frame_t s_low;
static volatile bool s_low_pending;
static uint32_t s_low_overwritten;

/* Debug counters, printed by cyphal_bridge_log_stats(). */
static struct {
    uint32_t rx, dec_fail, tx_ok, tx_fail;
    uint32_t last_rx_ms;           /* 0 = never */
} s_ls;
static uint32_t s_pack_fail;
static uint32_t s_tx_late;
static uint32_t s_tx_max_ms;

static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }

void cyphal_bridge_on_lowrate(struct CanardRxTransfer *transfer, void *user_reference)
{
    (void)user_reference;
    if (transfer == NULL) {
        return;
    }
    s_ls.rx++;
    s_ls.last_rx_ms = now_ms() | 1u;
    LOG_DEBUG("rx lowrate #%u %uB", (unsigned)s_ls.rx,
              (unsigned)transfer->payload.size);

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
    LOG_DEBUG("lowrate valid bme=%d tsl=%d ltr=%d pm=%d",
              (int)msg.bme688.valid, (int)msg.tsl2591.valid,
              (int)msg.ltr390.valid, (int)msg.pmsa003i.valid);
    lowrate_radio_frame_t frame;
    lowrate_to_frame(&msg, &frame);
    if (s_low_pending) {
        s_low_overwritten++;
    }
    s_low = frame;
    s_low_pending = true;
}

static void send_frame(const uint8_t *buf, size_t len)
{
    const uint32_t start_ms = now_ms();
    const int tx = radio_send(buf, len);
    const uint32_t tx_ms = now_ms() - start_ms;
    if (tx_ms > s_tx_max_ms) {
        s_tx_max_ms = tx_ms;
    }
    if (tx_ms >= CYPHAL_LOWRATE_TARGET_PERIOD_MS) {
        s_tx_late++;
    }
    if (tx != 0) {
        s_ls.tx_fail++;
        LOG_ERROR("TX failed rc=%d time=%ums", tx, (unsigned)tx_ms);
    } else {
        s_ls.tx_ok++;
        LOG_DEBUG("tx %uB %ums", (unsigned)len, (unsigned)tx_ms);
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
    static uint32_t previous_ms;
    static uint32_t previous_rx;
    static uint32_t previous_tx;
    static uint32_t previous_fail;
    static uint32_t previous_late;
    static uint32_t previous_overwritten;

    const uint32_t current_ms = now_ms();
    const uint32_t elapsed_ms = current_ms - previous_ms;
    const uint32_t rx = s_ls.rx - previous_rx;
    const uint32_t tx = s_ls.tx_ok - previous_tx;
    const uint32_t fail = s_ls.tx_fail - previous_fail;
    const uint32_t late = s_tx_late - previous_late;
    const uint32_t dropped = s_low_overwritten - previous_overwritten;
    const uint32_t rx_rate_x100 = elapsed_ms ?
        (uint32_t)((uint64_t)rx * 100000ULL / elapsed_ms) : 0U;
    const uint32_t tx_rate_x100 = elapsed_ms ?
        (uint32_t)((uint64_t)tx * 100000ULL / elapsed_ms) : 0U;
    const uint32_t target_x100 = CYPHAL_LOWRATE_TARGET_PPS * 100U;
    const bool slow = tx_rate_x100 < (target_x100 * 9U / 10U);

    char a[16];
    age(a, sizeof(a), s_ls.last_rx_ms);
    if (s_ls.rx == 0U) {
        LOG_ERROR("TX no Cyphal input last=%s", a);
    } else if (fail != 0U) {
        LOG_ERROR("TX rate=%u.%02upps target=1 rx=%u.%02u late=%u fail=%u skip=%u max=%ums",
                  (unsigned)(tx_rate_x100 / 100U), (unsigned)(tx_rate_x100 % 100U),
                  (unsigned)(rx_rate_x100 / 100U), (unsigned)(rx_rate_x100 % 100U),
                  (unsigned)late, (unsigned)fail, (unsigned)dropped, (unsigned)s_tx_max_ms);
    } else if (slow || late != 0U || dropped != 0U) {
        LOG_WARNING("TX rate=%u.%02upps target=1 rx=%u.%02u late=%u fail=0 skip=%u max=%ums",
                    (unsigned)(tx_rate_x100 / 100U), (unsigned)(tx_rate_x100 % 100U),
                    (unsigned)(rx_rate_x100 / 100U), (unsigned)(rx_rate_x100 % 100U),
                    (unsigned)late, (unsigned)dropped, (unsigned)s_tx_max_ms);
    } else {
        LOG_INFO("TX rate=%u.%02upps target=1 rx=%u.%02u late=0 fail=0 skip=0 max=%ums",
                 (unsigned)(tx_rate_x100 / 100U), (unsigned)(tx_rate_x100 % 100U),
                 (unsigned)(rx_rate_x100 / 100U), (unsigned)(rx_rate_x100 % 100U),
                 (unsigned)s_tx_max_ms);
    }

    LOG_DEBUG("held bme=%u tsl=%u ltr=%u pm=%u efm=%u decode=%u pack=%u",
              (unsigned)s_bme_stale, (unsigned)s_tsl_stale, (unsigned)s_ltr_stale,
              (unsigned)s_pm_stale, (unsigned)s_efm_stale,
              (unsigned)s_ls.dec_fail, (unsigned)s_pack_fail);
    previous_ms = current_ms;
    previous_rx = s_ls.rx;
    previous_tx = s_ls.tx_ok;
    previous_fail = s_ls.tx_fail;
    previous_late = s_tx_late;
    previous_overwritten = s_low_overwritten;
    s_tx_max_ms = 0U;
}
