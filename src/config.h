#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "leos/mcp251xfd/config.h"
#include "leos/sx126x.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

/* Cyphal subject IDs/extents. Keep these aligned with the generated DSDL. */
#define CYPHAL_SUB_SENSOR_GPS_ID 1500
#define CYPHAL_SUB_SENSOR_GPS_EXTENT 2048

#define CYPHAL_SUB_EFM_ID 1400
#define CYPHAL_SUB_EFM_EXTENT 300

#define CYPHAL_SUB_RADIO_MODE_CMD_ID 3
#define CYPHAL_SUB_RADIO_MODE_CMD_EXTENT 16

/* RF frame constants */
#define RADIO_RF_SYNC_BYTE 0xA5
#define RADIO_RF_VERSION 1u
#define RADIO_RF_MSG_SENSOR_GPS 0x01u
#define RADIO_RF_MSG_EFM 0x02u
#define RADIO_RF_MSG_COMMAND 0x03u
#define RADIO_RF_MAX_PACKET_SIZE 255u
#define RADIO_RF_MAX_COMMAND_ARGS 16u

/* -----------------------------------------------------------------------
 * SX126x shared SPI bus pins
 * TODO: verify against board schematic before first hardware test
 * --------------------------------------------------------------------- */
#define LEOS_SX126X_SPI_PORT spi1
#define LEOS_SX126X_SPI_BAUD_HZ 8000000 /* 8 MHz — TODO: confirm */
#define LEOS_SX126X_PIN_SCK 10          /* TODO: set from schematic */
#define LEOS_SX126X_PIN_MOSI 11         /* TODO: set from schematic */
#define LEOS_SX126X_PIN_MISO 12         /* TODO: set from schematic */

/* -----------------------------------------------------------------------
 * SX1262 per-device GPIO pins
 * TODO: verify against board schematic before first hardware test
 * --------------------------------------------------------------------- */
#define LEOS_SX1262_PIN_NSS 13   /* TODO: set from schematic */
#define LEOS_SX1262_PIN_BUSY 19  /* TODO: set from schematic */
#define LEOS_SX1262_PIN_RESET 15 /* TODO: set from schematic */ 
#define LEOS_SX1262_PIN_DIO1 17  /* TODO: set from schematic */

/* -----------------------------------------------------------------------
 * SX1268 per-device GPIO pins
 * TODO: verify against board schematic before first hardware test
 * --------------------------------------------------------------------- */
#define LEOS_SX1268_PIN_NSS 14   /* TODO: set from schematic */ 
#define LEOS_SX1268_PIN_BUSY 18  /* TODO: set from schematic */
#define LEOS_SX1268_PIN_RESET 15 /* TODO: set from schematic */
#define LEOS_SX1268_PIN_DIO1 16  /* TODO: set from schematic */

/* SX1262 LoRa config */
#define SX1262_RF_FREQUENCY_HZ 915000000UL /* TODO: confirm frequency */
#define SX1262_TX_POWER_DBM 14             /* TODO: confirm power */
#define SX1262_CRC_ENABLED true
#define SX1262_IQ_INVERTED false
#define SX1262_BANDWIDTH LEOS_RADIO_BW_125_KHZ  /* TODO: confirm */
#define SX1262_SPREADING_FACTOR LEOS_RADIO_SF_9 /* TODO: confirm */
#define SX1262_CODING_RATE LEOS_RADIO_CR_4_5    /* TODO: confirm */
#define SX1262_SYNC_WORD 0x12                   /* TODO: confirm */

/* SX1268 LoRa config */
#define SX1268_RF_FREQUENCY_HZ 435000000UL /* TODO: confirm frequency */
#define SX1268_TX_POWER_DBM 14             /* TODO: confirm power */
#define SX1268_CRC_ENABLED true
#define SX1268_IQ_INVERTED false
#define SX1268_BANDWIDTH LEOS_RADIO_BW_500_KHZ  /* TODO: confirm */
#define SX1268_SPREADING_FACTOR LEOS_RADIO_SF_7 /* TODO: confirm */
#define SX1268_CODING_RATE LEOS_RADIO_CR_4_5    /* TODO: confirm */
#define SX1268_SYNC_WORD 0x12                   /* TODO: confirm */

void config_build_sx1262(leos_radio_config_t *cfg);
void config_build_sx1268(leos_radio_config_t *cfg);
void config_build_sx1262_hw(leos_radio_hw_config_t *cfg);
void config_build_sx1268_hw(leos_radio_hw_config_t *cfg);
