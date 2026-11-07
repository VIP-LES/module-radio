#pragma once

/* Cyphal settings for module-radio. Kept separate from the root config.h
 * (radio pins) so the two don't collide. */

/* This node's ID. FC uses 10 (time master), 11 (lowrate agg), 12 (logger). */
#define CYPHAL_NODE_ID 13u

/* Fixed subject IDs, read from the FC's generated DSDL (fc/config.py):
 *   leos.aggregate.LowRate_0_1 -> 1500
 *   leos.efm.ADC_0_2           -> 1400
 * If the DSDL changes, re-run on the FC:
 *   python -c "from fc import config as c; print(c.PORT_LOW_AGG, c.PORT_EFM)" */
#define CYPHAL_SUB_LOWRATE_ID 1500u
#define CYPHAL_SUB_EFM_ID     1400u

/* Goes into leos.Packet.callsign. Max 16 ASCII characters. Set the real one. */
#define RADIO_CALLSIGN "LEOS"