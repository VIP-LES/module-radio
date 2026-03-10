# module-radio: Dual SX126x Integration Notes

What changes this semester is the radio side:

- the old XBee UART transport goes away completely
- `SX1262` is used primarily for flight telemetry TX
- `SX1268` is used for high-rate EFM TX
- `SX1262` RX is a later mission mode, most likely enabled by a command from the flight computer near end of flight so the payload can listen for cutdown commands

## Half Duplex and Why There Are Two Radios

Each SX126x is half duplex:

- SX1262 cannot receive while SX1262 is transmitting
- SX1268 cannot receive while SX1268 is transmitting

The main reason for two radios is mission behavior:

- EFM data is expected to be high rate, likely around `240 Hz`
- sensor + GPS telemetry is much lower rate, around `1 Hz`
- the EFM traffic should not contend with the telemetry traffic on the same radio

The current `leos_sx126x` design supports this by giving the RP2350 two separate radio contexts on one shared SPI bus, each with its own:

- `NSS`
- `BUSY`
- `RESET`
- `DIO1`

That allows:

- SX1262 telemetry TX while SX1268 is doing EFM TX
- later, SX1262 RX mode while SX1268 remains available for its own use

## What Stays the Same in module-radio

Developers new to the codebase should think of the application as three layers:

1. CAN hardware layer
   - `leos_mcp251xfd`
   - owns the external CAN controller and CAN FD frame movement
2. Cyphal messaging layer
   - `leos_cyphal_transport_mcp251xfd`
   - `leos_cyphal_node`
   - owns Cyphal subscriptions, transfer reassembly, and publication over CAN
3. Radio layer
   - `leos_sx126x`
   - owns SX1262/SX1268 hardware control over SPI

The downstream board application (the module-radio repository) glues those layers together:

- Cyphal subscriptions deliver typed application messages from the flight computer
- the application converts those messages into radio payloads
- the application sends those payloads over the appropriate LoRa radio
- when the radio later receives data, the application converts that data back into a Cyphal message and publishes it on CAN

`main.c`, `config.[ch]`, and `module_setup.[ch]` still have the same job they always do on these boards:

- `main.c`
  - top-level init
  - register Cyphal subscriptions
  - run the simple forever loop
- `config.[ch]`
  - board constants
  - CAN config
  - Cyphal subject IDs and extents
  - radio configs and packet constants
- `module_setup.[ch]`
  - initialize MCP251XFD
  - initialize the Cyphal node
  - initialize both radios
  - install radio GPIO interrupt hooks

## Recommended Source Layout

The old XBee-specific `radio_transport.c` should be replaced by code that separates three concerns cleanly:

```text
src/
  main.c
  config.c
  config.h
  module_setup.c
  module_setup.h
  radio.c
  radio.h
  radio_protocol.c
  radio_protocol.h
  cyphal_bridge.c
  cyphal_bridge.h
```

Responsibilities:

- `radio.c`
  - direct use of `leos_sx126x`
  - radio init
  - radio mode changes
  - radio send
  - radio RX drain
  - radio IRQ deferred processing
- `radio_protocol.c`
  - exact over-the-air packet definitions
  - pack/unpack helpers
  - checksum logic
- `cyphal_bridge.c`
  - Cyphal subscription callbacks
  - Cyphal publish helper for radio RX data
  - mapping between DSDL structs and RF structs

This is intentionally a small separation:

- one file for raw radio control
- one file for RF packet format
- one file for Cyphal-facing conversion logic

That keeps the code readable without wrapping every library call in an unnecessary abstraction layer.

## What Should Be Removed

Remove all XBee-specific logic:

- UART radio init
- AT command helpers
- transparent UART forwarding
- byte-stream assumptions about the radio link
- `check_radio_uart_rx()`

The old radio path was:

- Cyphal payload arrives
- raw bytes are written directly to UART

The new radio path should be:

- Cyphal payload arrives
- deserialize into the generated DSDL struct
- extract the specific fields needed on RF
- pack into a defined RF packet format
- send through `leos_sx126x`

## Build System Changes

The downstream target should link:

```cmake
target_link_libraries(MODULE_RADIO
    pico_stdlib
    leos_log
    leos_mcp251xfd
    leos_cyphal_transport_mcp251xfd
    leos_cyphal_node
    leos_sx126x
)
```

The downstream target must also ensure the SX126x Pico port pin definitions match the actual board wiring.

The current `leos_sx126x` Pico port uses compile-time macros for:

- shared SPI:
  - `LEOS_SX126X_SPI_PORT`
  - `LEOS_SX126X_SPI_BAUD_HZ`
  - `LEOS_SX126X_PIN_SCK`
  - `LEOS_SX126X_PIN_MOSI`
  - `LEOS_SX126X_PIN_MISO`
