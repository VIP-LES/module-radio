#pragma once

#include "canard.h"
#include "leos/aggregate/LowRate_0_1.h"   /* generated Nunavut headers: adjust path to your include layout */

#include "cyphal_config.h"

typedef leos_aggregate_LowRate_0_1 flight_lowrate_dsdl_t;

#define CYPHAL_SUB_LOWRATE_EXTENT leos_aggregate_LowRate_0_1_EXTENT_BYTES_

/* Subscription callback: decode the Cyphal transfer and keep the latest frame. It never
 * transmits, so CAN receive is never blocked by the radio. */
void cyphal_bridge_on_lowrate(struct CanardRxTransfer *transfer, void *user_reference);

/* Call from the main loop: sends the pending frame over the radio. */
void cyphal_bridge_service(void);

/* Prints counters (rx / decode failures / tx ok / tx failures / held-stale blocks). */
void cyphal_bridge_log_stats(void);