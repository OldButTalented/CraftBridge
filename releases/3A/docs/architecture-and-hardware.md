# Architecture and hardware

This document records the validated development-hardware boundary for the released Option 3A firmware/software. [SMARTCRAFT_INPUT_CONTRACT.md version 1.3.0](SMARTCRAFT_INPUT_CONTRACT.md) remains authoritative for the six normalized SmartCraft inputs. Production hardware is not part of this release; see [Option 3A hardware status](../hardware/README.md).

## System architecture

```text
SmartCraft CAN <-> external 3.3 V CAN transceiver <-> ESP32-S3 Motor Node A <-> Wi-Fi/Web UI
```

Motor Node A connects directly to SmartCraft CAN through an external bidirectional 3.3 V CAN transceiver. It receives and decodes SmartCraft traffic, transmits only the exact response-gated session startup sequence when required, and then returns to passive receive operation. It exposes normalized engine data and diagnostics through its local Wi-Fi access point and read-only Web UI.

CraftBridge is not a transparent CAN bridge, diagnostic interface or engine-control interface. The firmware provides no arbitrary CAN transmission, raw-frame forwarding, fuzzing, calibration or control passthrough.

## Implemented hardware

| Component | Current configuration |
|---|---|
| MCU/module | ESP32-S3 N16R8 |
| Arduino board type | `ESP32S3 Dev Module` |
| Flash | 16 MB, QIO |
| PSRAM | 8 MB, OPI |
| SmartCraft CAN TX | GPIO4 |
| SmartCraft CAN RX | GPIO5 |
| SmartCraft bitrate | 250 kbit/s |
| CAN physical layer | External bidirectional 3.3 V CAN transceiver |
| User interface | Local Wi-Fi access point, HTTP Web UI and JSON status endpoint |

The repository does not identify a verified CAN-transceiver part number. Hardware documentation must therefore use the generic description **external 3.3 V CAN transceiver**.

The authoritative firmware is `firmware/CraftBridge_Motor_Node_Web_v1_0_0/`. Pin assignments and bitrate are defined in `Config.h`; `CanBus.cpp` initializes ESP32 TWAI in normal mode with the 250 kbit/s timing preset. `platformio.ini` selects the ESP32-S3 target, 16 MB flash and OPI PSRAM.

## SmartCraft connection and termination

The CAN transceiver provides physical RX and TX between the ESP32-S3 TWAI controller and SmartCraft CAN. TX is required only for the verified [SmartCraft session authorization handshake](../../../SMARTCRAFT_SESSION_AUTHORIZATION.md).

CraftBridge does not add another 120 ohm terminator when connected as a node or tap on an existing, correctly terminated SmartCraft network. An isolated bench CAN segment must instead be terminated correctly at both physical ends before power is applied.

Before connection, verify CAN-H, CAN-L, common ground, transceiver supply and polarity against the actual harness. The repository does not define a public connector part number or pinout.

## Session and fail-safe behavior

After reset, Motor Node A observes fresh ECU-only S0 traffic for eight seconds without application TX. If complete fresh S3 traffic already exists, it remains passive. Otherwise it may execute the exact 30-step startup sequence with 27 response gates and three live challenge calculations.

After the final gate, application TX is disabled before S3 confirmation. Timeout, unexpected directed traffic, payload mismatch, malformed challenge, transmit failure or later S3 loss disables application TX without automatic replay. Recovery requires a deliberate reset or new ignition lifecycle.

## Build and test boundary

The same source is compiled by Arduino IDE and PlatformIO and exercised by the native host tests. Option 3A firmware/software is accepted and frozen at commit `06e6ab66588173c0f9072d2bfa5940969a2d0ba6` after physical bench and real-ECU/boat validation of that baseline.

Hardware development and hardware acceptance remain a separate active workstream. This firmware/software release therefore does not claim that the complete Option 3A hardware product is released.

Exact build settings, test commands and evidence boundaries are documented in [Implementation, build, and test status](build-and-test.md).
