# Decision Log

## Status vocabulary

- **Accepted** — current architectural rule.
- **Candidate** — selected for testing but not validated.
- **TBD** — decision requires evidence or implementation review.
- **Rejected** — must not be used in the current architecture.

## Decisions

| ID | Date | Status | Decision | Rationale / consequence |
|---|---|---|---|---|
| GW-001 | 2026-07-19 | Accepted | Create standalone `CraftBridge` repository | Keeps the gateway implementation self-contained and separate from unrelated diagnostic and protocol work |
| GW-002 | 2026-07-19 | Accepted | Use two ESP32 nodes with ESP-NOW between engine and helm | Avoids a wired CAN bridge and preserves electrical-domain separation |
| GW-003 | 2026-07-19 | Accepted | Engine-side SmartCraft interface is physically RX-only | Firmware failure must not enable SmartCraft transmission |
| GW-004 | 2026-07-19 | Accepted | SN65HVD230 D/TXD is hard-wired HIGH and has no ESP32 route | HIGH commands recessive state; no firmware-controllable driver input exists |
| GW-005 | 2026-07-19 | Rejected | Add 120 ohm termination at the engine node | Existing SmartCraft bus termination must not be changed by the passive tap |
| GW-006 | 2026-07-19 | Accepted | Send normalized values, not raw SmartCraft frames, over ESP-NOW | Helm node does not become a second SmartCraft decoder and protocol coupling is reduced |
| GW-007 | 2026-07-19 | Accepted | Version 1 wireless packet is fixed-width, sequenced, timestamped and CRC-protected | Supports deterministic parsing, reset detection, loss accounting and stale handling |
| GW-008 | 2026-07-19 | Accepted | Stale values are unavailable/suppressed, never silently zero | Zero can be a valid engine value and would hide a failed link |
| GW-009 | 2026-07-19 | Accepted | Use a dedicated two-device prototype NMEA 2000 segment | No traditional backbone is installed; testing remains isolated from SmartCraft |
| GW-010 | 2026-07-19 | Accepted | Exactly two 120 ohm terminators, one at each physical end | Required linear CAN termination; expected combined resistance approximately 60 ohms |
| GW-011 | 2026-07-19 | Accepted | NMEA segment receives external fused power on NET-S/NET-C | Garmin is not treated as the network power source |
| GW-012 | 2026-07-19 | Accepted | Helm transceiver is MCP2562 with VDD 5 V and VIO 3.3 V | Matches the selected transceiver's split supply/logic interface |
| GW-013 | 2026-07-19 | Accepted | Production NMEA output consumes only SmartCraft mappings documented as Verified in this repository | Preserves project evidence methodology |
| GW-014 | 2026-07-19 | Candidate | RPM -> PGN 127488; temperature/hours -> PGN 127489 | Plausible NMEA destinations; exact fields, rates and Garmin presentation require bench verification |
| GW-015 | 2026-07-19 | Rejected | Emit oil pressure, fuel rate/load or battery voltage now | Concrete mappings do not meet the Verified gateway-input threshold |
| GW-016 | 2026-07-19 | Accepted | Replay/simulation and Garmin bench proof precede live end-to-end output | Limits ECU and operational risk |
| GW-017 | 2026-07-19 | TBD | Exact ESP32 boards, GPIOs and PCB implementation | Requires schematic/layout work |
| GW-018 | 2026-07-19 | TBD | ESP-NOW channel, rate, encryption keys, retry and stale thresholds | Requires RF survey and fault testing |
| GW-019 | 2026-07-19 | TBD | NMEA library, identity values, engine instance, PGN periods and address strategy | Requires implementation and Garmin compatibility testing |
| GW-020 | 2026-07-19 | TBD | Fuse ratings, buck converters, TVS/protection and enclosure | Requires measured loads and marine electrical review |
| GW-021 | 2026-07-24 | Accepted | `SMARTCRAFT_INPUT_CONTRACT.md` is the sole authority for concrete SmartCraft input definitions | Establishes one controlled WP7-to-CraftBridge interface and prevents competing CAN or byte definitions |

## Decision-change rule

Any change that enables SmartCraft transmission, adds SmartCraft termination, bridges CAN conductors, emits a non-Verified mapping or bypasses a test gate requires an explicit new decision entry and safety review. It must not be introduced as an incidental firmware or wiring change.

