# Implementation, build, and test status

## Motor Node SW 1.0

Motor Node Web v1.0.0 implements a standalone ESP32-S3 Motor Node with these separated layers:

- Gate-2-authoritative SmartCraft authorization/session initialization;
- TWAI receive transport and diagnostics;
- six contract-conforming decoders, including ECU-reported numeric oil pressure in kPa;
- normalized values with `fresh`, `stale`, `invalid` and `missing` quality;
- full-S3 freshness/session supervision without automatic recovery replay;
- boot-scoped Fuel used and Trip time accumulation without persistence;
- typed 250 ms web snapshots formatted only when an HTTP request arrives;
- local Wi-Fi AP, responsive eight-card main page, separate diagnostics page and JSON status endpoint.

## Public Arduino source structure

The authoritative sketch folder contains only:

- `CraftBridge_Motor_Node_Web_v1_0_0.ino`: setup, receive dispatch and main loop;
- `Config.h`: identity, pins and fixed application settings;
- `CanBus.h/.cpp`: CAN frames, TWAI transport and diagnostics;
- `SmartCraftSession.h/.cpp`: startup authorization and session supervision;
- `EngineData.h/.cpp`: typed values, decoding, scaling and freshness;
- `RuntimeMetrics.h/.cpp`: boot-scoped Fuel used and Trip time accumulation;
- `WebInterface.h/.cpp`: Wi-Fi, typed snapshots, bounded HTML/JSON formatting;
- `partitions.csv`: the preserved 16 MB custom partition layout.

No generated files belong in the sketch folder. PlatformIO writes to `.pio/`. Automated Arduino CLI validation uses a hash-identical temporary sketch copy plus external `--build-path` and `--output-dir`, because the installed toolchain may create an ignored `build/` export beside the sketch when invoked directly. Any such local export must be removed before commit.

## Gate 2 real-ECU evidence

The controlled run `gate2_20260825_114511` passed using firmware `CraftBridge-Gate2-Real-ECU/1.1.0`. The complete serial log and metadata establish:

- power-on entered SAFE IDLE with SmartCraft application TX at zero;
- the operator issued one explicit START;
- the 8-second fresh ECU-only S0 check passed before TX;
- all 30 authoritative TX steps and all 27 response gates passed;
- all three responses were calculated from their current live challenges;
- the complete required S3 producer sets were detected;
- the 300-second passive phase completed with 40,988 received frames;
- every passive report recorded SmartCraft application TX at zero;
- terminal result was `GATE 2 PASS`.

`verify_against_authority.py` independently reports PASS: 30 TX, 27 gates, three transforms, S3 masks `0x807F`/`0x9FFF`, 300 seconds and no errors. This historical run proves the isolated Gate 2 harness on the tested ECU; final validation of the accepted Option 3A firmware is recorded separately below.

## Accepted Option 3A physical validation

Option 3A firmware/software is **ACCEPTED / FROZEN** at commit `06e6ab66588173c0f9072d2bfa5940969a2d0ba6`. The exact baseline completed physical existing-S3 bench validation and final testing against a real Mercury ECU in the boat.

The real-ECU/boat test covered:

1. Boat ignition on before CraftBridge: CraftBridge started correctly, received SmartCraft data and served the Web interface.
2. CraftBridge on before boat ignition: CraftBridge handled the ECU/SmartCraft network becoming active later and received data correctly.
3. Boat ignition on, CraftBridge on, ignition off and ignition on again: current SmartCraft traffic disappeared at ignition off, all signal values became stale and the main page showed `No current data`; current data returned automatically after ignition on.

The third scenario verified loss and return of current data. It did not test creation of a new standalone session after complete ECU power loss.

Observed on the real ECU:

- all six SmartCraft signals produced plausible/correct values;
- numeric oil pressure, Fuel used and Trip time operated correctly;
- the eight-card main Web UI and `/diagnostics` operated correctly;
- S3 was detected;
- the observed external-session test remained at application TX disabled, count 0;
- CAN missed/overrun/bus remained `0/0/0`.

