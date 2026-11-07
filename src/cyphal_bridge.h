#pragma once

#include "canard.h"
#include "leos/aggregate/LowRate_0_1.h"   /* generated Nunavut headers: adjust path to your include layout */
#include "leos/efm/ADC_0_2.h"

#include "cyphal_config.h"

typedef leos_aggregate_LowRate_0_1 flight_lowrate_dsdl_t;
typedef leos_efm_ADC_0_2           efm_dsdl_t;

#define CYPHAL_SUB_LOWRATE_EXTENT leos_aggregate_LowRate_0_1_EXTENT_BYTES_
#define CYPHAL_SUB_EFM_EXTENT     leos_efm_ADC_0_2_EXTENT_BYTES_

/* Subscription callbacks: Cyphal transfer -> RF packet -> radio_send(). */
void cyphal_bridge_on_lowrate(struct CanardRxTransfer *transfer, void *user_reference);
void cyphal_bridge_on_efm(struct CanardRxTransfer *transfer, void *user_reference);
