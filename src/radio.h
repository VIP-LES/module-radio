#pragma once

#include <stddef.h>
#include <stdint.h>

/* Thin adapter over the existing SX126x code (sx126x_hal_pico + Semtech driver). */
int radio_init(void);                                  /* 0 on success */
int radio_send(const uint8_t *buf, size_t len);        /* 0 on success */
int radio_receive(uint8_t *buf, size_t capacity);      /* bytes received, 0 if none */
