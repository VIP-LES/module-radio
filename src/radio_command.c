#include "radio_command.h"

#include "cyphal_config.h"
#include "leos/log.h"
#include "packet.pb.h"
#include "pb_decode.h"
#include "pico/stdlib.h"
#include "radio.h"
#include "radio_protocol.h"

#define CUTDOWN_ACK_TURNAROUND_MS 100U

bool radio_command_handle(
    leos_cyphal_node_t *node, const uint8_t *data, size_t size)
{
    if (node == NULL || data == NULL || size == 0U) {
        return false;
    }

    leos_Packet packet = leos_Packet_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    if (!pb_decode(&stream, leos_Packet_fields, &packet) ||
        packet.which_payload != leos_Packet_command_tag ||
        packet.payload.command.which_data != leos_CommandPacket_cutdown_tag ||
        packet.payload.command.data.cutdown.authorization_key !=
            CUTDOWN_AUTHORIZATION_KEY) {
        LOG_WARNING("rejected invalid or unauthorized radio command");
        return false;
    }

    static CanardTransferID transfer_id;
    const struct CanardTransferMetadata metadata = {
        .priority = CanardPriorityHigh,
        .transfer_kind = CanardTransferKindRequest,
        .port_id = CUTDOWN_SERVICE_PORT_ID,
        .remote_node_id = CUTDOWN_NODE_ID,
        .transfer_id = transfer_id++,
    };
    const struct CanardPayload payload = {.size = 0U, .data = NULL};
    if (leos_cyphal_push(node, &metadata, payload) != LEOS_CYPHAL_OK) {
        LOG_ERROR("failed to queue cutdown request on CAN");
        return false;
    }

    LOG_INFO("authorized cutdown request queued for flight computer");

    uint8_t ack[RADIO_RF_MAX_PACKET_SIZE];
    const size_t ack_size = radio_protocol_pack_cutdown_ack(
        packet.callsign, packet.sequence, ack, sizeof(ack));
    if (ack_size == 0U) {
        LOG_ERROR("failed to encode cutdown acknowledgment");
    } else {
        sleep_ms(CUTDOWN_ACK_TURNAROUND_MS);
        if (radio_send(ack, ack_size) != 0) {
            LOG_ERROR("failed to transmit cutdown acknowledgment");
        } else {
            LOG_INFO("cutdown acknowledgment transmitted");
        }
    }
    return true;
}
