# Contributing to CraftBridge

Contributions are welcome when they improve a reproducible standalone SmartCraft-to-NMEA 2000 implementation.

## Useful contributions

- Engine Node or Helm Node firmware with pinned, reproducible builds
- Schematics, protected power design, PCB/layout review, and verified connector information
- Decoder and session-state-machine tests
- Independent SmartCraft captures and compatibility results from other engines
- NMEA 2000 analyzer results and Garmin/ECHOMAP interoperability evidence
- Documentation corrections backed by source code or reproducible observations

## Evidence requirements

Use these labels:

- **Verified:** repeatedly confirmed by reproducible evidence, including physical validation where the claim is physical
- **Candidate:** plausible but not sufficiently validated
- **Unknown:** unresolved
- **Rejected:** tested and not supported

A verified SmartCraft source mapping does not verify its NMEA destination. Do not present reverse-engineered findings as official Mercury, Garmin, or NMEA specifications.

For protocol or compatibility reports, include engine model/year, ECU/software identity if available, topology, CAN bitrate, capture hardware/configuration, test conditions, raw minimal evidence, expected and observed result, and repetition count. Remove serial numbers and personal information unless intentionally required.

## Safety and scope

Contributions must not:

- electrically bridge SmartCraft and NMEA 2000;
- add termination to an already terminated SmartCraft bus;
- add arbitrary SmartCraft transmit, fuzzing, diagnostics, calibration, configuration, or control passthrough;
- use historical challenge responses as constants;
- continue a startup after timeout or unexpected directed traffic;
- emit Candidate or Unknown mappings as valid production values;
- replace stale or unavailable values with zero;
- include proprietary firmware, databases, licensed standards, service manuals, or decompiled third-party source.

Any SmartCraft transmission change requires a narrowly scoped design, explicit expected responses, abort behavior, and captured verification.

## Pull requests

Keep each pull request focused. Explain the change, evidence status, risk, test method, results, and remaining limitations. Update documentation and tests together with implementation changes. Avoid generated files and large binaries unless they are essential and discussed first.

By contributing, you agree that your contribution may be distributed under Apache License 2.0 and confirm that you have the right to submit it. Keep discussion technical, factual, and respectful.
