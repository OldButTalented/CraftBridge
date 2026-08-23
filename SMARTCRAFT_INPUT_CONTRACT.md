# SmartCraft Input Contract

## Document control

- Input-contract version: **1.1.0**
- Previous version: **1.0.0**
- Revision date: **2026-08-23**
- Status: **Controlled**
- Upstream evidence authority: **Mercury ECM-555 SmartCraft WP7 analysis**
- Upstream evidence reference: **TRACK2_FINAL_BASELINE.md, TRACK2_FINAL_SIGNAL_MAP.csv and TRACK2_FINAL_EVIDENCE_MATRIX.csv**
- Upstream evidence revision date: **2026-08-23**
- Upstream integration-validation SHA-256: `01705EA32C733324991992D0306D9A777CA9CAE989997373B402CD7B0E9F4BD9`
- Downstream consumer: **CraftBridge**

## Purpose and authority

This document is the sole controlled interface between the verified SmartCraft findings maintained by the Mercury ECM-555 WP7 analysis and CraftBridge.

It defines protocol inputs that CraftBridge is allowed to implement. A mapping listed as Verified here is not a claim that current CraftBridge firmware implements it. Supporting captures, screenshots, service information and analysis remain outside this repository.

## Responsibility boundary

The Mercury WP7 evidence baseline owns CAN ID/page/field identification, encoding, scaling, semantic interpretation and evidence promotion or withdrawal.

CraftBridge owns decoder implementation, validity and freshness behavior, normalized internal data, ESP-NOW transport, output selection, NMEA 2000/Garmin verification and gateway tests.

A Verified SmartCraft input does not verify any NMEA 2000 PGN, output field, conversion, transmission rate or Garmin presentation. Track 3 output-interface work is not started by this contract revision.

## Evidence status

- **Verified** — the mapping and the stated interpretation are established by the referenced WP7 evidence on the tested ECU.
- **Strong** — substantial evidence exists but a requirement for Verified remains unresolved.
- **Candidate** — plausible and selected for further testing.
- **Weak** — limited or ambiguous evidence with competing interpretations.
- **Unknown** — a required property has not been established.

Only mappings explicitly listed as Verified in this contract may enter a production decoder allowlist. Strong, Candidate, Weak and Unknown mappings must not be emitted as valid gateway inputs. An Unknown/TBD property attached to a Verified mapping must remain unresolved rather than being replaced by an assumption.

## Tested scope

The mappings are verified on the tested approximately 2006-model-year Mercury 40 EFI FourStroke in the ECM-555 / PCM-555 family. Universal compatibility with other Mercury engines, ECU software or SmartCraft variants is not claimed.

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

### 4. Oil-pressure status

- Normalized field: `oil_pressure_ok`
- Mapping status: **CAN MAPPING VERIFIED ON TESTED ECU**
- CAN ID: `0x1A0`
- Page selector: `D1 = 0x05`
- Data field: `D4:D5`
- Encoding: unsigned 16-bit, big-endian
- Physical source on tested engine: binary oil-pressure switch
- Semantic value: `OIL_PRESSURE_STATUS`
- Encoding classification: `FILTERED_BINARY_REPRESENTATION`
- Standalone availability: **AVAILABLE** in expanded S3

Verified physical behavior on the tested engine:

- pressure below approximately 20 kPa: switch CLOSED / continuity to GND;
- pressure above approximately 20 kPa: switch OPEN / no continuity.

Observed raw regimes:

- CLOSED: `0x0000` initially and `0x0001` on reproducible returns;
- stable OPEN: `0x9B82`;
- observed transition/filter values: `0x9B78`, `0x9B5C`, `0x99F4`.

Normative rule: do not decode this field as analog pressure and do not emit fictitious kPa. `oil_pressure_ok = false` is established for the stable CLOSED regime and `oil_pressure_ok = true` for the stable OPEN regime. During transition/filter values, an implementation must keep the normalized value invalid until a stable recognized regime is present, unless a later verified contract revision defines another rule.

The exact internal ECU filter/conversion algorithm is Unknown and is not required for this contract.

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
| `oil_pressure_ok` | boolean plus validity | fresh stable recognized CLOSED or OPEN regime |
| `battery_voltage_v` | volts | fresh recognized `0x1A0/page 09` field |
| `fuel_flow_lph` | litres/hour | fresh recognized `0x170/page 01` field |

Missing, malformed, stale, unrecognized or unavailable input must be marked invalid; it must not silently become zero.

## Protocol readiness versus implementation readiness

