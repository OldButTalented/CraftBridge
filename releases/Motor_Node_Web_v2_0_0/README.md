# CraftBridge Motor Node Web v2.0.0

## Release status

- Firmware: **Physically verified / accepted / frozen**
- Development firmware baseline: `3a37f6e1603241fe8cd43c758aa4d37e80c8508d`
- Scope: standalone Motor Node firmware for the developing CraftBridge 3B/3C system
- Complete CraftBridge 3B/3C product: **Not released**

This release preserves the verified SmartCraft acquisition, session, decoder and freshness behavior and the existing Motor Node Web interface. It adds ESP-NOW transmission of normalized engine values and is prepared for Instrument Node pairing/communication.

The first iteration uses compile-time peer configuration. Dynamic Web-based pairing/configuration is not implemented. Instrument Node ESP-NOW reception and OLED display are physically validated, but Instrument Node NMEA 2000 functionality is still under development. This is not the complete CraftBridge 3B/3C release.

## Architecture and scope

`SmartCraft CAN -> verified session/decoder -> normalized engine data -> Web + ESP-NOW`

ESP-NOW characteristics:

- Protocol version: `1`
- Packet size: `40 bytes`
- Default/configured Wi-Fi channel: `6`
- Transmission interval: `250 ms` (`4 Hz`)
- Peer configuration: compile time

The packet contains normalized RPM, coolant temperature, runtime hours, oil pressure, battery voltage, fuel flow, boot-relative fuel used and boot-relative trip time. It does not bridge raw SmartCraft CAN frames.

## Physical acceptance

The following bench chain passed physically:

`ECU simulator -> SmartCraft CAN/CANable -> Motor Node v2 -> ESP-NOW -> Instrument Node -> OLED`

Observed acceptance included unchanged Motor Node Web behavior, Wi-Fi SoftAP and ESP-NOW coexistence on channel 6, all eight normalized values reaching the OLED, and CAN `missed / overrun / bus = 0 / 0 / 0`.

## Release contents

| Path | Contents |
|---|---|
| [`firmware/`](firmware/README.md) | Arduino IDE-ready frozen Motor Node Web v2.0.0 source |

The shared SmartCraft startup behavior is documented in [SmartCraft Session Authorization](../../SMARTCRAFT_SESSION_AUTHORIZATION.md). The existing released Option 3A material remains under [`releases/3A/`](../3A/README.md).

## License and safety

Firmware is licensed under `GPL-3.0-or-later`; see the repository [licensing overview](../../LICENSE.md). CraftBridge is experimental, uncertified equipment and must not be the sole source of safety-critical engine information; see the [disclaimer](../../DISCLAIMER.md).