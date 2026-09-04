# Contributing to CraftBridge

Contributions are welcome when they improve the reproducible standalone Motor Node A/Web implementation.

## Useful contributions

- Motor Node A/Web firmware with pinned, reproducible builds
- Schematics, protected power design, PCB/layout review, and verified connector information
- Decoder and session-state-machine tests
- Independent SmartCraft captures and compatibility results from other engines
- Documentation corrections backed by source code or reproducible observations

## Controlled SmartCraft input boundary

[SmartCraft Input Contract 1.3.0](archive/Motor_Node_Web_v1_0_0/docs/SMARTCRAFT_INPUT_CONTRACT.md) is the sole authoritative source for concrete SmartCraft input definitions. Contributions may implement its Verified mappings but must not duplicate competing CAN/page/byte definitions elsewhere. A change to the contract requires new verified SmartCraft evidence and a versioned contract revision.

Results from other ECU variants should be reported as separate compatibility evidence; they must not weaken or silently generalize the tested-ECU baseline.

## Evidence requirements

Use these labels:

- **Verified:** repeatedly confirmed by reproducible evidence, including physical validation where the claim is physical
- **Candidate:** plausible but not sufficiently validated
- **Unknown:** unresolved
- **Rejected:** tested and not supported

Do not present reverse-engineered findings as official Mercury specifications.

For protocol or compatibility reports, include engine model/year, ECU/software identity if available, topology, CAN bitrate, capture hardware/configuration, test conditions, raw minimal evidence, expected and observed result, and repetition count. Remove serial numbers and personal information unless intentionally required.

## Safety and scope

Contributions must not:

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

By contributing, you confirm that you have the right to submit the contribution and agree that it may be distributed under the licence applicable to its scope: software, firmware, tools and tests under `GPL-3.0-or-later`; hardware under `CERN-OHL-S-2.0`; and documentation under `CC-BY-SA-4.0`. Third-party content retains its own licence. See the canonical [licensing overview](LICENSE.md). Keep discussion technical, factual, and respectful.
