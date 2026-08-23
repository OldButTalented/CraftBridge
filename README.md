# CraftBridge

CraftBridge is an independent, open engineering project for presenting selected Mercury SmartCraft engine data on Garmin/ECHOMAP displays through NMEA 2000.

```text
SmartCraft CAN -> Engine Node -> ESP-NOW -> Helm Node -> NMEA 2000 -> Garmin/ECHOMAP
```

The intended system uses two ESP32 nodes. The Engine Node interfaces with SmartCraft, decodes an explicit signal allowlist, and sends normalized values wirelessly. The Helm Node validates freshness and emits selected NMEA 2000 PGNs on a separate CAN network. SmartCraft and NMEA 2000 are never electrically bridged.

## Current status

This repository currently contains the system design, hardware requirements, verified SmartCraft findings and an implementation/test specification. **It does not contain buildable Engine Node or Helm Node firmware, PCB design files or a released wiring harness.** Track-3 output implementation has not started.

Protocol readiness and implementation readiness are deliberately separated:

| Capability | Protocol / ECU evidence | Current CraftBridge firmware |
|---|---|---|
| Standalone expanded producer state | Physically verified twice on tested ECU | Not implemented |
| RPM input | Verified mapping available | Not implemented |
| Coolant-temperature input | Verified mapping available | Not implemented |
| Runtime input | Verified mapping available | Not implemented |
| Oil-pressure status input | Verified mapping available | Not implemented |
| Battery/ECU supply-voltage input | Verified mapping available | Not implemented |
| Instantaneous fuel-flow input | Verified against Connect Mobile reference | Not implemented |
| Normalized engine-data model | Contracted | Not implemented |
| ESP-NOW transport | Specified architecture | Not implemented |
| NMEA 2000 / Garmin output | Track 3 not started | Not implemented or target-verified |

## Controlled SmartCraft inputs

[SMARTCRAFT_INPUT_CONTRACT.md version 1.1.0](SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source in this repository for concrete SmartCraft CAN IDs, pages, byte fields, encodings, scales and normalized input semantics.

The six contracted inputs are:

- RPM;
- engine coolant temperature;
- engine runtime;
- oil-pressure status from the tested engine's binary switch;
- ECU supply / battery voltage;
- instantaneous fuel flow.

Oil is status information, not analog kPa. Fuel is verified against synchronized Connect Mobile reference, not a laboratory flow meter. Accumulated fuel is not a separately verified raw ECU input.

## Tested engine and compatibility

The published SmartCraft results were verified on an approximately 2006-model-year Mercury 40 EFI FourStroke in the ECM-555/PCM-555 family. They are not a universal Mercury specification.

The standalone startup and expanded producer state are verified on that ECU. Related engines may differ in identity/profile responses, challenge handling, timing, producer pages or signal layout. Future compatibility contributions must preserve tested-ECU evidence while recording variant-specific results separately.

## Session coexistence

The controlled contract records a **COEXISTENCE FIRST** firmware design decision. CraftBridge should give Connect Mobile the first opportunity to establish the expanded producer state, remain passive when required pages already exist, initialize standalone only after a defined wait/timeout, and recover only after a sustained missing-data timeout. The approximately 5.8-second Connect startup delay is an observation from the tested setup, not a universal protocol constant.

This state machine is **not implemented in the current repository**.

## Hardware overview

The intended prototype requires two ESP32-WROOM boards, appropriate CAN transceivers, protected power supplies, a non-destructive SmartCraft tap, and a correctly powered and terminated NMEA 2000 segment. Component choices, pinout, protection and marine suitability require final engineering before installation.

See:

- [Controlled SmartCraft input contract](SMARTCRAFT_INPUT_CONTRACT.md)
- [Architecture and hardware](docs/architecture-and-hardware.md)
- [SmartCraft protocol findings](docs/smartcraft.md)
- [Implementation, build and test status](docs/build-and-test.md)
- [Contributing](CONTRIBUTING.md)
- [Disclaimer](DISCLAIMER.md)

## Safety boundary

CraftBridge is not a transparent CAN bridge, diagnostic tool, calibration interface or fuzzing platform. An implementation must transmit only the documented SmartCraft startup frames, use response gates, calculate responses from current live challenges, and abort on unexpected directed traffic or timeout. It must not expose arbitrary SmartCraft transmit or diagnostic/control passthrough.

This is experimental, uncertified equipment. Do not use it as the sole source of safety-critical engine information. See [DISCLAIMER.md](DISCLAIMER.md).

## License

Apache License 2.0. See [LICENSE](LICENSE).
