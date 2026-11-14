#include "radio.h"

#include <stdbool.h>
#include <stdio.h>

#include "config.h"          /* root config.h: RADIO_* pins, freq, power, TCXO */
#include "pico/stdlib.h"
#include "sx126x.h"
#include "sx126x_hal_pico.h"

/* Operational radio settings.
 * The ground-station radio MUST use the same: SF9, BW125, CR4/5, sync 0x12, CRC on. */
static const sx126x_pico_t radio = {
    .spi = RADIO_SPI, .cs = RADIO_CS, .reset = RADIO_RESET, .busy = RADIO_BUSY,
};
static const sx126x_mod_params_lora_t modulation = {
    .sf = SX126X_LORA_SF9, .bw = SX126X_LORA_BW_125,
    .cr = SX126X_LORA_CR_4_5, .ldro = 0,
};

static bool radio_ready = false;
static bool radio_receiving = false;

#define CHECK(call) do { \
    if ((call) != SX126X_STATUS_OK) { \
        printf("radio FAIL: %s\n", #call); \
        return false; \
    } \
} while (0)

static bool check_errors(void)
{
    sx126x_errors_mask_t errors = 0;
    sx126x_chip_status_t status = {0};
    CHECK(sx126x_get_device_errors(&radio, &errors));
    CHECK(sx126x_get_status(&radio, &status));
    if (errors || status.cmd_status == SX126X_CMD_STATUS_CMD_TIMEOUT ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_PROCESS_ERROR ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_EXEC_FAILURE) {
        printf("radio FAIL: errors=0x%04x mode=%u cmd=%u\n",
               (unsigned)errors, (unsigned)status.chip_mode, (unsigned)status.cmd_status);
        return false;
    }
    return true;
}

/* Everything that stays the same between packets. */
static bool radio_configure(void)
{
    const sx126x_pa_cfg_params_t pa = {
        .pa_duty_cycle = 0x04, .hp_max = 0x07, .device_sel = 0, .pa_lut = 1,
    };

    radio_ready = false;
    radio_receiving = false;
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
    CHECK(sx126x_set_rf_freq(&radio, RADIO_FREQ_HZ));
    CHECK(sx126x_set_pa_cfg(&radio, &pa));
    CHECK(sx126x_set_tx_params(&radio, RADIO_CHIP_POWER_DBM, SX126X_RAMP_200_US));
    CHECK(sx126x_set_lora_mod_params(&radio, &modulation));
    CHECK(sx126x_set_lora_sync_word(&radio, 0x12));
    CHECK(sx126x_set_buffer_base_address(&radio, 0, 0));
    CHECK(sx126x_set_rx_tx_fallback_mode(&radio, SX126X_FALLBACK_STDBY_RC));
    CHECK(sx126x_set_dio_irq_params(&radio,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, 0, 0, 0));
    CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
    if (!check_errors()) {
        return false;
    }
    printf("radio config: %lu Hz, %d dBm, SF%u, BW%u kHz, CR4/5, sync 0x12, CRC on\n",
           (unsigned long)RADIO_FREQ_HZ, (int)RADIO_CHIP_POWER_DBM,
           (unsigned)modulation.sf,
           modulation.bw == SX126X_LORA_BW_500 ? 500U :
           modulation.bw == SX126X_LORA_BW_250 ? 250U : 125U);
    radio_ready = true;
    return true;
}

