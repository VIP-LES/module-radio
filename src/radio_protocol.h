#ifndef RADIO_PROTOCOL_H
#define RADIO_PROTOCOL_H
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define RADIO_PROTOCOL_SYNC_BYTE 0xA5
#define RADIO_PROTOCOL_VERSION 1

#define RADIO_MSG_SENSOR_GPS 0x01
#define RADIO_MSG_EFM 0x02
#define RADIO_MSG_COMMAND 0x03

size_t radio_protocol_pack_sensor_gps_frame(...)
size_t radio_protocol_pack_efm_frame(...)
bool radio_protocol_unpack_command_frame(...)
uint16_t radio_protocol_crc16_ccitt(...)

// #define RADIO_MAX_PAYLOAD_SIZE 128 check in config.c
// #define RADIO_MAX_COMMAND_ARGS 16 might not be right

typedef struct {

}

typedef struct {
    
}