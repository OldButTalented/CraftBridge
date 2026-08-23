# SmartCraft protocol and signal findings

This document is the public technical baseline needed to implement or adapt CraftBridge. It describes independently observed behavior on one tested installation; it is not an official Mercury specification.

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

## Signal mappings

| Signal | Mapping | Evidence |
|---|---|---|
| RPM | `0x170/page 00/D2:D3`, unsigned big-endian, 1 RPM/bit | Verified; present in fresh baseline |
| Engine runtime | `0x1A0/page 02/D4:D5`, unsigned big-endian minutes; hours = raw / 60 | Verified; expanded state required |
| Coolant temperature | `0x1A0/page 07/D3`, 1 °C/bit | Verified; expanded state required |
| Oil status | tested engine has a binary pressure switch; `0x1A0/page 05/D4:D5` correlation exists | Candidate/Strong; physical byte/bit not verified |
| Fuel | unresolved | Unknown |

Do not publish candidate fields as valid production data.

## Adapting to another engine

1. Capture a cold ECU-only baseline and record IDs, pages, bitrate, topology, and engine/software identity.
2. Compare directed startup responses step by step; do not mix sessions.
3. Calculate each response from that session’s live challenge.
4. Stop on an unexpected directed response instead of continuing or retrying blindly.
5. Compare the post-startup producer set and persistence without assuming identical pages.
6. Validate every signal against a controlled physical reference and repeated state changes.
7. Report engine model/year, ECU/software identity if available, hardware, raw minimal captures, test conditions, and repetitions.

The startup traffic is narrowly scoped session establishment. CraftBridge must not expose arbitrary SmartCraft bridging, fuzzing, diagnostics, calibration, configuration, or control passthrough.
