# Hardware Design

## Design status

The selected functional parts come from the project objective. Protection values, connector part numbers, PCB layout and environmental qualification remain TBD and require schematic review before construction.

## Engine-side node

| Function | Selected part or rule | Status |
|---|---|---|
| MCU/radio | ESP32-WROOM development board | Selected architecture |
| SmartCraft receiver | SN65HVD230 | Selected architecture |
| Input supply | SmartCraft switched +12 V through a local fuse | Selected architecture; pin and fuse value TBD |
| Conversion | 12 V to regulated 5 V buck, then ESP32 VIN | Selected architecture; module rating TBD |
| Transceiver supply | ESP32 regulated 3.3 V to SN65HVD230 VCC | Required by part family |
| SmartCraft termination | None added | Hard safety requirement |
| CAN transmit path | Physically absent | Hard safety requirement |

### Physical receive-only implementation

Firmware listen-only mode is defense in depth, not the primary protection.

- SN65HVD230 `D/TXD` must have no trace, wire, jumper, header or test pad leading to an ESP32 GPIO.
- `D/TXD` must be hard-wired to 3.3 V so the driver command is permanently recessive. TI defines LOW as dominant and HIGH as recessive.
- SN65HVD230 `R/RXD` connects to the selected ESP32 TWAI receive GPIO.
- Any ESP-IDF-required TWAI transmit GPIO is an unused ESP32 pin and remains physically unconnected to the transceiver.
- Do not populate a SmartCraft-side 120 ohm resistor.
- `RS` mode/slope configuration and its resistor value remain TBD after bitrate and signal-integrity review; it must not create a transmit control path.
- The PCB review must prove that no solder bridge option can casually reconnect TXD.

This prevents firmware, boot-ROM or GPIO-failure behavior from commanding dominant SmartCraft bits. Component internal failure is outside the absolute guarantee and is addressed by part selection, layout and validation.

## Helm-side node

| Function | Selected part or rule | Status |
|---|---|---|
| MCU/radio | ESP32-WROOM development board | Selected architecture |
| NMEA CAN transceiver | MCP2562 | Selected architecture |
| MCP2562 VDD | Regulated 5 V | Required |
| MCP2562 VIO | ESP32 3.3 V | Required |
| MCU interface | ESP32 TWAI TXD/RXD to MCP2562 TXD/RXD | Required; GPIOs TBD |
| Input supply | External switched 12 V through local fuse | Selected architecture; fuse value TBD |
| Conversion | 12 V to regulated 5 V buck, then ESP32 VIN and MCP2562 VDD | Selected architecture |

Microchip documents MCP2562 VDD operation at 4.5–5.5 V and a VIO input intended for direct connection to 1.8–5 V logic. The design uses 5 V VDD and 3.3 V VIO as specified by the project objective.

## Dedicated prototype NMEA 2000 segment

- Only Garmin Echomap and the ESP32 gateway are connected.
- Use a linear, very short CAN segment with twisted CAN-H/CAN-L.
- Install exactly two 120 ohm terminators, one at each physical end.
- Expected powered-off differential resistance is approximately 60 ohms before devices are energized.
- Carry `NET-S` and `NET-C` through the segment/connectors.
- Feed network power from an external fused 12 V source. Garmin is not the source of NMEA network power.
- Keep the NMEA segment electrically separate from SmartCraft CAN. Sharing a vessel battery does not authorize sharing CAN conductors.

## Power philosophy

Each ESP32 node uses:

```text
12 V source -> local fuse -> 12 V-to-5 V buck -> ESP32 VIN
```

Design review must establish:

- Fuse rating and placement close to the source.
- Buck input range, reverse-polarity behavior, load-dump/transient tolerance and output ripple.
- ESP32 peak Wi-Fi current margin.
- Ground/reference topology and whether additional isolation is needed.
- Safe behavior during brownout and power sequencing.

## Protection components requiring selection

- Reverse-polarity protection.
- Supply TVS and input filtering.
- CAN-line TVS compatible with bus capacitance and common-mode limits.
- Optional common-mode choke after signal-integrity testing.
- Decoupling at each IC according to its datasheet.
- Waterproof connectors, enclosure, strain relief and conformal/environmental protection.

These are **TBD**, not omitted requirements.