- SX1262:
  - `LEOS_SX1262_PIN_NSS`
  - `LEOS_SX1262_PIN_BUSY`
  - `LEOS_SX1262_PIN_RESET`
  - `LEOS_SX1262_PIN_DIO1`
- SX1268:
  - `LEOS_SX1268_PIN_NSS`
  - `LEOS_SX1268_PIN_BUSY`
  - `LEOS_SX1268_PIN_RESET`
  - `LEOS_SX1268_PIN_DIO1`

These should be defined in one place in the downstream repo.

## Control Flow

The application should follow this control flow:

1. Initialize logging.
2. Initialize MCP251XFD.
3. Initialize the Cyphal node.
4. Initialize SX1262 with its telemetry TX configuration.
5. Initialize SX1268 with its EFM TX configuration.
6. Register Cyphal subscriptions:
   - consolidated sensor + GPS message
   - EFM message
   - later, if needed, a command message that tells the radio board to place SX1262 into RX mode for cutdown listening
7. Enter the main loop.
8. In every loop iteration:
   - run `leos_mcp251xfd_task(&dev)`
   - run `leos_cyphal_task(&node)`
   - run prompt deferred servicing for any radio IRQs
   - if SX1262 is currently in RX mode and a packet is available, read it and publish it to Cyphal

## Radio Modes by Mission Phase

The intended mission behavior should be reflected directly in software:

- normal flight:
  - SX1262 is used for telemetry TX
  - SX1268 is used for EFM TX
  - SX1262 is not expected to sit in continuous RX for the whole flight
- end-of-flight or recovery phase:
  - the flight computer can command the radio board to place SX1262 into RX mode
  - SX1262 then listens for cutdown or other ground command packets

This means the downstream app should expose an explicit mode transition function in `radio.c`, for example:

```c
int radio_enter_normal_flight_mode(void);
int radio_enter_command_rx_mode(void);
```

The second function should:

- put SX1262 into RX using `leos_sx126x_start_rx(LEOS_RADIO_SX1262)`
- set any local state needed so the main loop knows it should drain and publish SX1262 RX packets

The exact trigger for that switch is not finalized yet. The cleanest likely design is a Cyphal command from the flight computer.

## IRQ Handling: Why the Split Exists

`leos_sx126x` splits radio interrupt handling into two parts:

- `leos_sx126x_handle_dio1_irq(radio)`
- `leos_sx126x_process_irq(radio)`

This is intentional.

`leos_sx126x_handle_dio1_irq()` is the GPIO-ISR-side function. It is small because ISR code should stay small:

- no SPI transfers
- no packet parsing
- no application work
- just latch that DIO1 asserted

`leos_sx126x_process_irq()` is the deferred side:

- it runs outside the ISR
- it performs the SPI transactions needed to drain the radio IRQ status and received packet data

This split is the right design, but it comes with one hard requirement:

- the deferred servicing must happen quickly

The main loop should call the radio IRQ servicing function every iteration, not occasionally. If deferred servicing is delayed too long after DIO1, the radio packet can be lost before software reads it out.

## How the CAN / Cyphal / Radio Interaction Works

For new developers, the flow should be understood in these terms.

### Cyphal input to radio TX

1. MCP251XFD receives CAN frames.
2. `leos_mcp251xfd_task()` and `leos_cyphal_rx_process()` feed them into libcanard.
3. A subscribed Cyphal transfer is completed.
4. The relevant subscription callback in `cyphal_bridge.c` runs.
5. That callback deserializes the DSDL message.
6. The callback converts the typed DSDL data into an RF packet struct.
7. `radio_protocol.c` serializes that RF packet struct into bytes.
8. `radio.c` sends those bytes with `leos_sx126x_send(...)`.

### Radio RX to Cyphal output

1. SX1262 asserts DIO1.
2. The GPIO ISR calls `leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1262)`.
3. The main loop quickly calls `leos_sx126x_process_irq(LEOS_RADIO_SX1262)`.
4. If a packet was received, `leos_sx126x_packet_available(...)` becomes true.
5. The main loop reads the packet with `leos_sx126x_read(...)`.
6. `radio_protocol.c` validates and decodes the RF packet.
7. `cyphal_bridge.c` converts that decoded RF packet into the chosen DSDL message.
8. `cyphal_bridge.c` serializes and publishes it with `leos_cyphal_push(...)`.

## Over-the-Air Packet Format

Do not send raw Cyphal payload bytes over LoRa.

Use a simple fixed-format binary packet with:

