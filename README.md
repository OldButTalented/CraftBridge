# CraftBridge

CraftBridge is an independent, open engineering project for turning selected Mercury SmartCraft engine data into a normalized six-signal model for browser, display and gateway applications.

```text
SmartCraft CAN -> Engine ESP32 -> normalized engine data
                                      |-> Wi-Fi AP / web UI -> phone or browser
                                      `-> ESP-NOW -> display node or NMEA 2000 / Garmin node
```

The Engine Node establishes or reuses the required SmartCraft session, decodes an explicit six-signal allowlist and exposes only normalized values. Planned outputs are a local web display and ESP-NOW transport to either a SmartGauge-style display or a separate NMEA 2000/Garmin node. None of these firmware or output paths is implemented in this repository. SmartCraft and NMEA 2000 remain electrically separate.

## Current status

This repository currently contains the system design, hardware requirements, verified SmartCraft findings and an implementation/test specification. **It does not contain buildable Engine Node or downstream-node firmware, PCB design files or a released wiring harness.** Output-interface implementation has not started.

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
| Web, display and NMEA 2000 / Garmin outputs | Planned architecture only | Not implemented or target-verified |

## Controlled SmartCraft inputs

[SMARTCRAFT_INPUT_CONTRACT.md version 1.1.0](SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source in this repository for concrete SmartCraft CAN IDs, pages, byte fields, encodings, scales and normalized input semantics.

The six contracted inputs are:

| Normalized field | SmartCraft source | Conversion | Public evidence status |
|---|---|---|---|
| `rpm` | `0x170`, page `00`, `D2:D3` u16be | `rpm = raw` | Verified on tested ECU; baseline traffic |
| `coolant_temperature_c` | `0x1A0`, page `07`, `D3` | `°C = raw` | Verified on tested ECU; expanded state |
| `runtime_hours` | `0x1A0`, page `02`, `D4:D5` u16be | `hours = raw / 60` | Verified on tested ECU; expanded state |
| `oil_pressure_ok` | `0x1A0`, page `05`, `D4:D5` u16be | filtered binary switch status | Verified on tested ECU; not analog pressure |
| `battery_voltage_v` | `0x1A0`, page `09`, `D5:D6` u16be | `volts = raw × 0.001` | Verified on tested ECU for CraftBridge use |
| `fuel_flow_lph` | `0x170`, page `01`, `D2:D3` u16be | `l/h = raw × 0.01` | Verified against Connect Mobile reference on tested ECU |

Fuel is not independently verified by a laboratory flow meter. Accumulated fuel is not a separately verified raw ECU input.

## Tested engine and compatibility

The published SmartCraft results were verified on an approximately 2006-model-year Mercury 40 EFI FourStroke, in the ECM-555/PCM-555 family. They are not a universal Mercury specification.

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
