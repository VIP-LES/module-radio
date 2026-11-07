#include "cyphal_bridge.h"

#include <string.h>

#include "leos/log.h"
#include "pico/time.h"
#include "radio.h"
#include "radio_protocol.h"

/* Field mapping from the generated DSDL structs to the protobuf-ready frames.
 * Only fields that leos.Packet can carry are kept. */
static void lowrate_to_frame(const flight_lowrate_dsdl_t *m, lowrate_radio_frame_t *o)
{
    memset(o, 0, sizeof(*o));
    o->t_pkt_us       = (uint64_t)to_us_since_boot(get_absolute_time());

    o->temperature_c  = m->bme688.temperature.kelvin - 273.15f;   /* schema uses degrees C */
    o->pressure_pa    = m->bme688.pressure.pascal;
    o->humidity       = m->bme688.humidity;
    o->gas_resistance = m->bme688.gas_resistance;
    o->light_lux      = m->tsl2591.light_lux;
    o->uvs            = m->ltr390.uvs;

    o->gps_fix_ok     = m->gps_data.fix_ok;
    o->lat            = m->gps_data.lat;
    o->lon            = m->gps_data.lon;
    o->alt_m          = m->gps_data.alt_m;
    o->sats_used      = m->gps_data.sats_used;
    o->gps_utc_us     = m->gps_data.gps_utc.microsecond;
}

/* ADC_0_2: only ch1_diff and ch4_breakbeam on each ADC, all float32. */
static void efm_to_frame(const efm_dsdl_t *m, efm_radio_frame_t *o)
{
    memset(o, 0, sizeof(*o));
    o->t_pkt_us           = (uint64_t)to_us_since_boot(get_absolute_time());
    o->adc1_ch1_diff      = m->adc1_ch1_diff;
    o->adc1_ch4_breakbeam = m->adc1_ch4_breakbeam;
    o->adc2_ch1_diff      = m->adc2_ch1_diff;
    o->adc2_ch4_breakbeam = m->adc2_ch4_breakbeam;
}

void cyphal_bridge_on_lowrate(struct CanardRxTransfer *transfer, void *user_reference)
{
    (void)user_reference;
    if (transfer == NULL) {
        return;
    }
    LOG_INFO("rx LowRate: %u bytes", (unsigned)transfer->payload.size);

    flight_lowrate_dsdl_t msg;
    memset(&msg, 0, sizeof(msg));
    size_t size = transfer->payload.size;
    const int8_t rc = leos_aggregate_LowRate_0_1_deserialize_(
        &msg, (const uint8_t *)transfer->payload.data, &size);
    if (rc < 0) {
        LOG_ERROR("LowRate deserialize failed: %d", (int)rc);
        return;
    }

    lowrate_radio_frame_t frame;
    lowrate_to_frame(&msg, &frame);

    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];
    const size_t len = radio_protocol_pack_lowrate(&frame, buf, sizeof(buf));
    if (len == 0U) {
        LOG_ERROR("pack lowrate failed");
        return;
    }
    const int tx = radio_send(buf, len);
    if (tx != 0) {
        LOG_ERROR("radio_send lowrate failed: %d", tx);
    }
}

void cyphal_bridge_on_efm(struct CanardRxTransfer *transfer, void *user_reference)
{
    (void)user_reference;
    if (transfer == NULL) {
        return;
    }
    LOG_INFO("rx EFM: %u bytes", (unsigned)transfer->payload.size);

    efm_dsdl_t msg;
    memset(&msg, 0, sizeof(msg));
    size_t size = transfer->payload.size;
    const int8_t rc = leos_efm_ADC_0_2_deserialize_(
        &msg, (const uint8_t *)transfer->payload.data, &size);
    if (rc < 0) {
        LOG_ERROR("EFM deserialize failed: %d", (int)rc);
        return;
    }

    efm_radio_frame_t frame;
    efm_to_frame(&msg, &frame);

    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];
    const size_t len = radio_protocol_pack_efm(&frame, buf, sizeof(buf));
    if (len == 0U) {
        LOG_ERROR("pack efm failed");
        return;
    }
    const int tx = radio_send(buf, len);
    if (tx != 0) {
        LOG_ERROR("radio_send efm failed: %d", tx);
    }
}