- a sync byte
- a version byte
- a message type byte
- a sequence number
- a payload length
- payload bytes
- a CRC16

This is simple to implement, inspect in logs, and evolve later without adding another code generator or runtime dependency.

### Common RF Frame Envelope

All radio packets should use this outer format:

```text
Byte 0   : 0xA5                // sync
Byte 1   : version             // start at 1
Byte 2   : message_type
Byte 3   : sequence
Byte 4   : payload_length      // number of payload bytes
Byte 5.. : payload
Last 2   : CRC16-CCITT-FALSE over bytes 0 through end of payload
```

Rules:

- one packet must always fit inside one SX126x payload
- `payload_length` does not include the CRC bytes
- `sequence` is per message stream and may wrap naturally at `255`
- use `CRC16-CCITT-FALSE` (look this up for more details)
  - polynomial `0x1021`
  - init `0xFFFF`
  - no reflection
  - xorout `0x0000`

### Message Type Values

Use:

```text
0x01 : sensor + GPS telemetry over SX1262
0x02 : EFM telemetry over SX1268
0x03 : cutdown / command packet received on SX1262
```

If more packet types are added later, increment from there.

### Sensor + GPS RF Payload Definition

Payload for `message_type = 0x01` should mirror the final consolidated sensor + GPS DSDL type as closely as possible.

That means:

- the fields in the RF payload should come from the eventual DSDL definition that the other developers are finalizing
- the RF payload should not become a second independent schema with its own unrelated field list
- if the DSDL uses field widths or numeric types that are too expensive over RF, convert them deliberately into a compact binary representation and document that conversion in `radio_protocol.h`

The DSDL definition should remain the semantic source of truth. The RF payload is only the transport representation.

### EFM RF Payload Definition

Payload for `message_type = 0x02` should follow the same rule:

- mirror the final EFM DSDL type as closely as possible
- keep the packed representation compact, fixed-size if practical, and allocation-free
- document every conversion from DSDL field type to RF field type in `radio_protocol.[ch]`

This matters especially for the EFM path because it is expected to run at high rate.

### SX1262 Command RX Payload Definition

When SX1262 is later used in RX mode for cutdown or other commands, use:

Payload for `message_type = 0x03`:

```text
u8  command_id
u8  command_arg_len
u8  command_args[0..N]
```

For the first revision, keep `N <= 16`.

This keeps command parsing simple and allows the received command packet to be turned into a Cyphal message without ambiguity.

## Mapping DSDL to RF Packets

For both telemetry packet types:

- the DSDL definition is the source of truth for field meaning and units
- the RF payload is the compact binary form actually transmitted over LoRa

The intended workflow is:

1. inspect the final Nunavut-generated DSDL header
2. identify the fields that must go on RF
3. define a compact RF struct in `radio_protocol.h`
4. document any unit conversion or width reduction next to that RF struct
5. implement the conversion in `cyphal_bridge.c`

This keeps the packet bodies aligned with the DSDL work that other developers are actively finishing.

## Packet Helpers and Exact Function Names

Put all RF framing and checksum code in `radio_protocol.[ch]`.

Use these functions:

```c
size_t radio_protocol_pack_sensor_gps_frame(
    const sensor_gps_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size);

size_t radio_protocol_pack_efm_frame(
    const efm_radio_frame_t *frame,
    uint8_t *out_buf,
    size_t out_buf_size);

bool radio_protocol_unpack_command_frame(
    const uint8_t *buf,
    size_t len,
    command_radio_frame_t *out_frame);

uint16_t radio_protocol_crc16_ccitt(
    const uint8_t *buf,
    size_t len);
```

Put all DSDL-to-RF and RF-to-DSDL adaptation in `cyphal_bridge.[ch]`.

Use these functions:

```c
bool cyphal_bridge_sensor_gps_to_radio_frame(
    const flight_sensor_gps_dsdl_t *msg,
    sensor_gps_radio_frame_t *out_frame);

bool cyphal_bridge_efm_to_radio_frame(
    const efm_dsdl_t *msg,
    efm_radio_frame_t *out_frame);

bool cyphal_bridge_command_frame_to_dsdl(
    const command_radio_frame_t *frame,
    radio_command_rx_dsdl_t *out_msg);

void cyphal_bridge_on_sensor_gps(
    struct CanardRxTransfer *transfer,
    void *user_reference);

void cyphal_bridge_on_efm(
    struct CanardRxTransfer *transfer,
    void *user_reference);

leos_cyphal_result_t cyphal_bridge_publish_sx1262_rx(
    leos_cyphal_node_t *node);
```

