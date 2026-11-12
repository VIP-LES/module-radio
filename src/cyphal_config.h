#pragma once

/* Cyphal settings for module-radio. Kept separate from the root config.h
 * (radio pins) so the two don't collide. */

/* This node's ID. FC uses 10 (time master), 11 (lowrate agg), 12 (logger). */
#define CYPHAL_NODE_ID 13u

/* Radio cutdown requests target the flight computer's Cyphal node. */
#define FLIGHT_COMPUTER_NODE_ID 10u
#define CUTDOWN_AUTHORIZATION_KEY 0xDEADBEEFu
#define CUTDOWN_SERVICE_PORT_ID 101u /* leos.cutdown.Cutdown.0.2 */

/* Fixed subject IDs, read from the FC's generated DSDL (fc/config.py):
 *   leos.aggregate.LowRate_0_1 -> 1500   (carries the sensors, GPS, both EFMs and the geiger)
 * If the DSDL changes, re-run on the FC:
 *   python -c "from fc import config as c; print(c.PORT_LOW_AGG)" */
#define CYPHAL_SUB_LOWRATE_ID 1500u

/* Goes into leos.Packet.callsign. Max 16 ASCII characters. Set the real one. */
#define RADIO_CALLSIGN "LEOS"
