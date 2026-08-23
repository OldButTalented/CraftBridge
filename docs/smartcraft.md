# SmartCraft protocol and signal findings

This document is the public technical baseline needed to implement or adapt CraftBridge. It describes independently observed behavior on one tested installation; it is not an official Mercury specification.

[SMARTCRAFT_INPUT_CONTRACT.md version 1.1.0](../SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source for concrete SmartCraft input mappings and normalized semantics. This document provides protocol/session context and must not be used as a competing signal-definition source.

## Evidence scope

- **Tested engine:** approximately model-year 2006 Mercury 40 EFI FourStroke
- **ECU family:** ECM-555 / PCM-555
- **CAN bitrate:** 250 kbit/s
- **Physical status:** standalone startup completed twice without Connect Mobile
- **Portability:** not yet demonstrated on another Mercury engine

“Verified” means physically reproduced or repeatedly confirmed on this ECU. “Candidate” means plausible but not physically resolved. Temporal proximity alone is not treated as causality.

## Fresh ECU-only baseline

Before session establishment, the observed producer pages are:

| CAN ID | Pages |
|---|---|
| `0x170` | `00 03 06 FF` |
| `0x1A0` | `01 FF` |

RPM is available in this autonomous baseline. The other five contracted inputs require the expanded producer state.

## Directed session traffic

The verified directed pair used during standalone establishment is:

- client-like direction: `0x00000B73`;
- ECU-response direction: `0x0000730B`.

The low-byte values `0x73` and `0x0B` behave strongly like endpoint identifiers in the tested interactions. Their exact formal addressing semantics are not established, and this is not presented as a standard J1939 session protocol.

## Standalone startup result

A Python + CANable implementation reproduced the required startup without Connect Mobile:

- 30 client-side CAN transmissions;
- 27 fixed transmissions;
- 3 responses calculated from live ECU challenges;
- 27 response gates;
- 0 automatic retry paths;
- repeated captured startup structures with consistent ordering;
- physically repeated successful standalone runs on the tested ECU.

This verified ECU behavior is **not implemented in firmware currently published in this repository**.

### Live challenge transforms

Interpret the four-byte challenge `E` as an unsigned big-endian integer. Calculate modulo `2^32`, serialize `R` big-endian, and prepend the response opcode shown by the sequence.

```text
R = low32(E * multiplier) XOR constant
```

| Request | Response payload | Multiplier | XOR constant |
|---|---|---:|---:|
| `0x00000B73 : FA 04 01` | `F9 <R32>` | `0xD379A9C8` | `0x1B4610CA` |
| `0x00000B73 : FA 02 06` | `F9 <R32>` | `0xCF88B813` | `0x4353E4D3` |
| `0x00000B73 : 80 04` | `81 <R32>` | `0xAB20FA1B` | `0x208FB01A` |

Use the live challenge from the current session. Historical response bytes must not be replayed as constants. The transforms reproduce all relevant captured exchanges exactly and were accepted in the physical standalone tests. Finding the same primitive in later vendor software does not establish that the tested 2006 ECU is byte-identical to that software. Cross-engine portability remains unverified.

## Canonical 30-transmission sequence

Advance only after the stated ECU response. Autonomous 11-bit producer frames may interleave and do not satisfy a gate. Where no gate is listed, proceed as an explicit state-machine step. Abort on timeout or an unexpected directed/extended response; production timeout margins remain to be established.

| # | Client transmission | Required ECU response before next step |
|---:|---|---|
| 1 | `0x00000B73 : 55` | `0x0000730B : AA` |
| 2 | `0x00000B73 : C0 00` | `0x0000730B : 1B` |
| 3 | `0x00000B73 : C0 01` | `0x0000730B : 03` |
| 4 | `0x00000B73 : C0 06` | `0x0000730B : 0C` |
| 5 | `0x00000B73 : C0 05` | `0x0000730B : 0A` |
| 6 | `0x00000B73 : FA 04 01` | `0x0000730B : <E32>` |
| 7 | `0x00000B73 : F9 <live R32>` | `0x0000730B : 04 01` |
| 8 | `0x00000B73 : 06 00 0C 00 00` | `0x0000730B : 0C 00 00` |
| 9 | `0x00000B73 : 03 01` | `0x0000730B : 4D 59 32 30` |
| 10 | `0x00000B73 : 03 01` | `0x0000730B : 30 36 70 30` |
| 11 | `0x00000B73 : 03 01` | `0x0000730B : 41 41 41 49` |
| 12 | `0x00000B73 : 00 01` | `0x0000730B : 00` |
| 13 | `0x00000B73 : 06 00 0D 00 00` | `0x0000730B : 0D 00 00` |
| 14 | `0x00000B73 : 03 01` | `0x0000730B : 4D 59 32 30` |
| 15 | `0x00000B73 : 03 01` | `0x0000730B : 30 36 70 30` |
| 16 | `0x00000B73 : 03 01` | `0x0000730B : 41 41 41 49` |
| 17 | `0x00000B73 : 03 01` | `0x0000730B : 5F 30 39 5F` |
| 18 | `0x00000B73 : 03 01` | `0x0000730B : 33 63 79 6C` |
| 19 | `0x00000B73 : 03 01` | `0x0000730B : 34 30 5F 30` |
| 20 | `0x00000B73 : 03 01` | `0x0000730B : 31 5F 30 30` |
| 21 | `0x00000B73 : 03 01` | `0x0000730B : 30 00 00 00` |
| 22 | `0x00000B73 : 55` | no response observed |
| 23 | `0x00000B73 : 55` | `0x0000730B : AA` |
| 24 | `0x00000B73 : 55` | `0x0000730B : AA` |
| 25 | `0x1608B073 : 00 FF FF FF FF 7F FF FF` | no response observed |
| 26 | `0x1608B173 : 00 FF FF 7F FF FF FF FF` | no response observed |
| 27 | `0x00000B73 : FA 02 06` | `0x0000730B : <E32>` |
| 28 | `0x00000B73 : F9 <live R32>` | `0x0000730B : 02 06` |
| 29 | `0x00000B73 : 80 04` | `0x0000730B : <E32>` |
| 30 | `0x00000B73 : 81 <live R32>` | `0x0000730B : 04` |

The identity/profile response bytes above were observed on the tested ECU and may be engine/software-specific. A compatible implementation should validate rather than blindly assume them when adapting to another engine.

## Expanded producer state and persistence

After successful startup, with no further SmartCraft protocol TX, the tested ECU produced:

| CAN ID | Pages |
|---|---|
| `0x170` | `00 01 02 03 04 05 06 FF` |
| `0x1A0` | `00 01 02 03 04 05 06 07 08 09 0A 0B 0C FF` |

No `0x1E0` or `0x1F0` traffic was required or observed in the standalone runs. The expanded state persisted through a complete 300-second observation with zero post-startup SmartCraft transmissions. This verifies that no periodic keepalive was required within that interval; it does not prove indefinite, ignition-cycle or power-cycle persistence.

During later voltage testing, an IGN/session reset was directly followed by loss of expanded producer pages. A final implementation must therefore monitor required-page freshness and establish a new session after a sustained reset/loss condition.

CraftBridge requires target-data coverage, not replication of every frame seen with Connect Mobile. The expanded `0x170` and `0x1A0` sets contain all six required inputs; additional `0x1E0`, `0x1F0`, `0x673` and management/session traffic are not required for the contracted data set.

## Six verified signal mappings

[SMARTCRAFT_INPUT_CONTRACT.md version 1.1.0](../SMARTCRAFT_INPUT_CONTRACT.md) remains normative. This table is the synchronized public engineering summary.

| Signal / normalized field | CAN mapping | Conversion | Availability and evidence boundary |
|---|---|---|---|
| RPM / `rpm` | `0x170`, page `00`, `D2:D3`, u16be | `rpm = raw`; 1 RPM/bit | Verified on tested ECU; autonomous baseline |
| Coolant / `coolant_temperature_c` | `0x1A0`, page `07`, `D3` | `temperature_c = raw`; 1 °C/bit | Verified on tested ECU; expanded state |
| Runtime / `runtime_hours` | `0x1A0`, page `02`, `D4:D5`, u16be minutes | `runtime_hours = raw / 60.0` | Verified on tested ECU; expanded state; `0x1E0/page 00` mirror not required |
| Oil status / `oil_pressure_ok` | `0x1A0`, page `05`, `D4:D5`, u16be | stable CLOSED (`0x0000`/`0x0001`) = false; stable OPEN (`0x9B82`) = true | Verified filtered binary switch representation; transition values remain invalid; never convert to kPa |
| Battery / `battery_voltage_v` | `0x1A0`, page `09`, `D5:D6`, u16be | `battery_voltage_v = raw × 0.001` | Verified on tested ECU for charging/status use; ECU supply may differ from battery-terminal DMM |
| Fuel / `fuel_flow_lph` | `0x170`, page `01`, `D2:D3`, u16be | `fuel_flow_lph = raw × 0.01` | Verified against synchronized Connect Mobile reference; not laboratory flow-meter verification |

### Oil evidence boundary

The tested engine uses a binary oil-pressure switch: below approximately 20 kPa the switch is CLOSED to ground; above approximately 20 kPa it is OPEN. Repeated controlled interventions produced repeatable CAN changes. Short transition/filter values included `0x9B78`, `0x9B5C` and `0x99F4`; the ECU filtering algorithm is unknown. Connect Mobile numeric presentation does not turn this into an analog pressure measurement.

### Battery evidence boundary

The mapping combines multiple Connect Mobile correlations with independent battery-terminal DMM measurements and consistent stopped-versus-charging behavior. It represents ECU supply/battery-system voltage; it does not prove direct measurement at the battery terminals. Approximately 0.1 V practical resolution is sufficient for the intended charging indication.

### Fuel evidence boundary

The mapping explained 35/36 usable synchronized references, 8/8 repeated 0.4-to-0.5 l/h events and 4/4 large excursions. Mean absolute reference error was approximately 0.034 l/h, maximum approximately 0.130 l/h, and observed app lag approximately 0.392-0.856 s with 0.806 s median. Different fuel rates near the same RPM demonstrate that it is not a simple RPM lookup. Accumulated consumption is not a separate verified ECU field; software may later integrate fresh instantaneous flow.

None of the six decoders, normalized fields or downstream transports is implemented in the current repository.

## Session coexistence design requirement

Status: **DESIGN DECISION — NOT IMPLEMENTED**.

CraftBridge follows a coexistence-first policy: after IGN ON it waits and monitors for the required expanded producer state, gives Connect Mobile the first opportunity to establish it, remains passive when the required pages are already present, and performs standalone initialization only after a defined timeout. During operation, a single missed frame does not trigger recovery; required expanded pages must remain absent beyond a defined freshness timeout before controlled re-establishment.

The approximately 5.8-second Connect startup delay is an observation on the tested setup and a design reference, not a universal Mercury protocol constant. An IGN OFF/session reset requires a new establishment.

## Non-normative architectural interpretation

**PLAUSIBLE ARCHITECTURAL INTERPRETATION — NOT VERIFIED PROTOCOL FACT.** Addressed 29-bit traffic is usefully modeled as a management/session/control plane; `0x170` and `0x1A0` are the primary contracted engine-data producer families; `0x673` appears auxiliary/presence/session-related; and `0x1E0`/`0x1F0` appear as extra or capability-specific Connect-expanded families. CraftBridge needs target-data coverage, not full-bus equivalence.

The ECU may calculate instantaneous fuel rate from injection-control information and calibration, and Connect Mobile may integrate instantaneous flow for accumulated consumption. This is a plausible interpretation, not a reverse-engineered internal algorithm.

## Adapting to another engine

1. Capture a cold ECU-only baseline and record IDs, pages, bitrate, topology, and engine/software identity.
2. Compare directed startup responses step by step; do not mix sessions.
3. Calculate each response from that session’s live challenge.
4. Stop on an unexpected directed response instead of continuing or retrying blindly.
5. Compare the post-startup producer set and persistence without assuming identical pages.
6. Validate every signal against a controlled physical reference and repeated state changes.
7. Report engine model/year, ECU/software identity if available, hardware, raw minimal captures, test conditions, and repetitions.

The startup traffic is narrowly scoped session establishment. CraftBridge must not expose arbitrary SmartCraft bridging, fuzzing, diagnostics, calibration, configuration, or control passthrough.
