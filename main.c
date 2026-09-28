#include <stdio.h>
#include "pico/stdlib.h"
#include "sx126x.h"
#include "sx126x_hal_pico.h"
#include "config.h"

static const sx126x_pico_t radio = {
    .spi = RADIO_SPI,
    .cs = RADIO_CS,
    .reset = RADIO_RESET,
    .busy = RADIO_BUSY,
};

static void check_radio(void)
{
    if (sx126x_reset(&radio) != SX126X_STATUS_OK ||
        sx126x_wakeup(&radio) != SX126X_STATUS_OK ||
        sx126x_set_standby(&radio, SX126X_STANDBY_CFG_RC) != SX126X_STATUS_OK) {
        puts("FAIL: radio initialization/BUSY timeout");
        return;
    }

    sx126x_chip_status_t status = {0};
    const sx126x_status_t result = sx126x_get_status(&radio, &status);
    const bool command_error =
        status.cmd_status == SX126X_CMD_STATUS_CMD_TIMEOUT ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_PROCESS_ERROR ||
        status.cmd_status == SX126X_CMD_STATUS_CMD_EXEC_FAILURE;

    if (result != SX126X_STATUS_OK ||
        status.chip_mode != SX126X_CHIP_MODE_STBY_RC || command_error) {
        printf("FAIL: status (HAL=%u, mode=%u, command=%u)\n",
            (unsigned)result, (unsigned)status.chip_mode,
            (unsigned)status.cmd_status);
        return;
    }

    printf("PASS: SX126x reports RC standby (mode=%u, command=%u)\n",
        (unsigned)status.chip_mode, (unsigned)status.cmd_status);
}

int main(void)
{
    stdio_init_all();
    gpio_init(UNUSED_RADIO_CS);
    gpio_put(UNUSED_RADIO_CS, 1);
    gpio_set_dir(UNUSED_RADIO_CS, GPIO_OUT);
    sx126x_pico_init(&radio, RADIO_SCK, RADIO_MOSI, RADIO_MISO);

    while (true) {
        check_radio();
        sleep_ms(2000);
    }
}