Further firmware changes require a concrete verified defect. Hardware development and hardware acceptance continue separately; the complete Option 3A hardware product is therefore not released with this firmware/software release.

## Session safety

The complete verified startup conversation is documented in [SmartCraft session authorization handshake](../../../SMARTCRAFT_SESSION_AUTHORIZATION.md); this section records implementation and test behavior without duplicating its 30-step table.

After reset the product performs no application TX while observing fresh ECU-only S0 for eight seconds. If a complete fresh external S3 exists, it remains passive and records an external session. Otherwise startup is permitted only when the complete S0 baseline is present and no unexpected non-RTR traffic was observed.

Startup uses exactly 30 TX steps, 27 response gates, three live big-endian transforms and the authoritative 250 ms gate timeout. After the final gate, application TX is disabled before S3 confirmation. Wrong traffic, timeout or send failure enters terminal `StartupFailed`. Sustained S3 loss enters terminal `SessionLost`. Neither state automatically replays startup. A deliberate reset or a new ignition lifecycle is required.

## Signals and sources

All mappings are controlled by `SMARTCRAFT_INPUT_CONTRACT.md` version 1.3.0. Runtime status is stated separately:

| Field | Source | Conversion |
|---|---|---|
| RPM | `0x170/00 D2:D3` u16be | raw RPM |
| Engine temperature | `0x1A0/07 D3` | raw °C |
| Engine hours | `0x1A0/02 D4:D5` u16be | minutes / 60 |
| ECU-reported oil pressure | `0x1A0/05 D4:D5` u16be | raw × 0.01 kPa |
| Battery voltage | `0x1A0/09 D5:D6` u16be | raw × 0.001 V |
| Fuel flow | `0x170/01 D2:D3` u16be | raw × 0.01 l/h |

The numeric oil-pressure mapping is implemented in the accepted firmware and displayed in kPa. On this ECU the numeric value is switch-derived and is not measured analog hydraulic pressure.

## Web interface

The ESP32 starts the read-only access point `CraftBridge-Motor` and serves:

- `http://192.168.4.1/` — responsive eight-card engine dashboard;
- `http://192.168.4.1/diagnostics` — session, CAN and signal diagnostics;
- `http://192.168.4.1/api/status` — normalized JSON status.

The main page reports the six current engine values plus boot-scoped Fuel used and Trip time. Stale or missing engine signals display `No current data`. The diagnostics page reports session and S3 state, startup result, CAN counters/errors, application-TX state/count, signal validity/freshness/age, firmware and uptime. The interface exposes no raw CAN bridge or transmit command.

## Build and verification

```powershell
pio run -e engine-node-s3-n16r8
pio test -e native
py test/tools/verify_motor_node_authority.py --authority "<FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE>"
```

The Windows validation workstation has no GCC, so the deterministic native suite is additionally compiled with Zig C++17 and `-Wall -Wextra -Werror`. Tests cover current runtime decoder/scaling, malformed/RTR rejection, signal quality/freshness, numeric oil pressure, boot-scoped metrics, transforms, safe S0/no-TX, external S3/no-TX, exact sequence hash, full-S3 activation, 250 ms timeout, TX failure, no recovery replay, new ignition lifecycle and simulated HTML/JSON output.

The firmware target is ESP32-S3 N16R8 with 16 MB QIO flash, 8 MB OPI PSRAM, GPIO4 TX, GPIO5 RX and 250 kbit/s TWAI. SmartCraft uses an external bidirectional 3.3 V CAN transceiver, providing physical RX and restricted TX. Do not add another 120 ohm terminator when connecting to an existing correctly terminated SmartCraft network; an isolated bench CAN segment must be terminated at both physical ends.

## Arduino IDE build and upload

Open `firmware/CraftBridge_Motor_Node_Web_v1_0_0/CraftBridge_Motor_Node_Web_v1_0_0.ino` in Arduino IDE. The validated workstation configuration is:

