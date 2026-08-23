# CraftBridge

CraftBridge is an independent, open engineering project for presenting selected Mercury SmartCraft engine data on Garmin/ECHOMAP displays through NMEA 2000.

```text
SmartCraft CAN -> Engine Node -> ESP-NOW -> Helm Node -> NMEA 2000 -> Garmin/ECHOMAP
```

The intended system uses two ESP32 nodes. The Engine Node interfaces with SmartCraft, decodes an explicit signal allowlist, and sends normalized values wirelessly. The Helm Node validates freshness and emits selected NMEA 2000 PGNs on a separate CAN network. SmartCraft and NMEA 2000 are never electrically bridged.

## Current status

This repository currently contains the system design, hardware requirements, verified SmartCraft findings, and an implementation/test specification. **It does not yet contain buildable Engine Node or Helm Node firmware, PCB design files, or a released wiring harness.**

Protocol behavior and firmware capability are deliberately separated:

| Capability | Protocol / ECU evidence | Current CraftBridge firmware |
|---|---|---|
| Read fresh ECU producer pages | Physically observed | Not implemented in this repository |
| Standalone session establishment | Physically verified twice on the tested ECU | Not implemented in this repository |
| Live challenge responses | Exact capture match and physically accepted | Not implemented in this repository |
| RPM decoding | Verified | Not implemented in this repository |
| Runtime and coolant decoding after startup | Verified | Not implemented in this repository |
| ESP-NOW transport | Specified architecture | Not implemented in this repository |
| NMEA 2000 output to Garmin/ECHOMAP | Candidate mapping and test plan | Not implemented or target-verified |

## Tested engine and compatibility

The published SmartCraft results were physically verified on a 2006-generation Mercury 40 EFI FourStroke using the ECM-555/PCM-555 family. They are not a universal Mercury specification.

The standalone 30-transmission startup, its three live response transforms, and the expanded producer state are verified on that ECU. Related engines may differ in identity/profile responses, challenge handling, timing, producer pages, or signal layout. Test with live challenges from the current ECU session; never replay historical response bytes as constants.

## Verified SmartCraft data

| Signal | Source | Encoding |
|---|---|---|
| Engine speed | `0x170`, page `00`, `D2:D3` | big-endian unsigned 16-bit, 1 RPM/bit |
| Engine runtime | `0x1A0`, page `02`, `D4:D5` | big-endian unsigned 16-bit minutes; hours = raw / 60 |
| Coolant temperature | `0x1A0`, page `07`, `D3` | 1 °C/bit |

The tested engine uses a binary oil-pressure switch. A CAN candidate exists, but the physical byte/bit mapping is not verified. Fuel mapping remains unresolved.

## Hardware overview

The intended prototype requires two ESP32-WROOM boards, appropriate CAN transceivers, protected power supplies, a non-destructive SmartCraft tap, and a correctly powered and terminated NMEA 2000 segment. Component choices, pinout, protection, and marine suitability require final engineering before installation.

See:

- [Architecture and hardware](docs/architecture-and-hardware.md)
- [SmartCraft protocol and signal findings](docs/smartcraft.md)
- [Implementation, build, and test status](docs/build-and-test.md)
- [Contributing](CONTRIBUTING.md)
- [Disclaimer](DISCLAIMER.md)

## Safety boundary

CraftBridge is not a transparent CAN bridge, diagnostic tool, calibration interface, or fuzzing platform. An implementation must transmit only the documented SmartCraft startup frames, use response gates, calculate responses from the current live challenges, and abort on unexpected directed traffic or timeout. It must not expose arbitrary SmartCraft transmit or diagnostic/control passthrough.

This is experimental, uncertified equipment. Do not use it as the sole source of safety-critical engine information. See [DISCLAIMER.md](DISCLAIMER.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