| Signal | Verified mapping available | Decoder implemented | Normalized field implemented | ESP-NOW transport implemented | Downstream NMEA/output implemented |
|---|---|---|---|---|---|
| RPM | Yes | No | No | No | No |
| Coolant temperature | Yes | No | No | No | No |
| Runtime | Yes | No | No | No | No |
| Oil-pressure status | Yes | No | No | No | No |
| Battery voltage | Yes | No | No | No | No |
| Instantaneous fuel flow | Yes | No | No | No | No |

Audit basis: the current repository contains no firmware source or build project. This table records repository state at revision 1.1.0; it is not an implementation plan or Track-3 start.

## Producer and session requirements

The verified standalone expanded S3 producer state includes:

- `0x170`: pages `00 01 02 03 04 05 06 FF`;
- `0x1A0`: pages `00 01 02 03 04 05 06 07 08 09 0A 0B 0C FF`.

RPM is available in baseline ECU traffic. The other five contract inputs depend on their expanded producer pages. A dependent value is valid only after its required page is present and fresh.

The verified standalone session mechanism can activate S3 without Connect Mobile. CraftBridge does not require Connect-only producer families `0x1E0` or `0x1F0` for the six contract inputs.

- Full-bus equivalence: **NOT REQUIRED / NOT PRESENT**.
- Target-data coverage: **SUFFICIENT FOR ALL SIX CONTRACT SIGNALS ON TESTED ECU**.

## Session coexistence requirement

Status: **DESIGN DECISION — NOT IMPLEMENTED IN CURRENT REPOSITORY**.

Policy: **COEXISTENCE FIRST**.

1. After IGN ON, wait before initiating a standalone SmartCraft session.
2. Give Connect Mobile the first opportunity to establish the session.
3. Use the approximately 5.8-second Connect startup delay observed on the tested setup only as a design reference, not as a universal Mercury protocol constant.
4. During the wait, monitor for the required expanded producer pages.
5. If an existing Connect-established session provides the required pages, remain passive and do not transmit the standalone initialization.
6. If no working expanded producer state appears after a defined startup timeout, the later firmware may execute the verified standalone initialization.
7. During operation, monitor freshness of required producer data. One missed frame must not trigger recovery.
8. If required expanded pages remain absent beyond a defined timeout, attempt a controlled session re-establishment.
9. After IGN OFF or ECU session reset, require a new session establishment.

Exact startup and recovery timeout values remain implementation parameters requiring tests. This revision documents the requirement but implements no state machine.

## Non-normative architectural interpretation

**PLAUSIBLE ARCHITECTURAL INTERPRETATION — NOT VERIFIED PROTOCOL FACT.**

A useful model treats addressed 29-bit traffic as a management/session/control plane, `0x170` and `0x1A0` as the primary CraftBridge engine-data producer families, `0x673` as auxiliary presence/session-related traffic, and `0x1E0`/`0x1F0` as extra, alternative, legacy or capability-specific producer families seen in Connect-expanded operation. Connect Mobile may enable a broader generic profile to support multiple engine families and app capabilities. These interpretations are non-normative and do not change decoder requirements.

## Non-normative fuel interpretation

**PLAUSIBLE INTERPRETATION — NOT VERIFIED IMPLEMENTATION DETAIL.**

The ECU may derive instantaneous fuel rate from injection-control information such as commanded/effective injector quantity or injector open-time plus fuel-system calibration. Connect Mobile may integrate instantaneous flow to present accumulated consumption. The actual ECU or app algorithm has not been reverse engineered.

## Change control and repository restrictions

Any technical addition, removal or modification requires a new Verified WP7 conclusion, complete mapping review, contract-version increment and dated revision record. CraftBridge implementation evidence alone cannot promote a mapping.

This contract must not contain raw CAN logs, Connect Mobile images, relative links to the Mercury repository, CDS/DDT or Diacom content, proprietary software/data, service-manual content, personal paths, serial numbers, credentials or private configuration.

## Revision record

### Version 1.1.0 — 2026-08-23

- Previous version: 1.0.0.
- Source: completed six-signal Track-2 baseline, revision 2026-08-23.
- Added verified oil-pressure status, battery/ECU supply voltage and instantaneous fuel-flow mappings.
- Made `0x1A0/page 02` the sole required runtime input; removed the `0x1E0` mirror from required sources.
- Added normalized six-field model, S3 target-data requirement and coexistence-first session design decision.
- Added an explicit audit separating protocol readiness from current firmware/transport/output implementation.

| Version | Date | Change |
|---|---|---|
| 1.0.0 | 2026-07-24 | Initial controlled contract with RPM, coolant-temperature and runtime mappings |
| 1.1.0 | 2026-08-23 | Six-signal Track-2 baseline, S3/session requirements and implementation-status audit |
