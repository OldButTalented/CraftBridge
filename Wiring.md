# Wiring

## Master reference

See the master Mermaid diagram in [`Architecture_Overview.md`](Architecture_Overview.md). This document provides net-level wiring rules and does not replace that diagram.

## Engine-side node wiring

| From | To | Rule |
|---|---|---|
| SmartCraft CAN-H | SN65HVD230 CANH | Passive tap only; pinout must be verified before connection |
| SmartCraft CAN-L | SN65HVD230 CANL | Passive tap only; pinout must be verified before connection |
| SmartCraft reference/GND | SN65HVD230 GND and engine-node reference | Connection and ground strategy require harness verification |
| SN65HVD230 R/RXD | ESP32 TWAI RX GPIO | GPIO TBD |
| SN65HVD230 D/TXD | 3.3 V | Hard-wired recessive; no GPIO, header, jumper or test pad |
| SN65HVD230 VCC | ESP32 3.3 V rail | Not 5 V |
| SN65HVD230 RS | Mode/slope network | Value TBD; no software transmit control |
| SmartCraft switched +12 V | Engine-node fuse input | Exact connector pin must be verified |
| Fuse output | Buck input | Fuse value TBD |
| Buck 5 V output | ESP32 VIN | Buck current and transient rating TBD |
| ESP32 TWAI TX GPIO | No connection | Use only an unused, physically unwired GPIO if required by the firmware API |

Forbidden on the SmartCraft side:

- No 120 ohm termination.
- No ESP32-to-transceiver TX path.
- No direct 12 V to ESP32 or SN65HVD230.
- No connection until CAN-H, CAN-L, ground/reference and switched +12 V pins are independently verified.

## Helm-side node wiring

| From | To | Rule |
|---|---|---|
| ESP32 TWAI TX GPIO | MCP2562 TXD | GPIO TBD |
| MCP2562 RXD | ESP32 TWAI RX GPIO | GPIO TBD |
| Buck 5 V | MCP2562 VDD | 5 V supply |
| ESP32 3.3 V | MCP2562 VIO | 3.3 V logic supply |
| Common local ground | MCP2562 VSS and ESP32 GND | Local reference |
| MCP2562 CANH | Prototype segment CAN-H | Twisted pair with CAN-L |
| MCP2562 CANL | Prototype segment CAN-L | Twisted pair with CAN-H |
| External switched 12 V | Helm-node fuse | Separate protected branch |
| Helm-node fuse | Buck input | Fuse value TBD |
| Buck 5 V | ESP32 VIN | Same power philosophy as engine node |

## Prototype NMEA 2000 segment wiring

```text
120 ohm terminator
        |
CAN-H / CAN-L / NET-S / NET-C ---- Gateway ---- Garmin Echomap
        |
120 ohm terminator at opposite physical end

External 12 V source -> NMEA fuse -> NET-S
External return --------------------> NET-C
```

The physical implementation should use proper NMEA 2000-compatible connectors or a clearly labelled bench harness. `NET-S` and `NET-C` must be shown on the schematic even if the gateway electronics use a separate fused buck branch.

## Pre-power continuity checks

1. ESP32 TX to SN65HVD230 D/TXD: open circuit.
2. SN65HVD230 D/TXD to 3.3 V: hard connection as designed.
3. SmartCraft CAN-H to CAN-L through the new node alone: no added 120 ohm path.
4. Prototype NMEA CAN-H to CAN-L with both terminators installed: approximately 60 ohms.
5. No continuity between SmartCraft CAN-H/L and NMEA CAN-H/L.
6. Correct polarity and no short between `NET-S` and `NET-C`.

## TBD before schematic release

- Exact SmartCraft connector pins and mating connector.
- ESP32 GPIO assignments and board variant.
- Fuse and wire sizes.
- Protection devices and grounding/isolation strategy.
- NMEA connector arrangement and measured cable length.
- Garmin model-specific power and NMEA port details.

