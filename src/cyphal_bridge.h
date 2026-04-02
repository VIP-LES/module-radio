#pragma once

/*
 * cyphal_bridge.h
 *
 * Adaptation layer between Cyphal DSDL messages and RF packet structs.
 *
 * This file is the only place in the application that should know about
 * the generated Nunavut DSDL types. When the DSDL definitions are
 * finalized, only the functions in cyphal_bridge.c that convert between
 * DSDL structs and RF structs should need to change.
 *
 * Responsibilities:
 *   - Cyphal subscription callbacks (on_sensor_gps, on_efm)
 *   - DSDL-to-RF frame conversion helpers
 *   - RF-to-DSDL conversion helper (for received command packets)
 *   - Cyphal publish helper for SX1262 RX data
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "canard.h"
#include "leos/cyphal/node.h"
#include <leos/aggregate/LowRate_0_1.h>
#include <leos/efm/ADC_0_1.h>
#include "radio_protocol.h"

/*
 * Keep the generated telemetry DSDL types hidden behind local aliases so
 * the rest of the application only depends on this bridge surface.
 *
 * Command RX is still a placeholder because there is not yet a dedicated
 * DSDL message for a received radio command packet.
 */
typedef leos_aggregate_LowRate_0_1 flight_sensor_gps_dsdl_t;
typedef leos_efm_ADC_0_1 efm_dsdl_t;

typedef struct
{
    /* TODO: fill in radio command RX DSDL fields */
    uint8_t _placeholder;
} radio_command_rx_dsdl_t;

/*
 * DSDL-to-RF conversion helpers.
 *
 * These are the primary functions that change when the DSDL types are
 * finalized. Everything else in the bridge should remain stable.
 */

/*
 * cyphal_bridge_sensor_gps_to_radio_frame
 *
 * Extracts fields from the consolidated sensor + GPS DSDL struct and
 * packs them into the RF frame struct. Any unit conversion or field
 * width reduction from DSDL types to RF types should happen here and
 * should be documented in radio_protocol.h next to the RF struct.
 *
 * Returns true on success, false if msg or out_frame is NULL.
 */
bool cyphal_bridge_sensor_gps_to_radio_frame(
    const flight_sensor_gps_dsdl_t *msg,
    sensor_gps_radio_frame_t *out_frame);

/*
 * cyphal_bridge_efm_to_radio_frame
 *
 * Extracts fields from the EFM DSDL struct and packs them into the
 * RF frame struct. Keep this path as lean as possible — EFM runs at
 * ~240 Hz.
 *
 * Returns true on success, false if msg or out_frame is NULL.
 */
bool cyphal_bridge_efm_to_radio_frame(
    const efm_dsdl_t *msg,
    efm_radio_frame_t *out_frame);

/*
 * cyphal_bridge_command_frame_to_dsdl
 *
 * Converts a decoded command RF frame into the radio command RX DSDL
 * struct for publishing onto Cyphal.
 *
 * Returns true on success, false if either pointer is NULL.
 */
bool cyphal_bridge_command_frame_to_dsdl(
    const command_radio_frame_t *frame,
    radio_command_rx_dsdl_t *out_msg);

/*
 * Cyphal subscription callbacks.
 *
 * Register these with leos_cyphal_subscribe() from main.c.
 * user_reference is not used and may be NULL.
 */

/*
 * cyphal_bridge_on_sensor_gps
 *
 * Deserializes the incoming transfer, converts to RF frame, packs the
 * RF packet, and transmits over SX1262.
 */
void cyphal_bridge_on_sensor_gps(
    struct CanardRxTransfer *transfer,
    void *user_reference);

/*
 * cyphal_bridge_on_efm
 *
 * Deserializes the incoming transfer, converts to RF frame, packs the
 * RF packet, and transmits over SX1268.
 */
void cyphal_bridge_on_efm(
    struct CanardRxTransfer *transfer,
    void *user_reference);

/*
 * cyphal_bridge_publish_sx1262_rx
 *
 * Called from the main loop when radio_sx1262_packet_available() is
 * true. Reads the packet, unpacks the command RF frame, converts to
 * DSDL, and publishes onto Cyphal.
 *
 * Returns LEOS_CYPHAL_OK on success.
 */
leos_cyphal_result_t cyphal_bridge_publish_sx1262_rx(
    leos_cyphal_node_t *node);
