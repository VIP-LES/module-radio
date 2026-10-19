#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pb_encode.h"
#include "packet.pb.h"
#include "sx126x.h"
#include "sx126x_hal_pico.h"
#include "config.h"
#include "tx_payload.h"

_Static_assert(leos_Packet_size <= 255, "Protobuf exceeds SX1262 packet limit");
_Static_assert(RADIO_TEST_FREQ_HZ >= 902000000u && RADIO_TEST_FREQ_HZ <= 928000000u,
    "Update image calibration before selecting another band");

#define CHECK(call) do { \
    if ((call) != SX126X_STATUS_OK) { \
        printf("FAIL: %s\n", #call); \
        return false; \
    } \
} while (0)

static const sx126x_pico_t radio = {
    .spi = RADIO_SPI, .cs = RADIO_CS, .reset = RADIO_RESET, .busy = RADIO_BUSY,
};
static const sx126x_mod_params_lora_t modulation = {
    .sf = SX126X_LORA_SF7, .bw = SX126X_LORA_BW_125,
    .cr = SX126X_LORA_CR_4_5, .ldro = 0,
};

static bool encode_test_packet(uint8_t *buffer, size_t capacity, size_t *encoded_size)
{
    leos_Packet message = leos_Packet_init_zero;
    strcpy(message.callsign, "LEOS-TEST");
    message.sequence = 42;
    message.timestamp_ms = 123456;
    message.which_payload = leos_Packet_telemetry_tag;
    message.payload.telemetry.which_data = leos_TelemetryPacket_low_rate_tag;

    leos_LowRateTelemetry *low = &message.payload.telemetry.data.low_rate;
    low->has_sensors = true;
    low->sensors = (leos_PurpleboardTelemetry){
        .temperature = 20.5f,
        .pressure = 101325.0f,
        .humidity = 45.5f,
        .uvi = 3,
        .light_lux = 128.0f,
        .gas_resistance = 10000.0f,
    };
    low->has_beacon = true;
    low->beacon.has_gps = true;
    low->beacon.gps = (leos_GPS){
        .lat = 33.75,
        .lon = -84.5,
        .alt_m = 300.0f,
        .satellites = 8,
        .fix = 3,
        .climb_mps = 1.5f,
        .hdop = 0.75f,
        .utc_unix_ms = 1789992000000ULL,
    };
    low->beacon.has_health = true;
    low->beacon.health = (leos_Health){
        .flight_computer = 1,
        .power = 2,
        .efm = 4,
        .sensors = 8,
        .radio = 16,
    };

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, capacity);
    if (!pb_encode(&stream, leos_Packet_fields, &message)) {
        printf("FAIL: protobuf encode: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    *encoded_size = stream.bytes_written;
    return true;
}

static bool check_reference_payload(const uint8_t *payload, size_t payload_size)
{
    if (payload_size != sizeof tx_payload) {
        printf("FAIL: encoded size %u, expected %u\n",
            (unsigned)payload_size, (unsigned)sizeof tx_payload);
        return false;
    }
    for (size_t i = 0; i < payload_size; ++i) {
        if (payload[i] != tx_payload[i]) {
            printf("FAIL: encoded byte %u is 0x%02x, expected 0x%02x\n",
                (unsigned)i, payload[i], tx_payload[i]);
            return false;
        }
    }
    printf("PASS: protobuf encoding matches %u-byte reference payload\n",
        (unsigned)payload_size);
    return true;
}

static bool check_errors(void)
{
    sx126x_errors_mask_t errors = 0;
    sx126x_chip_status_t status = {0};
    CHECK(sx126x_get_device_errors(&radio, &errors));
    CHECK(sx126x_get_status(&radio, &status));
    if (errors || status.cmd_status == SX126X_CMD_STATUS_CMD_TIMEOUT ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_PROCESS_ERROR ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_EXEC_FAILURE) {
        printf("FAIL: device errors=0x%04x mode=%u command=%u\n",
            (unsigned)errors, (unsigned)status.chip_mode, (unsigned)status.cmd_status);
        return false;
    }
    return true;
}

static bool send_packet(const uint8_t *payload, size_t payload_size)
{
    const sx126x_pa_cfg_params_t pa = {
        .pa_duty_cycle = 0x04, .hp_max = 0x07, .device_sel = 0, .pa_lut = 1,
    };
    const sx126x_pkt_params_lora_t packet = {
        .preamble_len_in_symb = 8,
        .header_type = SX126X_LORA_PKT_EXPLICIT,
        .pld_len_in_bytes = (uint8_t)payload_size,
        .crc_is_on = true,
        .invert_iq_is_on = false,
    };

    CHECK(sx126x_reset(&radio));
    CHECK(sx126x_wakeup(&radio));
    CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
    CHECK(sx126x_clear_device_errors(&radio));
    CHECK(sx126x_set_dio3_as_tcxo_ctrl(&radio, RADIO_TCXO_VOLTAGE, 320));
    CHECK(sx126x_cal(&radio, SX126X_CAL_ALL));
    CHECK(sx126x_cal_img(&radio, 0xE1, 0xE9));
    CHECK(sx126x_set_reg_mode(&radio, SX126X_REG_MODE_LDO));
    CHECK(sx126x_set_dio2_as_rf_sw_ctrl(&radio, true));
    CHECK(sx126x_set_pkt_type(&radio, SX126X_PKT_TYPE_LORA));
    CHECK(sx126x_set_rf_freq(&radio, RADIO_TEST_FREQ_HZ));
    CHECK(sx126x_set_pa_cfg(&radio, &pa));
    CHECK(sx126x_set_tx_params(&radio, RADIO_TEST_CHIP_POWER_DBM, SX126X_RAMP_200_US));
    CHECK(sx126x_set_lora_mod_params(&radio, &modulation));
    CHECK(sx126x_set_lora_pkt_params(&radio, &packet));
    CHECK(sx126x_set_lora_sync_word(&radio, 0x12));
    CHECK(sx126x_set_buffer_base_address(&radio, 0, 0));
    CHECK(sx126x_write_buffer(&radio, 0, payload, (uint8_t)payload_size));
    CHECK(sx126x_set_rx_tx_fallback_mode(&radio, SX126X_FALLBACK_STDBY_RC));
    CHECK(sx126x_set_dio_irq_params(&radio,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, 0, 0, 0));
    CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
    if (!check_errors()) return false;

    printf("TX: %u protobuf bytes, estimated airtime %lu ms\n",
        (unsigned)payload_size,
        (unsigned long)sx126x_get_lora_time_on_air_in_ms(&packet, &modulation));
    const absolute_time_t start = get_absolute_time();
    CHECK(sx126x_set_tx(&radio, 1000));
    const absolute_time_t deadline = make_timeout_time_ms(1500);
    while (!time_reached(deadline)) {
        sx126x_irq_mask_t irq = 0;
        CHECK(sx126x_get_irq_status(&radio, &irq));
        if (irq & SX126X_IRQ_TIMEOUT) {
            puts("FAIL: radio TX timeout");
            return false;
        }
        if (irq & SX126X_IRQ_TX_DONE) {
            CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
            CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
            if (!check_errors()) return false;
            printf("TX_DONE: elapsed %lld us; receiver confirmation required\n",
                (long long)absolute_time_diff_us(start, get_absolute_time()));
            return true;
        }
        sleep_ms(2);
    }
    puts("FAIL: host TX deadline expired");
    return false;
}

int main(void)
{
    uint8_t payload[leos_Packet_size];
    size_t payload_size = 0;

    stdio_init_all();
    gpio_init(UNUSED_RADIO_CS);
    gpio_put(UNUSED_RADIO_CS, 1);
    gpio_set_dir(UNUSED_RADIO_CS, GPIO_OUT);
    sx126x_pico_init(&radio, RADIO_SCK, RADIO_MOSI, RADIO_MISO);
    if (sx126x_reset(&radio) != SX126X_STATUS_OK)
        puts("FAIL: initial reset/BUSY timeout");

    while (true) {
        printf("Protobuf TX test: %lu Hz, chip power %d dBm, SF7, BW125, CR4/5, sync 0x12, CRC on.\n",
            (unsigned long)RADIO_TEST_FREQ_HZ, RADIO_TEST_CHIP_POWER_DBM);
        puts("Attach antenna/load. Enter t to encode and send ONE packet.");
        int ch = getchar_timeout_us(2000000);
        if (ch != 't') continue;
        if (encode_test_packet(payload, sizeof payload, &payload_size) &&
            check_reference_payload(payload, payload_size)) {
            printf("ENCODED: callsign LEOS-TEST, sequence 42, %u bytes\n",
                (unsigned)payload_size);
            if (!send_packet(payload, payload_size) &&
                sx126x_reset(&radio) != SX126X_STATUS_OK)
                puts("FAIL: reset failed; disconnect power");
        }
        sleep_ms(1000);
        while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {}
    }
}
