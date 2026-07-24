# SmartCraft Input Contract

## Document control

- Input-contract version: **1.0.0**
- Revision date: **2026-07-24**
- Status: **Controlled**
- Upstream evidence authority: **Mercury ECM-555 SmartCraft WP7 analysis**
- Upstream evidence reference: **WP7\_FINDINGS.md and associated verified mapping baseline**
- Upstream evidence revision date: **2026-07-24**
- Upstream evidence SHA-256: `2ECE7B8DD9F57AFC05464EDA583BFADAF9295AE11866F9B1004468C6B468C614`
- Downstream consumer: **CraftBridge**

## Purpose

This document is the sole controlled interface between the verified SmartCraft findings maintained by the Mercury ECM-555 WP7 analysis and the CraftBridge gateway.

CraftBridge may implement only the SmartCraft input mappings explicitly listed as Verified in this contract.

Supporting analysis, raw captures and reference material remain outside the CraftBridge repository.

## Scope and responsibility

### WP7 responsibility

The Mercury ECM-555 WP7 analysis owns:

- SmartCraft CAN capture and correlation
- CAN ID and page identification
- Byte and field identification
- Byte-order determination
- Scaling and raw-unit determination
- Evidence classification
- Promotion of a mapping to Verified
- Revision or withdrawal of a previously Verified mapping

### CraftBridge responsibility

CraftBridge owns:

- Passive reception of the contracted SmartCraft frames
- Implementation of the decoder allowlist
- Freshness and plausibility handling
- Normalization of decoded values
- ESP-NOW transport
- Selection and verification of NMEA 2000 destinations
- Gateway-specific tests and safety behavior

A Verified SmartCraft input does not verify an NMEA 2000 PGN, field, scaling, update rate or Garmin presentation. Those decisions require separate CraftBridge verification.

## Evidence status

- **Verified** — the CAN ID, page, data field, interpretation and required encoding details have been established by reproducible WP7 evidence.
- **Strong** — substantial supporting evidence exists, but one or more requirements for Verified status remain unresolved.
- **Candidate** — a plausible mapping selected for further testing.
- **Weak** — limited or ambiguous evidence exists and competing interpretations remain.
- **Unknown** — the mapping or a required property has not been established.

Only mappings classified as **Verified** in this contract may enter the CraftBridge production decoder allowlist.

Candidate, Strong, Weak and Unknown mappings must not be decoded or emitted as valid production gateway data.

An Unknown or TBD property attached to a Verified mapping must remain explicitly unresolved and must not be replaced by an implementation assumption.

## Contracted SmartCraft inputs

The notation `D1` through `D8` refers to the one-based data-byte positions in the SmartCraft CAN payload.

### 1. Engine speed

- Signal: **RPM**
- Mapping status: **Verified**
- CAN ID: `0x170`
- Page selector: `D1 = 0x00`
- Data field: `D2:D3`
- Byte order: **Big-endian**
- Scaling: **1 RPM/bit**
- CraftBridge normalized value: RPM

Decode:

`rpm = (D2 << 8) | D3`

### 2. Engine temperature

- Signal: **Engine temperature**
- Mapping status: **Verified**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x07`
- Data field: `D3`
- Scaling: **1 °C/bit**
- CraftBridge normalized value: degrees Celsius

Decode:

`engine_temperature_c = D3`

Byte order is not applicable to this single-byte field.

### 3. Engine runtime

- Signal: **Engine runtime**
- Mapping status: **Verified**
- Raw unit: **Minutes**
- CraftBridge normalized value: runtime minutes

#### Source A

- CAN ID: `0x1A0`
- Page selector: `D1 = 0x02`
- Data field: `D4:D5`
- Byte order: **Big-endian**

Decode:

`runtime_minutes_a = (D4 << 8) | D5`

#### Source B

- CAN ID: `0x1E0`
- Page selector: `D1 = 0x00`
- Data field: `D4:D5`
- Byte order: **Big-endian**

Decode:

`runtime_minutes_b = (D4 << 8) | D5`

#### Ownership limitation

The mappings and minute interpretation of Source A and Source B are Verified.

Primary-source ownership is **Unknown/TBD**. CraftBridge must not claim that either source is the authoritative originating node without additional verified evidence.

CraftBridge may decode both contracted representations, but primary-source selection, consistency handling and failover behavior require an explicit CraftBridge implementation decision and tests.

## Excluded inputs

No other SmartCraft mappings are authorized by this contract.

This includes mappings currently classified as:

- Candidate
- Strong
- Weak
- Unknown

Oil pressure, fuel rate, fueling, engine load, battery voltage, throttle, gear and IAC information remain excluded unless a later contract revision explicitly includes a Verified mapping.

A Verified observation that an application displays a value does not by itself authorize a concrete CAN mapping.

## Change control

Any technical addition, removal or modification to a contracted SmartCraft mapping requires:

1. A new Verified WP7 conclusion.
2. Review of the complete CAN ID, page, field, byte order, scaling, unit and interpretation.
3. An incremented input-contract version.
4. A dated revision entry.
5. Corresponding CraftBridge decoder and test updates.

CraftBridge implementation evidence alone must not promote a WP7 Candidate, Strong, Weak or Unknown mapping to Verified.

Editorial corrections that do not alter technical meaning must not silently change a mapping.

## Repository-content restrictions

This contract must not contain:

- Raw CAN logs
- Mercury Connect Mobile reference images
- Relative links to the Mercury repository
- CDS/DDT content
- Diacom content
- Proprietary software, firmware or databases
- Copyrighted service-manual content
- Personal filesystem paths
- Engine or device serial numbers
- Credentials or private configuration

## Revision history

| Version | Date | Change |
|---|---|---|
| 1.0.0 | 2026-07-24 | Initial controlled contract containing the Verified RPM, engine-temperature and engine-runtime mappings |
