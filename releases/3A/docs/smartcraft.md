# SmartCraft protocol and signal findings

This document is the public technical baseline needed to implement or adapt CraftBridge. It describes independently observed behavior on one tested installation; it is not an official Mercury specification.

[SMARTCRAFT_INPUT_CONTRACT.md version 1.3.0](SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source for concrete SmartCraft input mappings and normalized semantics. This document provides protocol/session context and must not be used as a competing signal-definition source.

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

This exact 30-transmission, 27-gate behavior is implemented in the Option 3A Motor Node core and verified by deterministic host tests. The accepted baseline also passed physical bench and real-ECU/boat validation. The separate Python/CANable result remains supporting protocol evidence and is not substituted for validation of the released firmware.

## S0 → 30-step handshake → S3

CraftBridge calls the limited producer traffic before authorization **S0** and the complete expanded producer data set **S3**. These are CraftBridge terms, not official Mercury terminology.

When a fresh S3 session is not already present, the Motor Node performs the physically verified **SmartCraft session authorization handshake**: 30 ordered client transmissions, 27 response gates, and three responses calculated from live ECU challenges. Passing the final gate is followed by passive S3 confirmation; the session is active only when the complete expanded producer set is fresh.

The authoritative human-readable sequence, including all 30 steps, gate responses, challenge-response parameters, and verified calculation examples, is [SmartCraft session authorization handshake](../../../SMARTCRAFT_SESSION_AUTHORIZATION.md). The executable firmware table remains authoritative for implementation behavior.
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

[SMARTCRAFT_INPUT_CONTRACT.md version 1.3.0](SMARTCRAFT_INPUT_CONTRACT.md) remains normative. This table is the synchronized public engineering summary.

| Signal / normalized field | CAN mapping | Conversion | Availability and evidence boundary |
|---|---|---|---|
| RPM / `rpm` | `0x170`, page `00`, `D2:D3`, u16be | `rpm = raw`; 1 RPM/bit | Verified on tested ECU; autonomous baseline |
| Coolant / `coolant_temperature_c` | `0x1A0`, page `07`, `D3` | `temperature_c = raw`; 1 °C/bit | Verified on tested ECU; expanded state |
| Runtime / `runtime_hours` | `0x1A0`, page `02`, `D4:D5`, u16be minutes | `runtime_hours = raw / 60.0` | Verified on tested ECU; expanded state; `0x1E0/page 00` mirror not required |
| ECU-reported oil pressure / `oil_pressure_kpa` | `0x1A0`, page `05`, `D4:D5`, u16be | `oil_pressure_kpa = raw × 0.01`; offset 0; unit kPa | Verified on tested ECU; observed 0.00–398.47 kPa is not a protocol-global valid range; no sentinel documented |
| Battery / `battery_voltage_v` | `0x1A0`, page `09`, `D5:D6`, u16be | `battery_voltage_v = raw × 0.001` | Verified on tested ECU for charging/status use; ECU supply may differ from battery-terminal DMM |
| Fuel / `fuel_flow_lph` | `0x170`, page `01`, `D2:D3`, u16be | `fuel_flow_lph = raw × 0.01` | Verified against synchronized Connect Mobile reference; not laboratory flow-meter verification |

### Oil evidence boundary

The numeric mapping is Verified against synchronized Connect Mobile reference on the tested ECU. The tested engine nevertheless uses a binary oil-pressure switch, so the ECU-reported numeric value is filtered or substituted from switch state and is not measured analog hydraulic pressure. Observed values include `0x0000`, `0x0001`, `0x9B82`, `0x9B8B`, `0x9B94` and `0x9BA7`. The observed 0.00–398.47 kPa profile is not a protocol-global valid range; no sentinel is documented, and the unobserved `0xFFFF` is not assigned invented semantics.

### Battery evidence boundary

The mapping combines multiple Connect Mobile correlations with independent battery-terminal DMM measurements and consistent stopped-versus-charging behavior. It represents ECU supply/battery-system voltage; it does not prove direct measurement at the battery terminals. Approximately 0.1 V practical resolution is sufficient for the intended charging indication.

### Fuel evidence boundary

The mapping explained 35/36 usable synchronized references, 8/8 repeated 0.4-to-0.5 l/h events and 4/4 large excursions. Mean absolute reference error was approximately 0.034 l/h, maximum approximately 0.130 l/h, and observed app lag approximately 0.392-0.856 s with 0.806 s median. Different fuel rates near the same RPM demonstrate that it is not a simple RPM lookup. Accumulated consumption is not a separate verified ECU field; software may later integrate fresh instantaneous flow.

Five unchanged contracted decoders remain implemented and host-tested. The version 1.3.0 numeric oil-pressure decoder and normalized field are not yet implemented; the current boolean oil-status runtime/UI path is legacy behavior pending a separate firmware task. Downstream transports and outputs remain unimplemented.

## Session coexistence design requirement

Status: **IMPLEMENTED AND HOST-TESTED**.

CraftBridge follows a coexistence-first policy: after IGN ON it waits and monitors for the required expanded producer state, gives Connect Mobile the first opportunity to establish it, remains passive when the required pages are already present, and performs standalone initialization only after a defined timeout. During operation, a single missed frame does not trigger recovery; required expanded pages remaining absent beyond the defined freshness timeout cause terminal session loss with application TX disabled; startup is not replayed automatically.

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
