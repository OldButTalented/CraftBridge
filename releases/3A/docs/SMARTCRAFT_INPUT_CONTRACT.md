# SmartCraft Input Contract

## Document control

- Input-contract version: **1.3.0**
- Revision date: **2026-08-31**
- Status: **Controlled**
- Evidence authority: **verified SmartCraft evidence baseline for the tested ECU**
- Evidence baseline: **six-signal mapping and standalone-session baseline approved 2026-08-23, plus controlled numeric oil-pressure handoff approved 2026-08-31**
- Evidence revision date: **2026-08-31**
- Upstream WP7 handoff: **CRAFTBRIDGE_OIL_NUMERIC_HANDOFF.md, revision 2026-08-31**
- Evidence integration: **reviewed and approved for public CraftBridge use**
- Downstream consumer: **CraftBridge**

## Purpose and authority

This document is the sole controlled CraftBridge interface to the verified SmartCraft findings for the tested ECU.

It defines protocol inputs that CraftBridge is allowed to implement. A mapping listed as Verified here is not a claim that current CraftBridge firmware implements it. Supporting captures, screenshots, service information and analysis remain outside this repository.

## Responsibility boundary

The verified SmartCraft evidence baseline controls CAN ID/page/field identification, encoding, scaling, semantic interpretation and evidence promotion or withdrawal.

CraftBridge owns decoder implementation, validity and freshness behavior, normalized internal data and Web UI presentation.

A Verified SmartCraft input does not by itself verify Web UI presentation. Firmware and interface behavior require their own implementation tests.

## Evidence status

- **Verified** — the mapping and stated interpretation are established by reproducible evidence on the tested ECU.
- **Strong** — substantial evidence exists but a requirement for Verified remains unresolved.
- **Candidate** — plausible and selected for further testing.
- **Weak** — limited or ambiguous evidence with competing interpretations.
- **Unknown** — a required property has not been established.

Only mappings explicitly listed as Verified in this contract may enter a production decoder allowlist. Strong, Candidate, Weak and Unknown mappings must not be emitted as valid gateway inputs. An Unknown/TBD property attached to a Verified mapping must remain unresolved rather than being replaced by an assumption.

## Tested scope

The mappings are verified on the tested approximately 2006-model-year Mercury 40 EFI FourStroke, in the ECM-555 / PCM-555 family. Universal compatibility with other Mercury engines, ECU software or SmartCraft variants is not claimed.

The notation `D1` through `D8` is one-based payload-byte numbering.

## Contracted SmartCraft inputs

### 1. Engine speed

- Normalized field: `rpm`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU**
- CAN ID: `0x170`
- Page selector: `D1 = 0x00`
- Data field: `D2:D3`
- Encoding: unsigned 16-bit, big-endian
- Resolution: 1 RPM/bit
- Conversion: `rpm = raw`
- Standalone availability: **AVAILABLE**; present in baseline ECU traffic and S3

Decode: `raw = (D2 << 8) | D3`.

### 2. Engine coolant temperature

- Normalized field: `coolant_temperature_c`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x07`
- Data field: `D3`
- Encoding: unsigned single byte
- Resolution: 1 °C/bit
- Conversion: `coolant_temperature_c = raw`
- Standalone availability: **AVAILABLE** in expanded S3

Byte order is not applicable to this single-byte field.

### 3. Engine runtime

- Normalized field: `runtime_hours`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x02`
- Data field: `D4:D5`
- Encoding: unsigned 16-bit, big-endian
- Raw unit: minutes
- Conversion: `runtime_hours = raw / 60.0`
- Standalone availability: **AVAILABLE** in expanded S3

Decode: `raw = (D4 << 8) | D5`.

A runtime mirror exists in Connect-expanded traffic on `0x1E0`, page `0x00`. CraftBridge does not require or use that mirror as a contracted source. Primary-source ownership of the mirror remains Unknown/TBD.

### 4. ECU-reported oil pressure