| Arduino IDE option | Exact setting |
|---|---|
| ESP32 board package | `esp32 by Espressif Systems 3.3.11` |
| Board | `ESP32S3 Dev Module` |
| Flash Size | `16MB (128Mb)` |
| Flash Mode | `QIO 80MHz` |
| PSRAM | `OPI PSRAM` |
| Partition Scheme | `Custom` (uses sketch-local `partitions.csv`) |
| USB Mode | `Hardware CDC and JTAG` |
| USB CDC On Boot | `Disabled` |
| Upload Mode | `UART0 / Hardware CDC` |
| Upload Speed | `921600` (installed package default) |
| Upload Port | `COM9` |
| Serial Monitor baud | `115200` |

`WiFi`, `WebServer` and TWAI support are supplied by that ESP32 board package; no third-party Arduino library is required. The installed Arduino CLI 1.5.1 compiles the sketch with the same board options. PlatformIO remains pinned to `espressif32@6.10.0` / Arduino-ESP32 2.0.17 and compiles the same sketch sources. Its board default upload speed is 460800; upload speed affects the loader only, not firmware behavior. Both environments use the authoritative sketch-local `partitions.csv`, which matches the former PlatformIO `default_16MB.csv` layout.

For a controlled upload, disconnect SmartCraft CAN or keep the bench CAN transceiver isolated, connect the Bench Motor Node on COM9, select the settings above, choose **Sketch > Upload**, then open Serial Monitor at 115200 baud. The expected boot prefix is `CraftBridge-Motor-Node/1.0.0` followed by `SAFE IDLE: SmartCraft application TX = 0`. The accepted Option 3A baseline has completed its physical bench and real-ECU/boat validation.

## Desktop ECU simulator physical CANable gate

Status: `ECU_SIMULATOR_PHYSICAL_CANABLE_GATE=PASS`.

Direct evidence: `test/tools/ecu_simulator/test_runs/ecu_sim_20260825_142935`.

The run artifacts establish:

- physical non-dry-run CANable operation on COM3 at serial baud 115200;
- LAWICEL/SLCAN communication and bidirectional 250 kbit/s CAN traffic;
- fresh-startup completion with exactly 30 accepted Motor Node TX steps and 27 response gates;
- transition to sustained complete S3 simulation;
- no unexpected Motor Node TX and no simulator failure;
- exact commands `START`, `PAUSE rpm`, `RESUME rpm`, `PAUSE s3`, and `STOP`;
- zero RPM-page transmission during the 23.61-second pause and resumed RPM transmission 47 ms after `RESUME rpm`;
- controlled `STOP`, terminal phase `stopped`, and result `PASS`.

The simulator evidence folder does not contain a Motor Node serial log, web/API capture or screenshot. Motor Node S3 status, correct web values and web stale/fresh presentation were observed by the operator during the run, but are not independently replay-verifiable from this folder alone.

That evidence run used a different Motor Node flash image and remains bounded to that image. The accepted Option 3A baseline was subsequently validated in a separate physical existing-S3 bench test and does not inherit evidence from this older run.

## Reproducible package gates

Development and fault isolation stay on the bench; boat/real-ECU work is final acceptance only. The accepted Option 3A firmware baseline completed both physical bench and final real-ECU/boat validation.

Arduino IDE is the user-facing firmware environment. The authoritative source is the visible `.ino`, `.h` and `.cpp` set in `firmware/CraftBridge_Motor_Node_Web_v1_0_0/`. PlatformIO automation and native host tests point to that same directory and compile the same source; there is no separate implementation. Firmware/software is accepted and frozen at commit `06e6ab66588173c0f9072d2bfa5940969a2d0ba6`.

The desktop ECU simulator is `SOFTWARE_VERIFIED` and `ECU_SIMULATOR_PHYSICAL_CANABLE_GATE=PASS` for evidence run `ecu_sim_20260825_142935`. That result remains bounded to its tested CANable/bench configuration; physical acceptance of the current firmware baseline is established by the separate current-baseline bench and real-ECU/boat tests recorded above.
