# Option 3A firmware

[`CraftBridge_Motor_Node_Web_v1_0_0/`](CraftBridge_Motor_Node_Web_v1_0_0/) is the released Motor Node A/Web Arduino sketch from firmware baseline `06e6ab66588173c0f9072d2bfa5940969a2d0ba6`.

Open [`CraftBridge_Motor_Node_Web_v1_0_0.ino`](CraftBridge_Motor_Node_Web_v1_0_0/CraftBridge_Motor_Node_Web_v1_0_0.ino) directly in Arduino IDE. The sketch folder and `.ino` file retain the same name so Arduino IDE treats the package correctly.

## Validated Arduino IDE settings

| Arduino IDE option | Exact setting |
|---|---|
| ESP32 board package | `esp32 by Espressif Systems 3.3.11` |
| Board | `ESP32S3 Dev Module` |
| Target memory | ESP32-S3 N16R8: 16 MB flash and 8 MB PSRAM |
| Flash Size | `16MB (128Mb)` |
| Flash Mode | `QIO 80MHz` |
| PSRAM | `OPI PSRAM` |
| Partition Scheme | `Custom` (uses sketch-local `partitions.csv`) |
| USB Mode | `Hardware CDC and JTAG` |
| USB CDC On Boot | `Disabled` |
| Upload Mode | `UART0 / Hardware CDC` |
| Upload Speed | `921600` |
| Serial Monitor baud | `115200` |

The 16 MB flash selection is required. The previous 4 MB selection caused boot failure on the validated N16R8 target.

`../platformio.ini` builds the same source files with the pinned ESP32-S3 N16R8 configuration.
