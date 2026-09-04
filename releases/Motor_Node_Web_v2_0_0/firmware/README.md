# Motor Node Web v2.0.0 firmware

[`CraftBridge_Motor_Node_Web_v2_0_0/`](CraftBridge_Motor_Node_Web_v2_0_0/) is the frozen Arduino sketch from development firmware baseline `3a37f6e1603241fe8cd43c758aa4d37e80c8508d`.

Open [`CraftBridge_Motor_Node_Web_v2_0_0.ino`](CraftBridge_Motor_Node_Web_v2_0_0/CraftBridge_Motor_Node_Web_v2_0_0.ino) directly in Arduino IDE. The sketch folder and `.ino` file retain the same name so Arduino IDE treats the package correctly.

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

The first ESP-NOW iteration uses compile-time peer MAC configuration in `Config.h`. Configure the peer deliberately for the target Instrument Node before deployment. The Wi-Fi/ESP-NOW channel remains fixed at `6` unless a separately validated firmware change is made.