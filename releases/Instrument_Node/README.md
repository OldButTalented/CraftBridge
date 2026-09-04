# CraftBridge Instrument Node

The CraftBridge Instrument Node is currently under development.

Planned functionality:

- Receive normalized engine data from the Motor Node via ESP-NOW.
- Show selected engine values on a local display.
- Send supported engine data to an NMEA 2000 network.
- Use one common Instrument Node firmware for both display and NMEA 2000 output.
- Allow display values to be selected in `Config.h`.
- Support one or more Instrument Nodes paired with the same Motor Node.

Current status:

- ESP-NOW reception: physically verified.
- OLED display output: physically verified.
- NMEA 2000 output: under development.
- Garmin/ECHOMAP validation: pending.
- Dynamic Web-based configuration and pairing: planned for a later version.

This firmware is not yet released.
