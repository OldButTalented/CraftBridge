# Firmware Architecture

## Principles

- Engine-side firmware is a passive consumer of SmartCraft broadcasts.
- Only inputs authorized by [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md) enter the production decoder allowlist.
- Wireless transport carries normalized values, not raw SmartCraft frames.
- Helm-side firmware owns NMEA formatting, address claim and PGN scheduling.
- Stale, malformed or unsupported data fails closed.
- SmartCraft receive processing must never block on Wi-Fi or logging.

## Shared modules

| Module | Responsibility |
|---|---|
| `gateway_types` | Fixed-width normalized signal types and validity flags |
| `esp_now_packet` | Encode/decode versioned wire packets and CRC |
| `freshness` | Per-signal age, stale thresholds and unavailable state |
| `health_counters` | Resets, sequence gaps, parse failures and queue overflows |
| `config` | Compile-time defaults and validated nonvolatile settings |

Names are architectural placeholders; source-tree layout is TBD when implementation begins.

## Engine-side firmware

### Startup sequence

1. Configure safe GPIO defaults; if the ESP-IDF API requires a TWAI TX GPIO, select an unused GPIO that has no physical route to the transceiver.
2. Initialize monotonic clock and boot ID.
3. Initialize queues and health counters.
4. Configure TWAI in listen-only mode at the independently verified SmartCraft bitrate.
5. Initialize Wi-Fi station mode, fixed channel and ESP-NOW peer.
6. Start periodic normalized snapshot publication.

If bitrate or required configuration is Unknown, startup must stop before live SmartCraft receive is enabled.

### Tasks

| Task | Priority/behavior |
|---|---|
| SmartCraft RX | Highest application priority; drains TWAI receive queue, validates frame shape and timestamps frames |
| Verified decoder | Applies the exact rules from the controlled SmartCraft input contract and updates a lock-free or short-lock signal store |
| Snapshot publisher | Copies the current signal store, calculates per-signal age and sends a bounded ESP-NOW packet |
| Health monitor | Tracks bus errors, queue overflow, wireless failures, resets and uptime |
| Optional debug logger | Bench-only, rate-limited and unable to inject CAN traffic |

### Decoder allowlist

[`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md), version 1.0.0, is the sole authority for concrete CAN IDs, pages, fields, byte order, scaling and units used by the decoder.

The initial eligible signal names are RPM, engine temperature and engine runtime. Their concrete input definitions are intentionally not duplicated here.

The implementation must choose one documented runtime source or reconcile the two without double-counting. That choice is TBD.

All inputs absent from the current SmartCraft input contract remain excluded.

## Helm-side firmware

### Startup sequence

1. Initialize local clock, queues and health counters.
2. Configure ESP-NOW peer and receive callback.
3. Initialize the selected NMEA 2000 stack in listen-only or simulation mode first.
4. After the test-plan gate, enable NMEA address claim and permitted PGN transmission on the dedicated segment.

### Tasks

| Task | Responsibility |
|---|---|
| ESP-NOW RX callback | Copy packet into a bounded queue only; no long processing in Wi-Fi callback context |
| Packet validator | Validate MAC/peer, length, magic, version, CRC, boot ID, sequence and signal bounds |
| Signal state | Record local receive time, per-signal source age and current validity |
| Freshness monitor | Mark each signal stale independently; never substitute zero |
| NMEA scheduler | Emit only enabled mappings at approved rates and with the selected engine instance |
| NMEA stack service | Address claim, product information, incoming stack events and bus health |
| Diagnostics | Expose counters through serial/bench logging without affecting timing |

Espressif states that ESP-NOW callbacks run in the Wi-Fi task and should queue work rather than perform lengthy processing. The architecture follows that rule.

## State machines

### Wireless link state

```text
INIT -> WAIT_FOR_PACKET -> FRESH -> STALE
  ^          |              |        |
  |          +-- bad data --+        |
  +---------- peer reinit/reboot -----+
```

- `FRESH` requires a valid current-version packet and acceptable age.
- New `boot_id` resets sequence history.
- Duplicate or out-of-order packets are discarded.
- Link recovery does not backfill old NMEA values.

### NMEA output state

```text
BENCH_DISABLED -> ADDRESS_CLAIM -> ACTIVE -> DATA_STALE
       ^              |             |          |
       +------ fault / approval removed -------+
```

NMEA transmission has an explicit build/runtime enable gate. SmartCraft reception does not depend on NMEA state.

## Stale data policy

- Engine node calculates source age from the last matching SmartCraft frame.
- Helm node adds local wireless age from packet reception.
- Each signal has a separately reviewed timeout based on its observed update rate and intended PGN rate.
- Proposed timeout values are **TBD**; they must be derived from capture statistics and fault-injection tests.
- On stale transition, the NMEA scheduler uses the library's unavailable encoding or suppresses the PGN, depending on the selected PGN and Garmin behavior.

## Configuration requiring review

- SmartCraft bitrate.
- ESP32 GPIOs.
- ESP-NOW channel, peer MACs, keys and send interval.
- Signal stale thresholds.
- NMEA library, device identity, source address strategy, engine instance and PGN periods.
- Debug logging and watchdog thresholds.