static bool transmit(const uint8_t *payload, size_t len)
{
    const sx126x_pkt_params_lora_t packet = {
        .preamble_len_in_symb = 8,
        .header_type = SX126X_LORA_PKT_EXPLICIT,
        .pld_len_in_bytes = (uint8_t)len,
        .crc_is_on = true,
        .invert_iq_is_on = false,
    };

    if (radio_receiving) {
        CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
    }
    CHECK(sx126x_set_lora_pkt_params(&radio, &packet));
    radio_receiving = false;
    CHECK(sx126x_write_buffer(&radio, 0, payload, (uint8_t)len));
    /* radio_receive() enables only RX-related IRQs. Restore TX_DONE here or
     * every transmission after entering RX mode will hit the host deadline. */
    CHECK(sx126x_set_dio_irq_params(&radio,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, 0, 0, 0));
    CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
    CHECK(sx126x_set_tx(&radio, 1000));

    const absolute_time_t deadline = make_timeout_time_ms(1500);
    while (!time_reached(deadline)) {
        sx126x_irq_mask_t irq = 0;
        CHECK(sx126x_get_irq_status(&radio, &irq));
        if (irq & SX126X_IRQ_TIMEOUT) {
            puts("radio FAIL: TX timeout");
            return false;
        }
        if (irq & SX126X_IRQ_TX_DONE) {
            CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
            CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
            return check_errors();
        }
        sleep_us(250);
    }
    puts("radio FAIL: host TX deadline expired");
    return false;
}

int radio_init(void)
{
    gpio_init(UNUSED_RADIO_CS);
    gpio_put(UNUSED_RADIO_CS, 1);
    gpio_set_dir(UNUSED_RADIO_CS, GPIO_OUT);
    sx126x_pico_init(&radio, RADIO_SCK, RADIO_MOSI, RADIO_MISO);
    return radio_configure() ? 0 : -1;
}

int radio_send(const uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0U || len > 255U) {
        return -2;
    }
    if (!radio_ready && !radio_configure()) {
        return -1;
    }
    if (transmit(buf, len)) {
        return 0;
    }
    radio_ready = false;   /* rebuild full config before the next packet */
    return -1;
}

int radio_receive(uint8_t *buf, size_t capacity)
{
    if (buf == NULL || capacity == 0U) {
        return -2;
    }
    if (!radio_ready && !radio_configure()) {
        return -1;
    }

    if (!radio_receiving) {
        const sx126x_pkt_params_lora_t packet = {
            .preamble_len_in_symb = 8,
            .header_type = SX126X_LORA_PKT_EXPLICIT,
            .pld_len_in_bytes = 255,
            .crc_is_on = true,
            .invert_iq_is_on = false,
        };
        if (sx126x_set_lora_pkt_params(&radio, &packet) != SX126X_STATUS_OK ||
            sx126x_set_dio_irq_params(&radio, SX126X_IRQ_RX_DONE |
                SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT,
                0, 0, 0) != SX126X_STATUS_OK ||
            sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL) != SX126X_STATUS_OK ||
            sx126x_set_rx_with_timeout_in_rtc_step(
                &radio, SX126X_RX_CONTINUOUS) != SX126X_STATUS_OK) {
            radio_ready = false;
            return -1;
        }
        radio_receiving = true;
        return 0;
    }

    sx126x_irq_mask_t irq = 0;
    if (sx126x_get_irq_status(&radio, &irq) != SX126X_STATUS_OK) {
        radio_ready = false;
        radio_receiving = false;
        return -1;
    }
    if ((irq & (SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERROR |
                SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT)) == 0U) {
        return 0;
    }

    radio_receiving = false;
    (void)sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC);
    if ((irq & SX126X_IRQ_RX_DONE) != 0U) {
        (void)sx126x_handle_rx_done(&radio);
    }
    (void)sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL);
    if ((irq & SX126X_IRQ_RX_DONE) == 0U ||
        (irq & (SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR)) != 0U) {
        return 0;
    }

    sx126x_rx_buffer_status_t status = {0};
    if (sx126x_get_rx_buffer_status(&radio, &status) != SX126X_STATUS_OK ||
        status.pld_len_in_bytes > capacity ||
        sx126x_read_buffer(&radio, status.buffer_start_pointer, buf,
            status.pld_len_in_bytes) != SX126X_STATUS_OK) {
        return -1;
    }
    return status.pld_len_in_bytes;
}
