#include "sx126x_hal_pico.h"
#include "sx126x_hal.h"
#include "pico/stdlib.h"

static sx126x_hal_status_t wait_ready(const sx126x_pico_t *radio)
{
    const absolute_time_t deadline = make_timeout_time_ms(1000);
    while (gpio_get(radio->busy)) {
        if (time_reached(deadline)) {
            return SX126X_HAL_STATUS_ERROR;
        }
        tight_loop_contents();
    }
    return SX126X_HAL_STATUS_OK;
}

void sx126x_pico_init(const sx126x_pico_t *radio, uint sck, uint mosi, uint miso)
{
    gpio_init(radio->cs);
    gpio_put(radio->cs, 1);
    gpio_set_dir(radio->cs, GPIO_OUT);
    gpio_init(radio->reset);
    gpio_put(radio->reset, 1);
    gpio_set_dir(radio->reset, GPIO_OUT);
    gpio_init(radio->busy);
    gpio_set_dir(radio->busy, GPIO_IN);
    gpio_pull_up(radio->busy);

    spi_init(radio->spi, 1000000);
    spi_set_format(radio->spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(sck, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
    const sx126x_pico_t *radio = context;
    gpio_put(radio->reset, 0);
    sleep_ms(1);
    gpio_put(radio->reset, 1);
    sleep_ms(10);
    return wait_ready(radio);
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
    const sx126x_pico_t *radio = context;
    gpio_put(radio->cs, 0);
    sleep_us(150);
    const sx126x_hal_status_t result = wait_ready(radio);
    gpio_put(radio->cs, 1);
    sleep_us(1);
    return result;
}

sx126x_hal_status_t sx126x_hal_write(const void *context,
    const uint8_t *command, uint16_t command_length,
    const uint8_t *data, uint16_t data_length)
{
    const sx126x_pico_t *radio = context;
    if (wait_ready(radio) != SX126X_HAL_STATUS_OK) {
        return SX126X_HAL_STATUS_ERROR;
    }
    gpio_put(radio->cs, 0);
    spi_write_blocking(radio->spi, command, command_length);
    if (data_length) {
        spi_write_blocking(radio->spi, data, data_length);
    }
    gpio_put(radio->cs, 1);
    sleep_us(1);
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void *context,
    const uint8_t *command, uint16_t command_length,
    uint8_t *data, uint16_t data_length)
{
    const sx126x_pico_t *radio = context;
    if (wait_ready(radio) != SX126X_HAL_STATUS_OK) {
        return SX126X_HAL_STATUS_ERROR;
    }
    gpio_put(radio->cs, 0);
    spi_write_blocking(radio->spi, command, command_length);
    if (data_length) {
        spi_read_blocking(radio->spi, SX126X_NOP, data, data_length);
    }
    gpio_put(radio->cs, 1);
    sleep_us(1);
    return SX126X_HAL_STATUS_OK;
}
