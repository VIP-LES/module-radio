#include "radio_command.h"

#include "cyphal_config.h"
#include "leos/log.h"
#include "packet.pb.h"
#include "pb_decode.h"

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
        .remote_node_id = FLIGHT_COMPUTER_NODE_ID,
        .transfer_id = transfer_id++,
    };
    const struct CanardPayload payload = {.size = 0U, .data = NULL};
    if (leos_cyphal_push(node, &metadata, payload) != LEOS_CYPHAL_OK) {
        LOG_ERROR("failed to queue cutdown request on CAN");
        return false;
    }

    LOG_INFO("authorized cutdown request queued for flight computer");
    return true;
}
