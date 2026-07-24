# ESP-NOW Protocol

## Protocol goals

- Deterministic fixed-width encoding.
- Explicit versioning and byte order.
- Detection of corruption, duplicates, resets and stale data.
- No dependency on raw SmartCraft frame layout at the helm.
- Forward extension without silently reinterpreting old packets.

## Transport profile

- ESP-NOW unicast between one configured engine peer and one helm peer.
- Fixed Wi-Fi channel configured on both nodes; channel selection is TBD after installation testing.
- Link-layer encryption is recommended; PMK/LMK provisioning is TBD.
- A repeated full snapshot is preferred over delta packets so one lost packet does not corrupt state.
- Send period is TBD from source update rates and RF testing.

ESP-NOW send success confirms MAC-layer delivery, not application-layer processing. Version 1 therefore uses repeated snapshots plus sequence monitoring. An application ACK/retry extension remains TBD if loss testing shows it is necessary.

## Version 1 packet

All multibyte fields use little-endian wire order. The packet is 48 bytes.

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 2 | `magic` | Constant `0x5343` (`SC`) |
| 2 | 1 | `protocol_version` | `1` for this layout |
| 3 | 1 | `message_type` | `1` = normalized engine snapshot |
| 4 | 2 | `payload_length` | Total bytes, currently `48` |
| 6 | 2 | `header_flags` | Reserved; transmit as zero |
| 8 | 4 | `boot_id` | Random/nonrepeating identifier generated at engine-node boot |
| 12 | 4 | `sequence` | Incremented once per attempted snapshot; modulo 2^32 |
| 16 | 4 | `source_monotonic_ms` | Engine-node monotonic time at snapshot creation |
| 20 | 4 | `valid_mask` | One bit per normalized signal |
| 24 | 2 | `rpm` | Unsigned RPM |
| 26 | 2 | `engine_temp_centi_c` | Signed temperature in 0.01 degree C |
| 28 | 4 | `engine_runtime_minutes` | Verified raw runtime unit normalized to minutes |
| 32 | 2 | `rpm_age_ms` | Age at source, saturated at 65535 |
| 34 | 2 | `engine_temp_age_ms` | Age at source, saturated at 65535 |
| 36 | 2 | `runtime_age_ms` | Age at source, saturated at 65535 |
| 38 | 2 | `reserved` | Transmit as zero |
| 40 | 4 | `status_flags` | Source health and diagnostic flags |
| 44 | 4 | `crc32` | CRC-32 over bytes 0–43; polynomial/profile TBD and version-controlled |

### Valid mask

| Bit | Signal | Version 1 status |
|---:|---|---|
| 0 | RPM | Eligible from a Verified source mapping |
| 1 | Engine temperature | Eligible from a Verified source mapping |
| 2 | Engine runtime minutes | Eligible from a Verified source mapping |
| 3–31 | Reserved | Must be zero |

A numeric field whose validity bit is clear must be ignored regardless of its encoded value.

### Status flags

Proposed flags are architectural and must be frozen with firmware implementation:

- SmartCraft controller initialized.
- SmartCraft receive queue overflow observed.
- ESP-NOW send failure observed.
- Decoder rejected malformed source frames.
- Runtime-source disagreement observed.
- Engine-node brownout/reset cause available.

## Sequence and reset handling

- Receiver tracks `(peer MAC, boot_id, sequence)`.
- Same boot ID and same sequence: duplicate, discard.
- Same boot ID and backward sequence outside modular wrap rules: out of order, discard and count.
- New boot ID: clear prior sequence and signal state; require fresh valid samples.
- Sequence gaps increment a loss counter but do not invalidate a later complete snapshot by themselves.

## Timestamp and stale handling

ESP32 clocks are not assumed synchronized.

- `source_monotonic_ms` orders snapshots within one boot epoch.
- Per-signal age records how old the source CAN observation was when transmitted.
- Helm records its own `local_rx_ms` on callback receipt.
- Effective age is source field age plus elapsed helm time since reception.
- Overflow/saturation means stale unless a reviewed signal policy explicitly allows it.

## Reconnect behavior

ESP-NOW has no stream connection to reconnect. Recovery is implemented as peer and freshness management:

1. Each node initializes Wi-Fi and the configured peer after boot.
2. Engine node continues CAN reception even if wireless initialization or sends fail.
3. Send failures trigger bounded backoff and peer reinitialization without blocking the RX task.
4. Helm remains `STALE` until a valid current-version packet arrives.
5. A new boot ID starts a clean epoch; no old value is replayed.
6. After RF recovery, the next complete snapshot restores eligible signals without a historical backlog.

## Validation rules

Reject packets with:

- Unexpected source MAC.
- Wrong length, magic, version or message type.
- CRC failure.
- Reserved bits set when the version requires zero.
- Out-of-range RPM, temperature or runtime values beyond reviewed plausibility limits.
- Duplicate/out-of-order sequence.
- Source or local age beyond the configured threshold.

## Remaining TBD

- ESP-IDF version and ESP-NOW API generation.
- Wi-Fi channel and coexistence survey.
- PMK/LMK provisioning and replacement process.
- Send rate, backoff and whether application ACK is necessary.
- CRC-32 polynomial/profile and golden test vectors.
- Plausibility limits and stale thresholds.

