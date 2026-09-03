# CraftBridge Option 3A — Motor Node A/Web

## Release status

- Firmware/software: **Released**
- Firmware baseline: `06e6ab66588173c0f9072d2bfa5940969a2d0ba6`
- Bench validation: **PASS**
- Real-ECU/boat validation: **PASS**
- Hardware platform: **Not released; shared platform in development**

Option 3A establishes or reuses the verified SmartCraft session, decodes six allowlisted engine inputs and exposes normalized data through a local Wi-Fi Web UI, a diagnostics page and JSON status output. After the verified startup sequence, application transmission is disabled and operation is passive.

## Release contents

| Path | Contents |
|---|---|
| [`firmware/`](firmware/README.md) | Buildable Motor Node A/Web Arduino source |
| [`platformio.ini`](platformio.ini) | Pinned ESP32-S3 and native-test build configuration |
| [`test/`](test/README.md) | Host tests, ECU simulator, physical Gate 2 harness and authority verifier |
| [`docs/`](docs/README.md) | SmartCraft contract, session details, architecture and validated build/test status |
| [`hardware/`](hardware/README.md) | Status of the shared, not-yet-released hardware platform |

## Supported engine data

- Engine RPM
- Engine temperature
- Engine runtime
- ECU-reported numeric oil pressure
- Battery/ECU supply voltage
- Instantaneous fuel flow
- Boot-relative fuel used and trip time derived by the firmware

The exact mappings and evidence boundaries are defined in the [SmartCraft Input Contract 1.3.0](docs/SMARTCRAFT_INPUT_CONTRACT.md).

## Build and validation

Open the sketch at `firmware/CraftBridge_Motor_Node_Web_v1_0_0/CraftBridge_Motor_Node_Web_v1_0_0.ino` in Arduino IDE, or run from this `releases/3A` directory:

```powershell
pio run -e engine-node-s3-n16r8
pio test -e native
py -m unittest discover -s test/tools/ecu_simulator/tests -v
```

Exact tool versions, board settings, safety gates and validation boundaries are documented in [Implementation, build, and test status](docs/build-and-test.md).

## Compatibility and safety

The published SmartCraft behavior is verified on an approximately 2006-model-year Mercury 40 EFI FourStroke in the ECM-555/PCM-555 family. It is not a universal Mercury specification.

CraftBridge provides no arbitrary CAN transmission, raw-frame bridge, fuzzing, diagnostics, calibration, configuration or engine-control passthrough. Use the verified startup state machine and documented hardware constraints only.
