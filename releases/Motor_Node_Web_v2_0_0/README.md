# CraftBridge Motor Node Web v2.0.0

## Release status

- Firmware: **Released / physically verified / accepted / frozen**
- Development firmware baseline: `3a37f6e1603241fe8cd43c758aa4d37e80c8508d`
- Scope: current Motor Node firmware with SmartCraft/Web plus ESP-NOW output for Instrument Node communication
- Instrument Node firmware: **In development / not released**

This release preserves the verified SmartCraft acquisition, session, decoder and freshness behavior and the existing Motor Node Web interface. It adds ESP-NOW transmission of normalized engine values and is prepared for Instrument Node pairing/communication.

The first iteration uses compile-time peer configuration. Dynamic Web-based pairing/configuration is not implemented. Instrument Node ESP-NOW reception and OLED display are physically validated, but Instrument Node NMEA 2000 functionality is still under development. The Instrument Node firmware is not yet released.

## Architecture and scope

`SmartCraft CAN -> verified session/decoder -> normalized engine data -> Web + ESP-NOW`

ESP-NOW characteristics:

- Protocol version: `1`
- Packet size: `40 bytes`
- Default/configured Wi-Fi channel: `6`
- Transmission interval: `250 ms` (`4 Hz`)
- Peer configuration: compile time

Before building, replace `kEspNowPeerMac` in `firmware/CraftBridge_Motor_Node_Web_v2_0_0/Config.h` with your Instrument Node **STA MAC**. Use [CraftBridge Node Identity](../CraftBridge_Node_Identity/README.md) to read it. The non-zero MAC currently present in the frozen source is a device-specific development/test configuration and must not be used for another device.

The packet contains normalized RPM, coolant temperature, runtime hours, oil pressure, battery voltage, fuel flow, boot-relative fuel used and boot-relative trip time. It does not bridge raw SmartCraft CAN frames.

## Physical acceptance

The following bench chain passed physically:

`ECU simulator -> SmartCraft CAN/CANable -> Motor Node v2 -> ESP-NOW -> Instrument Node -> OLED`

Observed acceptance included unchanged Motor Node Web behavior, Wi-Fi SoftAP and ESP-NOW coexistence on channel 6, all eight normalized values reaching the OLED, and CAN `missed / overrun / bus = 0 / 0 / 0`.

Validation of v2.0.0 is scoped to the ECU simulator/CANable bench chain, the physically tested ESP-NOW path through the Instrument Node and display, and the accepted Motor Node Web UI regression. Real Mercury ECU/boat validation applies to the archived Motor Node Web v1.0.0 baseline and is not claimed for v2.0.0.

## Release contents

| Path | Contents |
|---|---|
| [`firmware/`](firmware/README.md) | Arduino IDE-ready frozen Motor Node Web v2.0.0 source |

The shared SmartCraft startup behavior is documented in [SmartCraft Session Authorization](../../SMARTCRAFT_SESSION_AUTHORIZATION.md). The previous Motor Node Web v1.0.0 release is retained under [`archive/Motor_Node_Web_v1_0_0/`](../../archive/Motor_Node_Web_v1_0_0/README.md).

## License and safety

Firmware is licensed under `GPL-3.0-or-later`; see the repository [licensing overview](../../LICENSE.md). CraftBridge is experimental, uncertified equipment and must not be the sole source of safety-critical engine information; see the [disclaimer](../../DISCLAIMER.md).