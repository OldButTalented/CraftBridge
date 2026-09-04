# CraftBridge Node Identity

Use this tool to find the MAC addresses needed for ESP-NOW pairing.

## Pairing

1. Flash the identity tool to the **Motor Node**.
   - Note the **SoftAP MAC**.

2. Flash the identity tool to the **Instrument Node**.
   - Note the **STA MAC**.

3. Enter the **Motor Node SoftAP MAC** in the **Instrument Node `Config.h`**.

4. Enter the **Instrument Node STA MAC** in the **Motor Node `Config.h`**.

5. Compile and flash the normal Motor Node and Instrument Node firmware.

## In short

```text
Motor Node SoftAP MAC
        ↓
Instrument Node Config.h

Instrument Node STA MAC
        ↓
Motor Node Config.h