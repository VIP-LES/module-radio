#pragma once

/* Cyphal settings for module-radio. Kept separate from the root config.h
 * (radio pins) so the two don't collide. */

/* This radio node's Cyphal node ID. */
#define CYPHAL_NODE_ID 20u

/* Radio cutdown requests target the cutdown node. */
#define CUTDOWN_NODE_ID 13u
#define CUTDOWN_AUTHORIZATION_KEY 0xDEADBEEFu
#define CUTDOWN_SERVICE_PORT_ID 104u /* leos.cutdown.RadioCutdown.0.1 */

/* Fixed subject IDs, read from the FC's generated DSDL (fc/config.py):
 *   leos.aggregate.LowRate_0_1 -> 1500   (carries the sensors, GPS, both EFMs and the geiger)
 * If the DSDL changes, re-run on the FC:
 *   python -c "from fc import config as c; print(c.PORT_LOW_AGG)" */
#define CYPHAL_SUB_LOWRATE_ID 1500u

/* The flight computer publishes one low-rate packet per second. */
#define CYPHAL_LOWRATE_TARGET_PPS 1u
#define CYPHAL_LOWRATE_TARGET_PERIOD_MS 1000u

/* Goes into leos.Packet.callsign. Max 16 ASCII characters. Set the real one. */
#define RADIO_CALLSIGN "KN4MXH"
