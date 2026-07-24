# CraftBridge

## Purpose

CraftBridge is an experimental two-node ESP32 gateway that transfers selected, verified marine engine values from a passively monitored SmartCraft CAN network to a separate NMEA 2000 network and Garmin Echomap.

The engine-side interface is physically receive-only. CraftBridge does not transmit, acknowledge or inject frames onto the SmartCraft network.

## Scope

CraftBridge includes:

- Passive SmartCraft CAN reception
- Decoding of independently verified engine values
- Normalized telemetry over ESP-NOW
- NMEA 2000 output on a separate CAN network
- Hardware, firmware, wiring and test documentation

CraftBridge does not include:

- CDS/DDT service-port work
- Diagnostic protocol reverse engineering
- Diacom analysis
- Fault clearing, configuration or ECU programming
- Proprietary Mercury software, firmware, databases or documentation

Only SmartCraft mappings classified as **Verified** in [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md) are eligible for gateway output. Candidate, Strong, Weak and Unknown mappings are withheld.

A verified SmartCraft source does not automatically verify the selected NMEA 2000 destination. PGN selection, field scaling and Garmin presentation require independent bench testing.

[`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md), currently version 1.0.0, is the sole authority for concrete SmartCraft input definitions. [`NMEA2000_Mapping.md`](NMEA2000_Mapping.md) is authoritative for destination selection, PGN decisions and Garmin verification status.

## System summary

1. The engine-side ESP32 listens passively to SmartCraft CAN through a physically receive-only SN65HVD230 interface.
2. It decodes an allowlist of verified values and sends normalized, versioned snapshots over ESP-NOW.
3. The helm-side ESP32 validates packet integrity, sequence and freshness.
4. Eligible values are mapped to candidate NMEA 2000 PGNs and transmitted through an MCP2562.
5. Garmin Echomap and the gateway operate on a short, externally powered NMEA 2000 segment with 120-ohm termination at both physical ends.

The master architecture diagram is in [`Architecture_Overview.md`](Architecture_Overview.md).

## Safety principles

- The SmartCraft interface must be physically incapable of transmitting, even if firmware crashes.
- SN65HVD230 `D/TXD` has no ESP32 connection and is hard-wired to the recessive state.
- No additional termination is connected to the SmartCraft bus.
- SmartCraft CAN and NMEA 2000 are separate electrical domains.
- Only normalized values cross the wireless ESP-NOW link.
- Initial testing uses replay or simulation rather than a live engine bus.
- Stale or invalid values are suppressed or marked unavailable, never silently replaced with zero.

See [`Safety_ECU_Protection.md`](Safety_ECU_Protection.md) and [`Test_Plan.md`](Test_Plan.md).

## Documentation

- [`Architecture_Overview.md`](Architecture_Overview.md) — system architecture and trust boundaries
- [`Hardware_Design.md`](Hardware_Design.md) — node hardware and electrical design
- [`Wiring.md`](Wiring.md) — net-level wiring and pre-power checks
- [`Firmware_Architecture.md`](Firmware_Architecture.md) — firmware tasks and failure handling
- [`ESP_NOW_Protocol.md`](ESP_NOW_Protocol.md) — wireless packet format and link behavior
- [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md) — controlled SmartCraft input definitions
- [`NMEA2000_Mapping.md`](NMEA2000_Mapping.md) — NMEA destination, PGN and Garmin verification status
- [`BOM.md`](BOM.md) — bill of materials and component status
- [`Test_Plan.md`](Test_Plan.md) — staged verification and approval gates
- [`Safety_ECU_Protection.md`](Safety_ECU_Protection.md) — ECU and bus protection rules
- [`Decision_Log.md`](Decision_Log.md) — architectural decisions and rationale

## Current status

Private development.

Architecture and initial documentation exist. GPIO assignment, schematic and PCB design, firmware implementation, NMEA 2000 library selection and live installation remain under development.

## Independence and trademarks

CraftBridge is an independent experimental project.

It is not affiliated with, endorsed by, sponsored by or approved by Mercury Marine, Garmin or the National Marine Electronics Association.

SmartCraft, Mercury, Garmin, Echomap and NMEA 2000 are names or trademarks belonging to their respective owners. Their use here is solely descriptive and does not imply certification or official compatibility.
