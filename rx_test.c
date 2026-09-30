#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "sx126x.h"
#include "sx126x_hal_pico.h"
#include "config.h"
#include "tx_payload.h"

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
    .pld_len_in_bytes = 255, .crc_is_on = true, .invert_iq_is_on = false,
};

static bool initialize_receiver(void)
{
    CHECK(sx126x_reset(&radio));
    CHECK(sx126x_wakeup(&radio));
    CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
    CHECK(sx126x_clear_device_errors(&radio));
    CHECK(sx126x_set_dio3_as_tcxo_ctrl(&radio, RADIO_TCXO_VOLTAGE, 320));
    CHECK(sx126x_cal(&radio, SX126X_CAL_ALL));
    CHECK(sx126x_cal_img(&radio, 0xE1, 0xE9));
    CHECK(sx126x_set_reg_mode(&radio, SX126X_REG_MODE_LDO));
    // DIO2 holds TXEN low in RX. RXEN must be separately held at 3.3 V.
    CHECK(sx126x_set_dio2_as_rf_sw_ctrl(&radio, true));
    CHECK(sx126x_set_pkt_type(&radio, SX126X_PKT_TYPE_LORA));
    CHECK(sx126x_set_rf_freq(&radio, RADIO_TEST_FREQ_HZ));
    CHECK(sx126x_set_lora_mod_params(&radio, &modulation));
    CHECK(sx126x_set_lora_pkt_params(&radio, &packet));
    CHECK(sx126x_set_lora_sync_word(&radio, 0x12));
    CHECK(sx126x_set_buffer_base_address(&radio, 0, 0));
    CHECK(sx126x_set_rx_tx_fallback_mode(&radio, SX126X_FALLBACK_STDBY_RC));
    CHECK(sx126x_set_dio_irq_params(&radio, SX126X_IRQ_RX_DONE |
        SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT,
        0, 0, 0));
    sx126x_errors_mask_t errors = 0;
    CHECK(sx126x_get_device_errors(&radio, &errors));
    if (errors) {
        printf("FAIL: device errors=0x%04x\n", (unsigned)errors);
        return false;
    }
    return true;
}

static bool receive_packet(void)
{
    CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
    // Single packet mode protects the buffer while USB output is printed.
    CHECK(sx126x_set_rx(&radio, 1000));
    const absolute_time_t deadline = make_timeout_time_ms(2000);
    while (!time_reached(deadline)) {
        sx126x_irq_mask_t irq = 0;
        CHECK(sx126x_get_irq_status(&radio, &irq));
        if (irq & (SX126X_IRQ_RX_DONE | SX126X_IRQ_CRC_ERROR |
                   SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_TIMEOUT)) {
            CHECK(sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC));
            if (irq & SX126X_IRQ_RX_DONE) CHECK(sx126x_handle_rx_done(&radio));
            CHECK(sx126x_clear_irq_status(&radio, SX126X_IRQ_ALL));
            if (irq & (SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR)) {
                printf("DROP: CRC/header error (IRQ=0x%04x)\n", (unsigned)irq);
                return true;
            }
            if (!(irq & SX126X_IRQ_RX_DONE)) return true; // Normal idle timeout.
            uint8_t payload[255];
            sx126x_rx_buffer_status_t buffer = {0};
            sx126x_pkt_status_lora_t status = {0};
            CHECK(sx126x_get_rx_buffer_status(&radio, &buffer));
            CHECK(sx126x_read_buffer(&radio, buffer.buffer_start_pointer,
                payload, buffer.pld_len_in_bytes));
            CHECK(sx126x_get_lora_pkt_status(&radio, &status));
            printf("RX: %u bytes, RSSI %d dBm, SNR %d dB\n",
                buffer.pld_len_in_bytes, status.rssi_pkt_in_dbm, status.snr_pkt_in_db);
            printf("HEX:");
            for (unsigned i = 0; i < buffer.pld_len_in_bytes; ++i)
                printf(" %02x", payload[i]);
            puts("");
            if (buffer.pld_len_in_bytes == sizeof tx_payload &&
                memcmp(payload, tx_payload, sizeof tx_payload) == 0)
                puts("PASS: exact test payload match (LEOS-TEST, sequence 42)");
            else
                puts("MISMATCH: received packet differs from tx_payload.h");
            return true;
        }
        sleep_ms(2);
    }
    puts("FAIL: host RX deadline expired");
    return false;
}

int main(void)
{
    stdio_init_all();
    gpio_init(UNUSED_RADIO_CS);
    gpio_put(UNUSED_RADIO_CS, 1);
    gpio_set_dir(UNUSED_RADIO_CS, GPIO_OUT);
    sx126x_pico_init(&radio, RADIO_SCK, RADIO_MOSI, RADIO_MISO);
    while (true) {
        if (initialize_receiver()) {
            absolute_time_t report = get_absolute_time();
            while (true) {
                if (time_reached(report)) {
                    printf("Listening: %lu Hz, SF7, BW125, CR4/5, sync 0x12, CRC on\n",
                        (unsigned long)RADIO_TEST_FREQ_HZ);
                    report = make_timeout_time_ms(10000);
                }
                if (!receive_packet()) break;
            }
        }
        puts("Restarting receiver in 2 seconds...");
        sleep_ms(2000);
    }
}
