# Implementation, build, and test status

## What can be built today

There is currently no firmware source, PlatformIO/Arduino project, PCB/CAD package, or released pinout in this repository. Therefore there is no honest end-to-end build or flash command yet. The documents define the implementation target and verified protocol inputs for contributors.

A release must not be described as buildable until it contains at minimum:

- Engine Node and Helm Node source trees;
- pinned framework, toolchain, board, and library versions;
- reproducible build commands;
- example non-secret configuration;
- GPIO/pinout and schematic references;
- unit/replay tests and expected results;
- flash and provisioning instructions.

## Recommended source layout

```text
firmware/
  common/
  engine-node/
  helm-node/
tests/
  decoder/
  smartcraft-session/
  esp-now/
  nmea2000/
hardware/
  schematics/
```

This is guidance, not a statement that these files already exist.

## Implementation requirements

### Engine Node

- Use 250 kbit/s CAN and explicit acceptance filters.
- Implement the 30-step startup as a deterministic response-gated state machine.
- Calculate the three response values from live big-endian challenges.
- Provide no automatic retry loop; return to a safe idle/error state after timeout or unexpected directed traffic.
- Decode only verified ID/page/field combinations with DLC and bounds checks.
- Publish normalized values with validity, source age, boot ID, and sequence.
- Provide no arbitrary raw-frame transmit or diagnostic/control interface.

### Helm Node

- Authenticate/provision the intended ESP-NOW peer and version the packet format.
- Reject malformed, duplicate, stale, old-version, or out-of-window packets.
- Never convert missing or stale data to zero.
- Implement NMEA output only after checking the selected library’s field units and unavailable values.
- Keep SmartCraft and NMEA 2000 electrically isolated.

## Verification sequence

1. **Document and schematic review:** power, pinout, termination, transceiver fail states, and CAN-domain separation.
2. **Unpowered checks:** continuity, shorts, polarity, and approximately 60 ohms across the isolated NMEA segment.
3. **Power bench test:** current limit, rail accuracy, ripple, startup/shutdown behavior, reverse-polarity and transient design review.
4. **Offline decoder tests:** known frames, wrong DLC/page, boundary values, stale values, malformed packets.
5. **Session replay tests:** all 30 steps, three transforms, every gate, unexpected response, timeout, reset, and no-retry behavior.
6. **ESP-NOW tests:** packet corruption, duplicate/loss/reorder, reboot epoch, range, interference, and stale suppression.
7. **NMEA bench test:** analyzer-verified PGNs, units, rate, instance, address behavior, unavailable values, and two-terminator topology.
8. **Garmin/ECHOMAP test:** confirm displayed values and behavior on loss/recovery.
9. **Controlled live test:** verify bus voltage/loading first; log all SmartCraft TX/RX; stop on unexpected traffic.
10. **End-to-end comparison:** compare display values with independent engine references across controlled states.

## Acceptance evidence

A build should not be called supported until its release records:

- exact hardware and firmware revision;
- reproducible build and flash result;
- automated test result;
- SmartCraft session outcome and transmitted-frame audit;
- no unintended SmartCraft transmissions;
- signal comparison and repetition count;
- NMEA analyzer and target-display result;
- stale/loss/recovery behavior;
- remaining limitations.

## Stop conditions

Do not power or connect hardware if connector identity, polarity, CAN-H/CAN-L, termination, common reference, transceiver voltage, or protection is uncertain. Stop testing on abnormal bus voltage, unexpected traffic, ECU/gauge faults, reset loops, overheated components, unstable supplies, or unexplained NMEA behavior.