The placeholder type names above should be replaced with the final generated Nunavut type names once the DSDL is finalized.

## Do We Need a Local Radio API?

Not a large one.

There is no need to build a thick wrapper around `leos_sx126x`. That would be abstraction for its own sake.

What is still useful is a very small `radio.c` file so:

- radio initialization is in one place
- mission-mode changes are in one place
- IRQ deferred servicing is in one place
- the rest of the application does not scatter raw SX126x calls everywhere

That file can stay very thin and call `leos_sx126x_*` directly.

Suggested functions in `radio.[ch]`:

```c
int radio_init(void);
void radio_service_irqs(void);
int radio_send_sx1262(const uint8_t *buf, size_t len);
int radio_send_sx1268(const uint8_t *buf, size_t len);
int radio_enter_command_rx_mode(void);
bool radio_sx1262_packet_available(void);
int radio_read_sx1262(uint8_t *buf, size_t buf_size, size_t *out_len, leos_radio_packet_info_t *info);
```

That is enough structure without duplicating the SDK API unnecessarily.

## How radio.c Should Use leos_sx126x

`radio.c` should be the only downstream file that directly calls the `leos_sx126x_*` API. The rest of the app should call thin helpers in `radio.c`.

This keeps:

- radio initialization in one place
- mode transitions in one place
- IRQ servicing in one place
- raw SX126x details out of `main.c` and the Cyphal callbacks

### Startup

`radio_init()` should:

1. build one `leos_radio_config_t` for SX1262
2. build one `leos_radio_config_t` for SX1268
3. call:

```c
leos_sx126x_init(LEOS_RADIO_SX1262, &sx1262_cfg);
leos_sx126x_init(LEOS_RADIO_SX1268, &sx1268_cfg);
```

The easiest way to build each config is:

```c
leos_radio_config_t sx1262_cfg;
leos_sx126x_get_default_config(LEOS_RADIO_SX1262, &sx1262_cfg);
```

then override the fields that matter for the module:

- `rf_frequency_hz`
- `bandwidth`
- `spreading_factor`
- `coding_rate`
- `sync_word`
- `tx_power_dbm`
- `crc_enabled`
- `iq_inverted`

Do the same for SX1268.

### Sending telemetry

`radio_send_sx1262(...)` should be a thin wrapper around:

```c
leos_sx126x_send(LEOS_RADIO_SX1262, buf, len);
```

`radio_send_sx1268(...)` should be a thin wrapper around:

```c
leos_sx126x_send(LEOS_RADIO_SX1268, buf, len);
```

Those wrappers should mainly handle local error checking and logging.

### Entering command RX mode on SX1262

When the flight computer later tells the radio board to start listening for cutdown or other commands, `radio_enter_command_rx_mode()` should call:

```c
leos_sx126x_start_rx(LEOS_RADIO_SX1262);
```

That places SX1262 into continuous RX.

If the application later needs to leave that mode explicitly, add a local helper that calls:

```c
leos_sx126x_standby(LEOS_RADIO_SX1262);
```

### DIO1 ISR side

The GPIO ISR should stay small. It should not touch SPI or parse packets. It should only latch the pending radio IRQ:

```c
leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1262);
leos_sx126x_handle_dio1_irq(LEOS_RADIO_SX1268);
```

as appropriate for the pin that asserted.

### Deferred IRQ servicing

`radio_service_irqs()` should run every loop iteration and should call:

```c
leos_sx126x_process_irq(LEOS_RADIO_SX1262);
leos_sx126x_process_irq(LEOS_RADIO_SX1268);
```

This is the function that actually services the radio interrupt outside ISR context and reads RX data out of the chip when needed.

### Reading received SX1262 packets

When SX1262 is in RX mode, `radio.c` should check for unread data with:

```c
leos_sx126x_packet_available(LEOS_RADIO_SX1262);
```

and read it with:

```c
leos_sx126x_read(
    LEOS_RADIO_SX1262,
    buf,
    buf_size,
    &out_len,
    &info);
```

`info` contains packet metadata such as RSSI and SNR.

### Optional mode inspection

If the downstream app needs to inspect current radio state, `radio.c` may use:

```c
leos_sx126x_mode(LEOS_RADIO_SX1262);
leos_sx126x_mode(LEOS_RADIO_SX1268);
```

but the application should generally drive radio state explicitly with send/start_rx/standby calls rather than relying on inferred behavior.

## module_setup Responsibilities

`module_setup.c` should:

- initialize the MCP251XFD
- initialize the Cyphal node
- attach the MCP251XFD RX callback so Cyphal receives are drained
- initialize SX1262
- initialize SX1268
- install GPIO interrupt handlers for the radio DIO1 pins

