#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "leos/mcp251xfd/config.h"
#include "leos_sx126x.h"

extern leos_mcp251xfd_hw_t can_hw_config;
extern leos_mcp251xfd_config_t can_config;

// Normal Radio Subject IDS
#define CYPHAL_SUB_SENSOR_GPS_ID 1
#define CYPHAL_SUB_SENSOR_GPS_EXTENT 64 // probably needs to change

// EFM Radio Subject IDS
#define CYPHAL_SUB_EFM_ID 2
#define CYPHAL_SUB_EFM_EXTENT 64 // probably needs to change

// Cutdown Radio Subject IDS
#define CYPHAL_SUB_RADIO_MODE_CMD_ID 3
#define CYPHAL_SUB_RADIO_MODE_CMD_EXTENT 16 // probably needs to change

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

//------NEEDS TO CHANGE!!!!!---------
#define SX1262_BANDWIDTH 10000          // NEEDS TO CHANGE!!!!!!
#define SX1262_SPREADING_FACTOR 1000000 // NEEDS TO CHANGE!!!!!!
#define SX1262_CODING_RATE 1000000      // NEEDS TO CHANGE!!!!!
#define SX1262_SYNC_WORD 1000000        // NEEDS TO CHANGE!!!!!

/* SX1268 config */
#define SX1268_RF_FREQUENCY_HZ 435000000UL // 435 mhz frequency probably needs to change
#define SX1268_TX_POWER_DBM 14             // power tx value probably needs to change
#define SX1268_CRC_ENABLED true
#define SX1268_IQ_INVERTED false

//------NEEDS TO CHANGE!!!!!---------
#define SX1262_BANDWIDTH 10000          // NEEDS TO CHANGE!!!!!!
#define SX1262_SPREADING_FACTOR 1000000 // NEEDS TO CHANGE!!!!!!
#define SX1262_CODING_RATE 1000000      // NEEDS TO CHANGE!!!!!
#define SX1262_SYNC_WORD 1000000        // NEEDS TO CHANGE!!!!!

void config_build_sx1262(leos_radio_config_t *cfg);
void config_build_sx1268(leos_radio_config_t *cfg);