- Normalized field: `oil_pressure_kpa`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x05`
- Data field: `D4:D5`
- Raw datatype: unsigned 16-bit
- Byte order: big-endian
- Scale: `0.01 kPa/bit`
- Offset: `0 kPa`
- Unit: `kPa`
- Conversion: `oil_pressure_kpa = raw * 0.01`
- Evidence status: **VERIFIED ON TESTED ECU**
- Standalone availability: **AVAILABLE** in expanded S3

Decode: `raw = (D4 << 8) | D5`.

Range and validity boundary:

- Observed/tested raw range: `0x0000` through `0x9BA7`, corresponding to 0.00 through 398.47 kPa.
- The observed range is evidence coverage only; it is not a protocol-global valid range and must not be used as a decoder rejection boundary.
- Protocol-global valid range: **Unknown**.
- No invalid or sentinel raw value is documented.
- `0xFFFF` was not observed; this absence does not establish sentinel or validity semantics.
- `0x0000`, `0x0001`, `0x9B82`, `0x9B8B`, `0x9B94` and `0x9BA7` are observed numeric values, not sentinels.

Normative rule: every fresh, well-formed contracted field is decoded as unsigned big-endian and scaled by 0.01 kPa/bit. CraftBridge must not retain the former CLOSED/OPEN whitelist as the numeric decoder and must not reject values solely because they fall outside the observed range. No sentinel or protocol-global range may be invented.

Physical/evidence limitation: the tested engine has a binary oil-pressure threshold switch. The ECU emits a filtered or substituted numeric value derived from that switch state. The contracted value is therefore **ECU-reported oil pressure**, not measured analog hydraulic pressure. Exact ECU filtering, protocol-global range, sentinel semantics, cross-engine portability and physical pressure accuracy remain Unknown.

The previous normalized boolean `oil_pressure_ok` is superseded as a contracted input. Option 3A implements the version 1.3.0 numeric `oil_pressure_kpa` field in runtime, normalized data and Web output.

### 5. ECU supply / battery voltage

- Normalized field: `battery_voltage_v`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU FOR CRAFTBRIDGE USE**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x09`
- Data field: `D5:D6`
- Encoding: unsigned 16-bit, big-endian
- Conversion: `battery_voltage_v = raw * 0.001`
- Practical useful resolution: approximately 0.1 V
- Semantic value: `ECU_SUPPLY_VOLTAGE / BATTERY_VOLTAGE`
- Standalone availability: **AVAILABLE** in expanded S3

Decode: `raw = (D5 << 8) | D6`.

The mapping is supported by combined Connect Mobile correlation and independent battery-terminal DMM measurement. It does not establish that the ECU measures directly at the battery terminals. Small differences between ECU-reported supply voltage and a battery-terminal DMM are expected. Intended use is charging/not-charging indication, approximate charging voltage and detection of obviously low charging voltage.

### 6. Instantaneous fuel flow

- Normalized field: `fuel_flow_lph`
- Mapping status: **CAN MAPPING VERIFIED AGAINST CONNECT MOBILE REFERENCE ON TESTED ECU**
- CAN ID: `0x170`
- Page selector: `D1 = 0x01`
- Data field: `D2:D3`
- Encoding: unsigned 16-bit, big-endian
- Scale: 0.01
- Offset: 0
- Unit: litres/hour
- Conversion: `fuel_flow_lph = raw * 0.01`
- Standalone availability: **AVAILABLE** in expanded S3

Decode: `raw = (D2 << 8) | D3`.

Evidence boundary: verification used synchronized Connect Mobile display and SavvyCAN references, not an independent laboratory fuel-flow meter. The analysis explained 35/36 usable references, 8/8 repeated 0.4↔0.5 l/h events and 4/4 large excursions, with MAE approximately 0.034 l/h, maximum observed reference error approximately 0.130 l/h and Connect Mobile display lag median 0.806 s (range 0.392–0.856 s).

The signal is not a simple RPM-derived lookup. The ECU's internal fuel-rate algorithm remains Unknown. Accumulated/consolidated consumption is not a separately verified raw ECU signal. A later output/application layer may integrate fresh instantaneous flow, but that derived value is outside this input contract.

## Normalized input model

