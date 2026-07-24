# Test Plan

## Test policy

- Do not start with a live engine bus.
- SmartCraft is receive-only at every stage.
- NMEA transmission is tested first on the dedicated bench segment with simulated normalized input.
- Each phase has an explicit exit gate. Failure returns the design to review; it does not authorize bypassing protection.
- Preserve captures, firmware version, hardware revision, configuration and results.

## Phase 0 — document and schematic review

### Checks

- Master architecture agrees with schematic and wiring list.
- Engine-side ESP32 TX has no physical route to SN65HVD230 D/TXD.
- D/TXD is hard-wired HIGH/recessive.
- No SmartCraft termination is present.
- MCP2562 VDD is 5 V and VIO is 3.3 V.
- NMEA segment has two and only two 120 ohm terminators.
- All three 12 V branches have documented fusing: engine node, helm node and NMEA network power.

### Exit gate

Independent schematic/PCB review completed with all TBD electrical values resolved.

## Phase 1 — unpowered continuity and resistance

1. Verify open circuit between every ESP32 engine-side TX-capable GPIO and SN65HVD230 D/TXD.
2. Verify D/TXD is connected to 3.3 V according to the reviewed schematic.
3. Verify the engine node alone does not place 120 ohms across SmartCraft CAN-H/CAN-L.
4. Verify no continuity between SmartCraft CAN conductors and NMEA CAN conductors.
5. Verify approximately 60 ohms across prototype NMEA CAN-H/CAN-L with both terminators installed.
6. Verify `NET-S`/`NET-C` polarity and absence of shorts.

### Exit gate

All readings recorded and within reviewed tolerances.

## Phase 2 — power subsystem bench test

Test each node from a current-limited supply before connecting either CAN bus.

- Validate input polarity and fuse path.
- Sweep the approved input-voltage range.
- Record 5 V and 3.3 V rails at idle and during ESP-NOW bursts.
- Exercise startup, shutdown, brownout and rapid power cycling.
- Confirm transceiver pins remain in safe states throughout sequencing.
- Measure current and buck temperature.

### Exit gate

Stable rails, no unsafe pin excursion and acceptable thermal/current margin.

## Phase 3 — engine-side physical RX-only proof

Use an isolated bench CAN source, not the engine.

1. Configure multiple firmware states: normal, bootloader/reset, watchdog reset and intentionally crashed task.
2. Observe SN65HVD230 D/TXD and CAN-H/CAN-L with an oscilloscope.
3. Confirm D/TXD remains HIGH and the node never creates a dominant bit or ACK.
4. Confirm removal/power loss does not disturb the bench bus.
5. Confirm the transceiver board adds no termination.

### Exit gate

Physical receive-only behavior proven independently of firmware state.

## Phase 4 — decoder replay/simulation

Feed known CAN frames on the isolated bench bus.

[`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md), version 1.0.0, is the sole source of expected SmartCraft decoder definitions.

- Stopped, idle and approximately 2000 RPM cases.
- Engine-temperature values covering the verified reference range.
- Engine-hours minute transitions.
- Wrong ID/page, short DLC, boundary and malformed cases.
- Candidate/Unknown oil, fuel, voltage and load fields.

Expected behavior:

- Contract-authorized inputs decode exactly.
- Candidate, Strong, Weak and Unknown mappings never set valid output bits.
- Malformed frames increment diagnostics without changing the last valid value.
- Values become stale according to the reviewed timeout policy.

## Phase 5 — ESP-NOW protocol

- Verify packet golden vectors, byte order, CRC and exact 48-byte length.
- Verify boot ID and sequence wrap logic.
- Inject duplicates, reordering, loss, corruption and wrong-version packets.
- Power-cycle each node independently.
- Remove RF path or change channel to create link loss, then restore it.
- Confirm SmartCraft RX processing continues while wireless sends fail.
- Confirm helm output becomes stale and recovers only from a new valid snapshot.
- Record range, packet-loss rate and coexistence behavior at the intended installation locations.

## Phase 6 — NMEA 2000 bench segment

With no Garmin connected initially:

1. Verify two terminators and approximately 60 ohms unpowered.
2. Apply fused external network power; verify `NET-S`, `NET-C` and voltage at both ends.
3. Confirm Garmin is not being treated as the power source.
4. Start the helm node with simulated normalized values.
5. Capture NMEA traffic independently.
6. Verify address claim, product information, engine instance, candidate PGNs, field scaling and periods.
7. Inject stale and unavailable states.
8. Measure bus errors and utilization.

### Exit gate

Captured PGNs match injected normalized values and stale behavior; no live SmartCraft input used.

## Phase 7 — Garmin interoperability

- Connect Garmin as the second device on the short segment.
- Confirm network power and Garmin main power follow the reviewed installation.
- Verify device discovery and address claim behavior.
- Verify each candidate PGN's label, units, instance and display page.
- Test helm reboot, Garmin reboot, wireless loss and stale data.
- Confirm no misleading zero values appear during failures.

### Exit gate

Repeated agreement between injected values, captured PGNs and Garmin presentation.

## Phase 8 — live SmartCraft passive receive

Requires explicit approval after phases 0–7.

- First connect the engine node with ESP-NOW and helm NMEA output disabled.
- Confirm measured SmartCraft termination/loading is unchanged.
- Confirm listen-only reception at the independently verified bitrate.
- Compare decoded values with SmartCraft Input Contract v1.0.0 and the preserved upstream evidence.
- Monitor CAN error counters and physical waveforms.
- Disconnect immediately on any bus disturbance, warning or unexpected behavior.

### Exit gate

Passive node coexists without measurable disturbance and reproduces the contract-authorized values.

## Phase 9 — end-to-end live demonstration

Requires a separate explicit approval.

- Enable ESP-NOW and the already bench-verified NMEA mappings.
- Compare engine source, normalized packet, captured NMEA PGN and Garmin display at each reference point.
- Exercise wireless interruption and node resets while stationary and safe.
- Preserve an end-to-end evidence bundle.

## Acceptance evidence

For each test record:

- Date, operator and location.
- Hardware/PCB revision and serial labels.
- Firmware commit/version and configuration hash.
- Supply voltage/current and fuse values.
- CAN wiring/termination measurements.
- CAN and ESP-NOW captures with timestamps.
- Expected versus observed result.
- Pass/fail and follow-up decision.

## Prohibited tests

- Any SmartCraft transmission or ACK test from the engine node.
- Deliberately creating engine faults or low oil pressure.
- Live NMEA output before replay/simulation and Garmin bench verification.
- Bypassing fuses, termination checks or physical RX-only review.

