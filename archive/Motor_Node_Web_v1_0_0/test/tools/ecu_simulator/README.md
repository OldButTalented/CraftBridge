# CraftBridge desktop ECU simulator

Windows terminal simulator for the verified subset of Mercury ECM-555 SmartCraft behavior needed by Motor Node SW 1.0. It controls one CANable through SLCAN and never opens the Motor Node COM port.

## Verification status

- Desktop simulator: `SOFTWARE_VERIFIED`
- Physical CANable gate: `ECU_SIMULATOR_PHYSICAL_CANABLE_GATE=PASS`
- Evidence: `test_runs/ecu_sim_20260825_142935`

## Physical CANable evidence

Run `ecu_sim_20260825_142935` records a non-dry-run PASS on COM3 at serial baud 115200 using LAWICEL/SLCAN and 250 kbit/s CAN. Fresh-startup completed with 30 exact Motor Node TX steps and 27 gates, followed by sustained S3. No unexpected Motor Node TX or failure was recorded. `PAUSE rpm` suppressed RPM-page TX for 23.61 seconds; the first RPM page followed `RESUME rpm` by 47 ms. `STOP` produced terminal `stopped`/`PASS`.

The run folder contains simulator `raw_events.jsonl` and `summary.json`; it does not contain Motor Node serial output, a web/API capture or screenshots. Motor Node S3 status and correct web value/stale/fresh presentation are operator observations, not independently preserved artifacts in this folder.

The physical run used a different Motor Node flash image. The current public refactored image has not passed this physical gate and must not inherit its PASS status.
## Safety boundary

- Default CANable port: `COM3`; `COM9` is explicitly refused.
- CAN: 250 kbit/s (`S5`), standard and extended frames supported.
- Startup is always `SAFE IDLE` with zero CAN application TX.
- Transmission starts only after the operator types exact uppercase `START`.
- No automatic start or restart.
- Unexpected Motor Node TX or timeout causes terminal FAIL and permanently stops simulator CAN TX.
- `STOP` and Ctrl+C close the SLCAN channel with `C` before closing Serial.

## Authority

`authority_snapshot.json` is generated from:

- `FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE/CONNECT_TX_SEQUENCE.csv`;
- `FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE/REPLAY_RESPONSE_GATES.json`;
- the successful `execute_20260823_094341/raw_can.jsonl` 300-second passive capture;
- SmartCraft Input Contract v1.3.0 for the six signal overlays.

The snapshot contains 30 Motor Node TX expectations, 27 ECU gates, three transform definitions and 22 complete S3 page templates. `build_authority_snapshot.py --check` detects drift against source-of-truth.

## Installation

```powershell
cd releases/3A/test/tools/ecu_simulator
py -m pip install -r requirements.txt
```

Only `pyserial` is required beyond Python's standard library.

## Modes

### Fresh startup

Sends only verified ECU-only S0 until the Motor Node executes the exact 30-frame startup. Every TX is checked before the corresponding authoritative ECU response is sent. Dynamic challenges come from the configured deterministic RNG seed. Successful startup transitions to complete S3 simulation.

```powershell
py ecu_simulator.py --port COM3 --serial-baud 115200 --mode fresh-startup --seed 20260825 --output test_runs
```

### Existing S3

Starts complete S3 after `START`. Any Motor Node application TX is an immediate FAIL.

```powershell
py ecu_simulator.py --port COM3 --serial-baud 115200 --mode existing-s3 --seed 20260825 --output test_runs
```

## Commands

Commands are line-oriented:

```text
START
STATUS
PAUSE rpm|temperature|hours|oil|battery|fuel|s3
RESUME rpm|temperature|hours|oil|battery|fuel|s3
SET rpm 1500
SET temperature 80
SET hours 224.5
SET oil 398.19
SET battery 13.8
SET fuel 0.7
STOP
```

Pause a signal before its first S3 page to test `missing`; pause it later to test `stale`. `SET oil <kPa>` encodes the requested value as unsigned big-endian hundredths of kPa. The simulator enforces only u16 representability; it does not invent a protocol-global valid range or sentinel.

Generated values use a bounded deterministic random walk. Hours are monotonic. Signal overlays change only the documented bytes; every other byte remains from the verified capture template. Expected engineering values are printed once per second and written to `raw_events.jsonl`.

## Dry run and tests

Dry run opens no COM port:

```powershell
py ecu_simulator.py --dry-run --mode fresh-startup --seed 123 --output dry_runs
```

Type `START`, inspect `STATUS`, then `STOP`. Unit tests emulate the complete Motor Node handshake without hardware:

```powershell
py -m unittest discover -s tests -v
```

Authority drift check:

```powershell
py build_authority_snapshot.py --check `
  --authority "<FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE>" `
  --raw "<PASSIVE_CAPTURE_RAW_CAN_JSONL>" `
  --contract "..\..\..\docs\SMARTCRAFT_INPUT_CONTRACT.md" `
  --output authority_snapshot.json
```

## Physical COM3 retest procedure

1. Leave Motor Node disconnected from USB while checking CAN-H, CAN-L, common ground and exactly two bench terminators.
2. Connect CANable to the PC as COM3 and Motor Node separately as COM9.
3. Start the simulator with the fresh-startup command above.
4. Confirm port, baudrate, 250 kbit/s, mode and `SAFE IDLE / CAN application TX = 0`.
5. Start Motor Node logging separately on COM9.
6. Type `START` in the ECU simulator terminal.
7. Observe startup validation, S3 output and expected signal values.
8. Use `STATUS` and controlled pause/resume/set scenarios.
9. Type `STOP`; preserve the complete timestamped simulator and Motor Node run folders.

Each run creates `test_runs/ecu_sim_YYYYMMDD_HHMMSS/` with `raw_events.jsonl` and `summary.json`.
