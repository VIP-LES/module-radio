#include "cyphal_bridge.h"

#include <string.h>

#include "canard.h"
#include "config.h"
#include "leos/cyphal/node.h"
#include "leos/log.h"
#include "leos/sx126x.h"
#include "radio.h"
#include "radio_protocol.h"

/* -------------------------------------------------------------------------
 * DSDL-to-RF conversion helpers
 *
 * These are the functions that change when DSDL types are finalized.
 * All other bridge logic should remain stable.
 * ---------------------------------------------------------------------- */

bool cyphal_bridge_sensor_gps_to_radio_frame(
    const flight_sensor_gps_dsdl_t *msg,
    sensor_gps_radio_frame_t *out_frame)
{
    if ((msg == NULL) || (out_frame == NULL))
    {
        return false;
    }

    memset(out_frame, 0, sizeof(*out_frame));

    /*
     * TODO: Replace the body of this function once the consolidated
     * sensor + GPS DSDL type is finalized. Map each DSDL field to the
     * corresponding RF struct field. Document any unit conversion or
     * width reduction next to the RF struct definition in
     * radio_protocol.h.
     *
     * Example (placeholder until DSDL is ready):
     *
     *   out_frame->t_pkt_us         = msg->t_pkt.microsecond;
     *   out_frame->bme688.humidity  = msg->bme688.humidity;
     *   out_frame->bme688_valid     = msg->bme688.valid;
     *   ...
     */

    return true;
}

bool cyphal_bridge_efm_to_radio_frame(
    const efm_dsdl_t *msg,
    efm_radio_frame_t *out_frame)
{
    if ((msg == NULL) || (out_frame == NULL))
    {
        return false;
    }

    memset(out_frame, 0, sizeof(*out_frame));

    /*
     * TODO: Replace the body of this function once the EFM DSDL type
     * is finalized. This path runs at ~240 Hz — keep it lean.
     *
     * Example (placeholder until DSDL is ready):
     *
     *   out_frame->t_pkt_us            = msg->t_pkt.microsecond;
     *   out_frame->adc1_ch1_diff       = msg->adc1_ch1_diff;
     *   out_frame->adc2_ch4_breakbeam  = msg->adc2_ch4_breakbeam;
     *   ...
     */

    return true;
}

bool cyphal_bridge_command_frame_to_dsdl(
    const command_radio_frame_t *frame,
    radio_command_rx_dsdl_t *out_msg)
{
    if ((frame == NULL) || (out_msg == NULL))
    {
        return false;
    }

    memset(out_msg, 0, sizeof(*out_msg));

    /*
     * TODO: Replace the body of this function once the radio command
     * RX DSDL type is finalized. Map command_id, command_arg_len, and
     * command_args into the appropriate DSDL fields.
     *
     * Example (placeholder until DSDL is ready):
     *
     *   out_msg->command_id      = frame->command_id;
     *   out_msg->command_arg_len = frame->command_arg_len;
     *   memcpy(out_msg->command_args,
     *          frame->command_args,
     *          frame->command_arg_len);
     */

    return true;
}

/* -------------------------------------------------------------------------
 * Cyphal subscription callbacks
 * ---------------------------------------------------------------------- */

void cyphal_bridge_on_sensor_gps(
    struct CanardRxTransfer *transfer,
    void *user_reference)
{
    (void)user_reference;

    if (transfer == NULL)
    {
        return;
    }

    /*
     * Step 1: Deserialize the incoming transfer into the DSDL struct.
     *
     * TODO: Replace this section with the real Nunavut deserialize call
     * once the generated header is available. The deserializer writes
     * into msg and returns a byte count or error.
     *
     * Example:
     *
     *   flight_sensor_gps_dsdl_t msg;
     *   int32_t result = flight_sensor_gps_dsdl_1_0_deserialize_(
     *       &msg,
     *       transfer->payload,
     *       &transfer->payload_size);
     *   if (result < 0) {
     *       LOG_ERROR("sensor_gps deserialize failed: %d", result);
     *       return;
     *   }
     */
    flight_sensor_gps_dsdl_t msg;
    memset(&msg, 0, sizeof(msg));
    /* TODO: deserialize transfer->payload into msg */

    /* Step 2: Convert DSDL struct to RF frame. */
    sensor_gps_radio_frame_t rf_frame;
    if (!cyphal_bridge_sensor_gps_to_radio_frame(&msg, &rf_frame))
    {
        LOG_ERROR("sensor_gps_to_radio_frame failed");
        return;
    }

    /* Step 3: Pack the RF frame into a wire-format packet. */
    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];
    size_t packed_len = radio_protocol_pack_sensor_gps_frame(
        &rf_frame,
        buf,
        sizeof(buf));

    if (packed_len == 0U)
    {
        LOG_ERROR("pack_sensor_gps_frame failed");
        return;
    }

    /* Step 4: Transmit over SX1262. */
    int rc = radio_send_sx1262(buf, packed_len);
    if (rc != 0)
    {
        LOG_ERROR("radio_send_sx1262 failed: %d", rc);
    }
}

