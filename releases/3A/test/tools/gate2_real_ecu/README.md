# CraftBridge HW Gate 2 — real ECU harness

## Purpose and boundary

This is dedicated field-test firmware for proving bidirectional communication between the physically Gate-1-approved ESP32-S3/CAN prototype and the tested Mercury ECU. It is not CraftBridge product firmware and must not be merged into the Motor Node application.

Behavioral authority is the verified `FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE` dataset supplied separately for comparison.

The sketch ports that tool's exact 30 application TX frames, 27 response gates, 250 ms gate timeout, three live big-endian transforms, and fail-safe no-retry behavior. The Gate-2 requirement additionally checks the full expanded `0x170`/`0x1A0` S3 page sets before a 300-second passive observation and continuously checks their freshness. It never requires `0x1E0` or `0x1F0`.

## Prerequisites

- Gate-1-approved ESP32-S3 N16R8 prototype
- external 3.3 V CAN transceiver
- TWAI TX GPIO4 and RX GPIO5
- SmartCraft CAN at 250 kbit/s
- USB serial at 115200 baud
- Arduino IDE with official `esp32 by Espressif Systems`
- Python 3 plus `pyserial` for evidence logging

Do not use CANable in Gate 2. Do not connect until CAN-H/CAN-L, ground reference, power, transceiver and termination have been checked.

## Arduino IDE build and upload

1. Open `Gate2_Real_ECU/Gate2_Real_ECU.ino`.
2. Select `ESP32S3 Dev Module`.
3. Select 16 MB flash, QIO flash mode and 8 MB OPI PSRAM settings matching the physically verified N16R8 prototype.
4. Select the Motor Node USB port, currently `COM9` on the field laptop.
5. Compile and upload.

Reset or power-on initializes Serial and TWAI, enables CAN reception/ACK participation, and enters `SAFE IDLE` with zero SmartCraft application TX. The active state machine starts only after the exact line-oriented command `START` (leading/trailing spaces or tabs are ignored). `START` is accepted once per boot. PASS or FAIL remains permanently passive; another attempt requires a deliberate reset or power cycle.

**RESET WITHOUT START = NO SMARTCRAFT APPLICATION TX.**

Optional repeatable workstation build from this directory:

```powershell
pio run -e gate2-esp32s3-n16r8
```

## Field run and evidence capture

Install `pyserial` once and close Arduino Serial Monitor before using the logger:

```powershell
py -m pip install pyserial
py serial_logger.py COM9 --output test_runs
```

The logger creates the timestamped run directory and begins raw capture before active testing. It displays ESP32 output live and forwards terminal lines to Serial; it never sends `START`, retries, or resets automatically.

Field procedure:

1. Upload `Gate2_Real_ECU.ino`.
2. Connect the Motor Node to the ECU.
3. Connect USB to the field laptop.
4. Set IGN ON according to the controlled test procedure.
5. Run `py serial_logger.py COM9 --output test_runs`.
6. Confirm `SAFE IDLE` and `SmartCraft application TX = 0`.
7. Confirm `Waiting for START command`.
8. Type `START` and press Enter.
9. Do not disturb the setup during active startup.
10. Observe startup and the 300-second passive test.
11. Preserve the complete timestamped run folder.
12. For another attempt, reset the ESP32 and create a new run folder.

Opening Serial may reset the ESP32; this is safe because reset enters SAFE IDLE. The same firmware works with Arduino Serial Monitor at 115200 baud using CR, LF, or CRLF: wait for SAFE IDLE, then send `START` with a normal line ending.

The run directory contains `raw_serial.log`, `metadata.json`, and `result.txt`. Preserve the complete directory, not excerpts.

## PASS criteria

- reset/boot remains in SAFE IDLE with zero SmartCraft application TX until `START`;
- `START` is accepted only once per boot;
- the initial 8-second fresh S0 ARM observation passes with zero application TX;
- all 30 verified extended application frames transmitted in exact order;
- all 27 ECU response gates pass;
- all three responses use the current live four-byte big-endian challenge;
- full S3 sets appear: `0x170` pages `00–06, FF` and `0x1A0` pages `00–0C, FF`;
- zero SmartCraft application TX after startup;
- expanded producer pages remain fresh through 300 seconds;
- terminal output ends with `GATE 2 PASS`.

The historical checksum `0x7B13B034` is valid for the known captured challenge instance used by offline verification. It is not a live-run invariant because the three response payloads depend on live challenges.

## FAIL behavior

On timeout, unexpected relevant frame, payload mismatch, malformed challenge, transmit error, missing S3 or S3 freshness loss, the harness prints the reason, last completed TX and expected gate, disables application TX, and remains passive. It does not retry.

## Offline authority comparison

Run from this directory, substituting the local authoritative path if needed:

```powershell
py verify_against_authority.py --authority "<FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE>"
```

This compares the sketch table against the authoritative CSV/JSON for all IDs, extended format, DLCs, payloads, ordering, gates and transform structure, then instantiates the known captured challenges and verifies checksum `0x7B13B034`.
