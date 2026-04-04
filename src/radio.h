#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "leos/sx126x.h"

typedef struct
{
    bool sx1262_enabled;
    bool sx1268_enabled;
} radio_init_options_t;

int radio_init(void);
int radio_init_with_options(const radio_init_options_t *options);

void radio_service_irqs(void);

int radio_send_sx1262(const uint8_t *buf, size_t len);
int radio_send_sx1268(const uint8_t *buf, size_t len);

void radio_handle_dio1_irq_sx1262(void);
void radio_handle_dio1_irq_sx1268(void);

bool radio_sx1262_packet_available(void);

int radio_read_sx1262(
    uint8_t *buf,
    size_t buf_size,
    size_t *out_len,
    leos_radio_packet_info_t *info);

int radio_enter_command_rx_mode(void);

bool radio_is_sx1262_enabled(void);
bool radio_is_sx1268_enabled(void);
