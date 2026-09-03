# SmartCraft session authorization handshake

This document shows the complete 30-step startup handshake used by CraftBridge to make the tested Mercury ECU enter the expanded SmartCraft data state.

The sequence was reconstructed from captured SmartCraft CAN traffic and physically verified on the tested Mercury 40 EFI FourStroke (2006) with ECM-555. It is not an official Mercury protocol specification.

**S0**, **S3**, **SmartCraft session authorization handshake**, and **30-step handshake** are CraftBridge terms used to describe the observed behavior. They are not official Mercury terminology.

## What happens

1. After power-up, CraftBridge listens passively for 8 seconds.
2. If the complete expanded data set is already present, CraftBridge remains passive and does not transmit the handshake.
3. Otherwise, CraftBridge executes the 30 steps below in exactly this order.
4. After step 30, CraftBridge stops transmitting and verifies that the complete expanded data set appears.

The handshake contains:

- 30 transmissions from CraftBridge.
- 27 ECU response gates.
- 24 fixed ECU responses.
- Three dynamic ECU challenges.
- Three CraftBridge transmissions with no expected ECU response: steps 22, 25, and 26.

Except where another ID is shown, CraftBridge transmits on `0x00000B73` and the ECU responds on `0x0000730B`.

## Complete 30-step sequence

The dynamic values shown in *italics* are examples from one verified capture. They are not fixed values and normally change in a new session.

| Step | TX CAN ID | CraftBridge to ECU | TX value type | ECU to CraftBridge | ECU value type | Processing |
|---:|---:|---|---|---|---|---|
| 1 | `0x00000B73` | `55` | Fixed | `AA` | Fixed | Validate response |
| 2 | `0x00000B73` | `C0 00` | Fixed | `1B` | Fixed | Validate response |
| 3 | `0x00000B73` | `C0 01` | Fixed | `03` | Fixed | Validate response |
| 4 | `0x00000B73` | `C0 06` | Fixed | `0C` | Fixed | Validate response |
| 5 | `0x00000B73` | `C0 05` | Fixed | `0A` | Fixed | Validate response |
| 6 | `0x00000B73` | `FA 04 01` | Fixed challenge request | *Example: `51 09 2A 8E`* | **Dynamic challenge** | Store as `E1` |
| 7 | `0x00000B73` | *Example: `F9 98 18 EC 3A`* | **Calculated response** | `04 01` | Fixed acknowledgement | Validate acknowledgement |
| 8 | `0x00000B73` | `06 00 0C 00 00` | Fixed | `0C 00 00` | Fixed | Validate response |
| 9 | `0x00000B73` | `03 01` | Fixed | `4D 59 32 30` (`MY20`) | Fixed | Validate response |
| 10 | `0x00000B73` | `03 01` | Fixed | `30 36 70 30` (`06p0`) | Fixed | Validate response |
| 11 | `0x00000B73` | `03 01` | Fixed | `41 41 41 49` (`AAAI`) | Fixed | Validate response |
| 12 | `0x00000B73` | `00 01` | Fixed | `00` | Fixed | Validate response |
| 13 | `0x00000B73` | `06 00 0D 00 00` | Fixed | `0D 00 00` | Fixed | Validate response |
| 14 | `0x00000B73` | `03 01` | Fixed | `4D 59 32 30` (`MY20`) | Fixed | Validate response |
| 15 | `0x00000B73` | `03 01` | Fixed | `30 36 70 30` (`06p0`) | Fixed | Validate response |
| 16 | `0x00000B73` | `03 01` | Fixed | `41 41 41 49` (`AAAI`) | Fixed | Validate response |
| 17 | `0x00000B73` | `03 01` | Fixed | `5F 30 39 5F` (`_09_`) | Fixed | Validate response |
| 18 | `0x00000B73` | `03 01` | Fixed | `33 63 79 6C` (`3cyl`) | Fixed | Validate response |
| 19 | `0x00000B73` | `03 01` | Fixed | `34 30 5F 30` (`40_0`) | Fixed | Validate response |
| 20 | `0x00000B73` | `03 01` | Fixed | `31 5F 30 30` (`1_00`) | Fixed | Validate response |
| 21 | `0x00000B73` | `03 01` | Fixed | `30 00 00 00` (`0` + padding) | Fixed | Validate response |
| 22 | `0x00000B73` | `55` | Fixed | — | No response expected | Continue immediately |
| 23 | `0x00000B73` | `55` | Fixed | `AA` | Fixed | Validate response |
| 24 | `0x00000B73` | `55` | Fixed | `AA` | Fixed | Validate response |
| 25 | `0x1608B073` | `00 FF FF FF FF 7F FF FF` | Fixed | — | No response expected | Continue immediately |
| 26 | `0x1608B173` | `00 FF FF 7F FF FF FF FF` | Fixed | — | No response expected | Continue immediately |
| 27 | `0x00000B73` | `FA 02 06` | Fixed challenge request | *Example: `F4 99 15 62`* | **Dynamic challenge** | Store as `E2` |
| 28 | `0x00000B73` | *Example: `F9 8B 98 E2 95`* | **Calculated response** | `02 06` | Fixed acknowledgement | Validate acknowledgement |
| 29 | `0x00000B73` | `80 04` | Fixed challenge request | *Example: `FE 36 4D 07`* | **Dynamic challenge** | Store as `E3` |
| 30 | `0x00000B73` | *Example: `81 04 5D 45 A7`* | **Calculated response** | `04` | Fixed acknowledgement | Validate acknowledgement |