At startup, do not automatically place SX1262 into long-term RX mode. Normal startup should leave both radios ready for TX-centric operation.

If the mission later needs command RX mode, `module_setup.c` does not need to own that transition. That transition should happen through a runtime function in `radio.c`, most likely triggered by a Cyphal callback in `cyphal_bridge.c`.

## config Responsibilities

`config.[ch]` should define:

- CAN controller SPI pins and timing
- Cyphal node-ID
- Cyphal subject IDs and extents for:
  - sensor + GPS input
  - EFM input
  - optional radio mode command input
  - radio RX output
- SX1262 LoRa config
  - frequency
  - bandwidth
  - spreading factor
  - coding rate
  - sync word
  - tx power
  - CRC enabled
  - IQ inversion
- SX1268 LoRa config
  - same fields
- RF packet constants
  - message type values
  - max packet size
  - CRC settings if documented in code comments

Prefer helper builders such as:

```c
void config_build_sx1262(leos_radio_config_t *cfg);
void config_build_sx1268(leos_radio_config_t *cfg);
```

## Cyphal Subscription and Publish Flow

Subscriptions should still be registered directly from `main.c` after initialization, because that matches the current board style and keeps application behavior easy to follow.

That means `main.c` should call `leos_cyphal_subscribe(...)` for:

- sensor + GPS callback in `cyphal_bridge.c`
- EFM callback in `cyphal_bridge.c`
- later, possibly a mode-switch or command callback in `cyphal_bridge.c`

This keeps `main.c` as the place where the application’s message-level behavior is visible, while the actual callback bodies stay out of `main.c`.

## How the Cyphal Callbacks Should Work

### Sensor + GPS callback

Location:

- `cyphal_bridge.c`
- function name: `cyphal_bridge_on_sensor_gps(...)`

Steps:

1. Deserialize the incoming transfer into the generated consolidated sensor + GPS DSDL struct.
2. Call `cyphal_bridge_sensor_gps_to_radio_frame(...)`.
3. Call `radio_protocol_pack_sensor_gps_frame(...)`.
4. Call `radio_send_sx1262(...)`.

### EFM callback

Location:

- `cyphal_bridge.c`
- function name: `cyphal_bridge_on_efm(...)`

Steps:

1. Deserialize the incoming transfer into the generated EFM DSDL struct.
2. Call `cyphal_bridge_efm_to_radio_frame(...)`.
3. Call `radio_protocol_pack_efm_frame(...)`.
4. Call `radio_send_sx1268(...)`.

### Command RX publish helper

Location:

- `cyphal_bridge.c`
- function name: `cyphal_bridge_publish_sx1262_rx(...)`

Steps:

1. Call `radio_read_sx1262(...)`.
2. Call `radio_protocol_unpack_command_frame(...)`.
3. Call `cyphal_bridge_command_frame_to_dsdl(...)`.
4. Serialize the output DSDL type.
5. Publish it with `leos_cyphal_push(...)`.

## Main Loop Shape

The application loop should stay simple:

```c
while (true) {
    leos_mcp251xfd_task(&dev);
    leos_cyphal_task(&node);

    radio_service_irqs();

    if (radio_sx1262_packet_available()) {
        cyphal_bridge_publish_sx1262_rx(&node);
    }
}
```

If SX1262 is not currently in command RX mode, `radio_sx1262_packet_available()` should simply return false because the radio is not being held in RX.

## DSDL Ambiguity and How To Contain It

The exact DSDL types for:

- consolidated sensor + GPS
- EFM
- radio RX publish

are still not finalized.

That should not leak through the whole application.

Keep all DSDL-specific type knowledge in `cyphal_bridge.c`, specifically in:

- `cyphal_bridge_sensor_gps_to_radio_frame(...)`
- `cyphal_bridge_efm_to_radio_frame(...)`
- `cyphal_bridge_command_frame_to_dsdl(...)`

When the final DSDL types are ready, those are the primary functions that should change.

## Minimum Deliverable

The first acceptable revision of `module-radio` should have:

- all XBee UART code removed
- existing MCP251XFD + Cyphal initialization preserved
- both SX126x radios initialized through `leos_sx126x`
- concrete RF packet definitions implemented in `radio_protocol.[ch]`
- one Cyphal callback that sends sensor + GPS data over SX1262
- one Cyphal callback that sends EFM data over SX1268
- prompt DIO1 deferred servicing implemented for SX1262 and SX1268
- a clean path for later switching SX1262 into command RX mode and publishing received command packets onto Cyphal

That is enough to move the board from the old XBee model to the new dual-LoRa model without painting the code into a corner.