void cyphal_bridge_on_efm(
    struct CanardRxTransfer *transfer,
    void *user_reference)
{
    (void)user_reference;

    if (transfer == NULL)
    {
        return;
    }

    /*
     * Step 1: Deserialize the incoming transfer into the DSDL struct.
     *
     * TODO: Replace with the real Nunavut deserialize call once the
     * generated header is available.
     *
     * Example:
     *
     *   efm_dsdl_t msg;
     *   int32_t result = efm_dsdl_1_0_deserialize_(
     *       &msg,
     *       transfer->payload,
     *       &transfer->payload_size);
     *   if (result < 0) {
     *       LOG_ERROR("efm deserialize failed: %d", result);
     *       return;
     *   }
     */
    efm_dsdl_t msg;
    memset(&msg, 0, sizeof(msg));
    /* TODO: deserialize transfer->payload into msg */

    /* Step 2: Convert DSDL struct to RF frame. */
    efm_radio_frame_t rf_frame;
    if (!cyphal_bridge_efm_to_radio_frame(&msg, &rf_frame))
    {
        LOG_ERROR("efm_to_radio_frame failed");
        return;
    }

    /* Step 3: Pack the RF frame into a wire-format packet. */
    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];
    size_t packed_len = radio_protocol_pack_efm_frame(
        &rf_frame,
        buf,
        sizeof(buf));

    if (packed_len == 0U)
    {
        LOG_ERROR("pack_efm_frame failed");
        return;
    }

    /* Step 4: Transmit over SX1268. */
    int rc = radio_send_sx1268(buf, packed_len);
    if (rc != 0)
    {
        LOG_ERROR("radio_send_sx1268 failed: %d", rc);
    }
}

/* -------------------------------------------------------------------------
 * SX1262 RX publish helper
 * ---------------------------------------------------------------------- */

leos_cyphal_result_t cyphal_bridge_publish_sx1262_rx(
    leos_cyphal_node_t *node)
{
    if (node == NULL)
    {
        return LEOS_CYPHAL_INVALID_ARGUMENT;
    }

    /* Step 1: Read the raw packet from SX1262. */
    uint8_t buf[RADIO_RF_MAX_PACKET_SIZE];
    size_t rx_len = 0U;
    leos_radio_packet_info_t info;

    int rc = radio_read_sx1262(buf, sizeof(buf), &rx_len, &info);
    if (rc != 0)
    {
        LOG_ERROR("radio_read_sx1262 failed: %d", rc);
        return LEOS_CYPHAL_TX_ERROR;
    }

    LOG_INFO("SX1262 RX: %u bytes, RSSI=%d SNR=%d",
             (unsigned)rx_len,
             (int)info.rssi_dbm,
             (int)info.snr_db);

    /* Step 2: Validate and unpack the command RF frame. */
    command_radio_frame_t cmd_frame;
    if (!radio_protocol_unpack_command_frame(buf, rx_len, &cmd_frame))
    {
        LOG_ERROR("unpack_command_frame failed (bad packet)");
        return LEOS_CYPHAL_TX_ERROR;
    }

    /* Step 3: Convert RF frame to DSDL struct. */
    radio_command_rx_dsdl_t dsdl_msg;
    if (!cyphal_bridge_command_frame_to_dsdl(&cmd_frame, &dsdl_msg))
    {
        LOG_ERROR("command_frame_to_dsdl failed");
        return LEOS_CYPHAL_TX_ERROR;
    }

    /*
     * Step 4: Serialize and publish the DSDL message onto Cyphal.
     *
     * TODO: Replace with the real Nunavut serialize call and the
     * correct subject ID once the DSDL type and publish subject are
     * finalized.
     *
     * Example:
     *
     *   uint8_t serialized[64];
     *   size_t serialized_size = sizeof(serialized);
     *   int32_t result = radio_command_rx_dsdl_1_0_serialize_(
     *       &dsdl_msg,
     *       serialized,
     *       &serialized_size);
     *   if (result < 0) {
     *       LOG_ERROR("command_rx serialize failed: %d", result);
     *       return LEOS_CYPHAL_ERROR;
     *   }
     *
     *   return leos_cyphal_push(
     *       node,
     *       CYPHAL_PUB_RADIO_CMD_RX_ID,
     *       serialized,
     *       serialized_size);
     */

    /* TODO: remove this stub once DSDL publish is implemented */
    LOG_INFO("command_rx DSDL publish: not yet implemented (DSDL pending)");
    (void)dsdl_msg;

    return LEOS_CYPHAL_OK;
}