Each row represents one CraftBridge transmission. Where an ECU response is shown on the same row, CraftBridge waits for that exact response before continuing.

The response CAN ID, DLC, and complete payload must match. A missing or incorrect response stops the handshake. The response timeout is 250 ms per gate, and the firmware does not automatically restart the sequence.

## The three dynamic steps

The requests in steps 6, 27, and 29 are fixed, but the ECU returns a different four-byte challenge for each session.

The next step calculates a response from the received challenge. All three calculations use the same formula with different fixed values:

```text
R = low32(E * M) XOR X
```

| Challenge request | Multiplier `M` | XOR value `X` | Response prefix | Fixed ECU acknowledgement |
|---|---:|---:|---:|---|
| `FA 04 01` | `0xD379A9C8` | `0x1B4610CA` | `F9` | `04 01` |
| `FA 02 06` | `0xCF88B813` | `0x4353E4D3` | `F9` | `02 06` |
| `80 04` | `0xAB20FA1B` | `0x208FB01A` | `81` | `04` |

Where:

- `E` is the ECU's four challenge bytes interpreted as one big-endian 32-bit value.
- `M` is the fixed multiplier selected by the challenge request.
- `low32` means that only the lowest 32 bits of the multiplication result are retained.
- `X` is the fixed XOR value selected by the challenge request.
- `R` is the calculated four-byte response.

The response sent to the ECU is:

```text
Response prefix + R
```

The prefix `F9` or `81` is not part of the calculation. It is a fixed first byte placed before the four calculated response bytes.

The multiplier and XOR values are not received from the ECU during startup. They are fixed parameters recovered during the SmartCraft reverse-engineering work and embedded in the CraftBridge implementation.

## Calculation example for steps 6 and 7

In the example capture, step 6 produced this challenge:

```text
E = 51 09 2A 8E = 0x51092A8E
```

For request `FA 04 01`:

```text
M = 0xD379A9C8
X = 0x1B4610CA
```

Apply the formula:

```text
E * M           = 0x42F11126835EFCF0
low32(E * M)    = 0x835EFCF0
R               = 0x835EFCF0 XOR 0x1B4610CA
R               = 0x9818EC3A
```

Add the fixed `F9` response prefix:

```text
F9 + 98 18 EC 3A = F9 98 18 EC 3A
```

This is the calculated payload transmitted in step 7. The ECU must then return the fixed acknowledgement `04 01` before CraftBridge proceeds to step 8.

## Successful completion

A correct acknowledgement after step 30 proves that all handshake gates passed. CraftBridge then stops transmitting and listens for the expanded ECU data.

The session is declared active only when the complete expected data set is present and fresh. Passing the 30-step handshake without the expanded data appearing is not considered a successful session.

## Scope

This sequence is physically verified on the tested ECU. It must not be described as universally compatible with all Mercury engines, ECU families, or model years without additional testing.