| Field | Type / unit | Validity requirement |
|---|---|---|
| `rpm` | non-negative RPM | fresh recognized `0x170/page 00` field |
| `coolant_temperature_c` | degrees Celsius | fresh recognized `0x1A0/page 07` field |
| `runtime_hours` | hours | fresh recognized `0x1A0/page 02` field |
| `oil_pressure_kpa` | kPa | fresh recognized `0x1A0/page 05` u16be field; no invented range or sentinel rejection |
| `battery_voltage_v` | volts | fresh recognized `0x1A0/page 09` field |
| `fuel_flow_lph` | litres/hour | fresh recognized `0x170/page 01` field |

Missing, malformed, stale, unrecognized or unavailable input must be marked invalid; it must not silently become zero.

## Protocol readiness versus implementation readiness

| Signal | Verified mapping available | Decoder implemented | Normalized field implemented | Web output implemented |
|---|---|---|---|---|
| RPM | Yes | Yes | Yes | Yes |
| Coolant temperature | Yes | Yes | Yes | Yes |
| Runtime | Yes | Yes | Yes | Yes |
| ECU-reported oil pressure | Yes | Yes | Yes | Yes |
| Battery voltage | Yes | Yes | Yes | Yes |
| Instantaneous fuel flow | Yes | Yes | Yes | Yes |

Implementation audit: all six mappings, including version 1.3.0 numeric ECU-reported oil pressure, are implemented and host-tested in Option 3A runtime, normalized data and Web output.

## Producer and session requirements

The verified standalone expanded S3 producer state includes:

- `0x170`: pages `00 01 02 03 04 05 06 FF`;
- `0x1A0`: pages `00 01 02 03 04 05 06 07 08 09 0A 0B 0C FF`.

RPM is available in baseline ECU traffic. The other five contract inputs depend on their expanded producer pages. A dependent value is valid only after its required page is present and fresh.

The verified standalone session mechanism can activate S3 without Connect Mobile. CraftBridge does not require Connect-only producer families `0x1E0` or `0x1F0` for the six contract inputs.

- Full-bus equivalence: **NOT REQUIRED / NOT PRESENT**.
- Target-data coverage: **SUFFICIENT FOR ALL SIX CONTRACT SIGNALS ON TESTED ECU**.

## Session coexistence requirement

Status: **IMPLEMENTED AND HOST-TESTED**.

Policy: **COEXISTENCE FIRST**.

1. After IGN ON, wait before initiating a standalone SmartCraft session.
2. Give Connect Mobile the first opportunity to establish the session.
3. Use the approximately 5.8-second Connect startup delay observed on the tested setup only as a design reference, not as a universal Mercury protocol constant.
4. During the wait, monitor for the required expanded producer pages.
5. If an existing Connect-established session provides the required pages, remain passive and do not transmit the standalone initialization.
6. If no working expanded producer state appears after the defined startup timeout, the firmware may execute the verified standalone initialization.
7. During operation, monitor freshness of required producer data. One missed frame must not trigger recovery.
8. If required expanded pages remain absent beyond the defined timeout, enter terminal session loss, disable application TX and require reset or a new ignition lifecycle. Do not replay startup automatically.
9. After IGN OFF or ECU session reset, require a new session establishment. Expanded pages disappearing after an observed IGN/session reset during voltage testing directly support this requirement.

The implementation uses an 8.0 s fresh S0 observation, requires the complete fresh S3 producer set and enters terminal failure/session-loss states without automatic replay. These are CraftBridge implementation rules covered by host tests, not Mercury protocol constants.

## Revision history

- **1.3.0 — 2026-08-31:** authorized numeric ECU-reported oil pressure at `0x1A0/page 0x05/D4:D5`, u16be, raw × 0.01 kPa; superseded the binary-only contracted decoder while preserving the tested-engine switch limitation.
- **1.2.0 — 2026-08-25:** prior controlled six-signal baseline; its binary-only oil-pressure definition is superseded by version 1.3.0.

## Change control and repository restrictions

Any technical addition, removal or modification requires new verified SmartCraft evidence, complete mapping review, contract-version increment and dated revision record. CraftBridge implementation evidence alone cannot promote a mapping.

This contract must not contain raw CAN logs, Connect Mobile images, links to private evidence trees, unrelated diagnostic research, proprietary software/data, personal paths, engine serial numbers, credentials, private configuration or other unique private identifiers.
