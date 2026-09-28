#pragma once

#include "hardware/spi.h"

typedef struct {
    spi_inst_t *spi;
    uint cs;
    uint reset;
    uint busy;
} sx126x_pico_t;

void sx126x_pico_init(const sx126x_pico_t *radio, uint sck, uint mosi, uint miso);
