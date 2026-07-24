# Safety and ECU Protection

## Controlling rule

The engine-side node must be physically receive-only. Software configuration alone is insufficient.

## Defense layers

### Layer 1 — no transmit conductor

- ESP32 TWAI TX is not connected to the SN65HVD230.
- No alternate GPIO, jumper, test pad, header or solder option reaches transceiver D/TXD.
- Firmware routes no GPIO to CAN TX; any API-required TWAI TX GPIO is unused and physically unconnected.

### Layer 2 — fixed recessive driver command

- SN65HVD230 D/TXD is hard-wired to 3.3 V.
- TI defines HIGH as recessive and LOW as dominant.
- This fixed hardware state remains independent of ESP32 boot, reset, watchdog, flash corruption or task failure.

### Layer 3 — listen-only controller mode

- ESP32 TWAI is configured listen-only.
- This prevents acknowledgements and active error frames at the controller level.
- It is defense in depth; it does not replace layers 1 and 2.

### Layer 4 — no added SmartCraft termination

- No 120 ohm resistor is fitted on the engine node.
- Avoid transceiver breakout boards with hidden or soldered termination.
- Measure loading before and after connection.

### Layer 5 — electrical-domain separation

- SmartCraft CAN-H/L never connect to NMEA CAN-H/L.
- The nodes exchange normalized values only through ESP-NOW.
- A helm/NMEA fault cannot become a SmartCraft CAN transmit path.

## Power protection

Both nodes use:

```text
12 V -> fuse -> protected buck converter -> 5 V -> ESP32 VIN
```

Required design review covers:

- Source identification and polarity.
- Fuse close to the power source.
- Wire gauge and fault-current capacity.
- Reverse-polarity protection.
- Vessel transients/load dump and buck input rating.
- Brownout behavior and transceiver unpowered loading.
- Ground/reference and possible galvanic-isolation needs.

SmartCraft switched +12 V pinout is **Unknown until physically verified** for the chosen harness. Do not infer it from connector position or wire color alone.

## CAN protection

- Select CAN-line TVS components from datasheet limits and measured bus conditions.
- Do not add capacitance or a common-mode choke without signal-integrity review.
- Keep SmartCraft tap stub short.
- Validate common-mode range, recessive level and unpowered behavior.
- Use strain relief and prevent CAN-H/CAN-L/12 V cross-shorts in connectors.

## Prototype NMEA segment safety

- Dedicated segment only; do not connect it to the boat's SmartCraft bus.
- Exactly two 120 ohm terminators.
- External fused power on `NET-S`/`NET-C`; Garmin is not the network power source.
- Verify polarity and approximately 60 ohms across CAN-H/CAN-L before power.
- Keep the segment short and use twisted CAN conductors.
- Use a separate protected local supply branch for the helm node unless a reviewed schematic explicitly combines power paths.

## Data safety

- Production output allowlist contains only inputs authorized by [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md).
- Candidate, Strong, Weak or Unknown mappings are never presented as facts to Garmin.
- Stale values become unavailable/suppressed, not zero.
- Plausibility checks must reject malformed and out-of-range data without retaining false freshness.
- Engine and device instances must be explicit to prevent data being attributed to the wrong engine.

## Hazard analysis

| Hazard | Prevention/detection | Safe response |
|---|---|---|
| Engine node transmits on SmartCraft | TX physically absent; D/TXD fixed HIGH; listen-only mode; oscilloscope proof | Disconnect node; design fails gate |
| Extra SmartCraft termination | No resistor/BOM rule; resistance measurement | Do not connect until removed |
| 12 V reaches logic rail | Connector verification, fuse, protected buck, polarity check | Fuse opens/current limit; redesign before reuse |
| Wireless loss shown as valid engine data | Sequence, CRC, per-signal age, stale gate | Suppress/use unavailable encoding |
| NMEA segment unpowered or mis-terminated | NET-S/NET-C and 60-ohm preflight checks | Disable helm CAN TX |
| Garmin and gateway both assumed to power network | Explicit external power injection and wiring review | Remove power until one reviewed source remains |
| Unauthorized SmartCraft field emitted | Contract-based compile-time allowlist and mapping tests | Withhold field |
| Cross-domain wiring error | Separate connectors, net names, continuity test | No power; correct harness |

## Installation stop conditions

Stop and disconnect if any of these occur:

- SmartCraft warnings, communication loss or unexpected device behavior.
- Termination/loading change outside reviewed tolerance.
- Engine node D/TXD not continuously HIGH.
- CAN errors increase after connection.
- Supply rail exceeds limits, resets repeatedly or overheats.
- NMEA network polarity/voltage is uncertain.
- Documentation and actual wiring disagree.

## Approval gates

1. Architecture and schematic review.
2. Physical RX-only bench proof.
3. Replay/simulation decoder proof.
4. ESP-NOW fault testing.
5. Dedicated NMEA segment and Garmin bench proof.
6. Explicit approval for live passive SmartCraft connection.
7. Separate explicit approval for live end-to-end NMEA output.

Passing one gate does not authorize skipping the next.

