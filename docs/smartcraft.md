# SmartCraft protocol and signal findings

This document is the public technical baseline needed to implement or adapt CraftBridge. It describes independently observed behavior on one tested installation; it is not an official Mercury specification.

[SMARTCRAFT_INPUT_CONTRACT.md version 1.1.0](../SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source for concrete SmartCraft input mappings and normalized semantics. This document provides protocol/session context and must not be used as a competing signal-definition source.

## Evidence scope

- **Tested engine:** 2006-generation Mercury 40 EFI FourStroke
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

## Standalone startup result

A Python + CANable implementation reproduced the required startup without Connect Mobile:

- 30 client-side CAN transmissions;
- 27 fixed transmissions;
- 3 responses calculated from live ECU challenges;
- 27 response gates;
- 0 automatic retry paths;
- 20/20 historical startup structures matched;
- 2 successful physical standalone runs on the tested ECU.

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

Use the live challenge from the current session. Historical response bytes must not be replayed as constants. These values are physically verified on the tested ECU and are likely reusable only within related software/project families until cross-engine testing proves more.

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

No `0x1E0` or `0x1F0` traffic was required or observed in the standalone runs. The expanded state persisted through a complete 300-second observation with zero post-startup SmartCraft transmissions. This verifies that no periodic keepalive was required within that interval; it does not prove indefinite, ignition-cycle, or power-cycle persistence.

## Contracted input set

The controlled version 1.1.0 contract authorizes six tested-ECU inputs: RPM, engine coolant temperature, engine runtime, oil-pressure status, ECU supply/battery voltage and instantaneous fuel flow.

Oil is a filtered representation of a binary pressure-switch state, not analog pressure. Fuel is verified against synchronized Connect Mobile display evidence rather than a physical flow meter. The repository currently implements none of the six decoders or normalized fields.

All concrete CAN IDs, pages, byte fields, encodings, scales and validity semantics are maintained only in [SMARTCRAFT_INPUT_CONTRACT.md](../SMARTCRAFT_INPUT_CONTRACT.md).

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
