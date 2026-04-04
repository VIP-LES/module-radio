#include "tethered_test.h"

#include <string.h>

#include "config.h"
#include "leos/log.h"
#include "pico/time.h"
#include "radio.h"
#include "radio_protocol.h"

#define TETHERED_TEST_PERIOD_MS 1000

static absolute_time_t g_next_tx_at;
static uint8_t g_sequence = 0u;

static void fill_fake_sensor_gps_frame(sensor_gps_radio_frame_t *frame)
{
    const uint32_t step = (uint32_t)g_sequence;

    memset(frame, 0, sizeof(*frame));

    frame->t_pkt_us = (uint64_t)to_us_since_boot(get_absolute_time());

    frame->bme688.humidity = 45.0f + (float)(step % 10u);
    frame->bme688.pressure = 101325.0f - (float)(step * 8u);
    frame->bme688.temperature = 273.15f + 20.0f + (float)(step % 5u);
    frame->bme688.altitude = 350.0f + (float)(step * 3u);
    frame->bme688.gas_resistance = 1200.0f + (float)(step * 15u);
    frame->bme688_valid = true;

    frame->tsl2591.light_lux = 1000.0f + (float)(step * 25u);
    frame->tsl2591.raw_visible = (uint16_t)(500u + step * 4u);
    frame->tsl2591.raw_infrared = (uint16_t)(240u + step * 3u);
    frame->tsl2591.raw_full_spectrum = 1800u + step * 9u;
    frame->tsl2591_valid = true;

    frame->ltr390.uvs = 20u + step;
    frame->ltr390_valid = true;

    frame->pmsa003i.pm10_env = 5u + (step % 4u);
    frame->pmsa003i.pm25_env = 7u + (step % 5u);
    frame->pmsa003i.pm100_env = 9u + (step % 6u);
    frame->pmsa003i.aqi_pm25_us = 12u + (step % 4u);
    frame->pmsa003i.aqi_pm100_us = 15u + (step % 5u);
    frame->pmsa003i.particles_03um = 100u + step * 10u;
    frame->pmsa003i.particles_05um = 80u + step * 8u;
    frame->pmsa003i.particles_10um = 60u + step * 6u;
    frame->pmsa003i.particles_25um = 40u + step * 4u;
    frame->pmsa003i.particles_50um = 20u + step * 2u;
    frame->pmsa003i.particles_100um = 10u + step;
    frame->pmsa003i_valid = true;

    frame->gps_data.fix_ok = true;
    frame->gps_data.lat = 35.7700 + ((double)(step % 20u) * 0.0001);
    frame->gps_data.lon = -78.6700 - ((double)(step % 20u) * 0.0001);
    frame->gps_data.alt_m = 350.0f + (float)(step * 3u);
    frame->gps_data.speed_mps = 4.0f + (float)(step % 3u);
    frame->gps_data.track_deg = (float)((step * 15u) % 360u);
    frame->gps_data.sats_used = 9u;
    frame->gps_data.sats_visible = 13u;
    frame->gps_data.gps_utc_us = frame->t_pkt_us;
}

void tethered_test_init(void)
{
    g_next_tx_at = delayed_by_ms(get_absolute_time(), TETHERED_TEST_PERIOD_MS);
}

void tethered_test_service(void)
{
    if (!radio_is_sx1262_enabled())
    {
        return;
    }

    if (absolute_time_diff_us(get_absolute_time(), g_next_tx_at) > 0)
    {
        return;
    }

    sensor_gps_radio_frame_t frame;
    uint8_t packet[RADIO_RF_MAX_PACKET_SIZE];
    size_t packet_len;

    fill_fake_sensor_gps_frame(&frame);
    packet_len = radio_protocol_pack_sensor_gps_frame(&frame, packet, sizeof(packet));
    if (packet_len == 0u)
    {
        LOG_ERROR("tethered_test pack_sensor_gps_frame failed");
        g_next_tx_at = delayed_by_ms(get_absolute_time(), TETHERED_TEST_PERIOD_MS);
        return;
    }

    if (radio_send_sx1262(packet, packet_len) != 0)
    {
        LOG_ERROR("tethered_test radio_send_sx1262 failed");
    }
    else
    {
        LOG_INFO("tethered_test TX seq=%u len=%u", (unsigned)g_sequence, (unsigned)packet_len);
    }

    g_sequence++;
    g_next_tx_at = delayed_by_ms(get_absolute_time(), TETHERED_TEST_PERIOD_MS);
}
