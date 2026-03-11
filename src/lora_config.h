#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "leos/mcp251xfd/config.h"
#include "leos_sx126x.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

// Normal Radio Subject IDS
#define CYPHAL_SUB_SENSOR_GPS_ID 7509
#define CYPHAL_SUB_SENSOR_GPS_EXTENT 64

// EFM Radio Subject IDS
#define CYPHAL_SUB_EFM_ID 7510
#define CYPHAL_SUB_EFM_EXTENT 64

// Cutdown Radio Subject IDS
#define CYPHAL_SUB_RADIO_MODE_CMD_ID 7511
#define CYPHAL_SUB_RADIO_MODE_CMD_EXTENT 16

// Publish Radio Subject IDS
#define CYPHAL_PUB_RADIO_RX_ID 7512
#define CYPHAL_PUB_RADIO_RX_EXTENT 32

/* RF frame constants */
#define RADIO_RF_SYNC_BYTE 0xA5
#define RADIO_RF_VERSION 1u
#define RADIO_RF_MSG_SENSOR_GPS 0x01u
#define RADIO_RF_MSG_EFM 0x02u
#define RADIO_RF_MSG_COMMAND 0x03u
#define RADIO_RF_MAX_PACKET_SIZE 255u
#define RADIO_RF_MAX_COMMAND_ARGS 16u

/* SX1262 config */
#define SX1262_RF_FREQUENCY_HZ 915000000UL // 915 mhz frequency probably needs to change
#define SX1262_TX_POWER_DBM 14             // power tx value probably needs to change
#define SX1262_CRC_ENABLED true
#define SX1262_IQ_INVERTED false

/* SX1268 config */
#define SX1268_RF_FREQUENCY_HZ 915000000UL // 915 mhz frequency probably needs to change
#define SX1268_TX_POWER_DBM 14             // power tx value probably needs to change
#define SX1268_CRC_ENABLED true
#define SX1268_IQ_INVERTED false

void config_build_sx1262(leos_radio_config_t *cfg);
void config_build_sx1268(leos_radio_config_t *cfg);