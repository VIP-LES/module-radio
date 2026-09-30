#include <stdio.h>
#include "pico/stdlib.h"
#include "sx126x.h"
#include "sx126x_hal_pico.h"
#include "config.h"
#include "tx_payload.h"

_Static_assert(sizeof tx_payload <= 255, "Payload exceeds SX1262 packet limit");
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
static const sx126x_pkt_params_lora_t packet = {
    .preamble_len_in_symb = 8, .header_type = SX126X_LORA_PKT_EXPLICIT,
    .pld_len_in_bytes = sizeof tx_payload, .crc_is_on = true,
    .invert_iq_is_on = false,
};

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

static bool send_packet(void)
{
    const sx126x_pa_cfg_params_t pa = {
        .pa_duty_cycle = 0x04, .hp_max = 0x07, .device_sel = 0, .pa_lut = 1,
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
    CHECK(sx126x_write_buffer(&radio, 0, tx_payload, sizeof tx_payload));
    CHECK(sx126x_set_rx_tx_fallback_mode(&radio, SX126X_FALLBACK_STDBY_RC));
    CHECK(sx126x_set_dio_irq_params(&radio,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, 0, 0, 0));
    CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
    if (!check_errors()) return false;

    printf("TX: %u bytes, estimated airtime %lu ms\n", (unsigned)sizeof tx_payload,
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
    stdio_init_all();
    gpio_init(UNUSED_RADIO_CS);
    gpio_put(UNUSED_RADIO_CS, 1);
    gpio_set_dir(UNUSED_RADIO_CS, GPIO_OUT);
    sx126x_pico_init(&radio, RADIO_SCK, RADIO_MOSI, RADIO_MISO);
    if (sx126x_reset(&radio) != SX126X_STATUS_OK)
        puts("FAIL: initial reset/BUSY timeout");

    while (true) {
        printf("TX test: %lu Hz, chip power %d dBm, SF7, BW125, CR4/5, sync 0x12, CRC on.\n",
            (unsigned long)RADIO_TEST_FREQ_HZ, RADIO_TEST_CHIP_POWER_DBM);
        puts("Hold RXEN at GND and attach antenna/load. Enter t for ONE packet.");
        int ch = getchar_timeout_us(2000000);
        if (ch != 't') continue;
        if (!send_packet()) {
            if (sx126x_reset(&radio) != SX126X_STATUS_OK)
                puts("FAIL: reset failed; disconnect power");
        }
        sleep_ms(1000);
        while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {}
    }
}
