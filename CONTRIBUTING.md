# Contributing to CraftBridge

Thank you for your interest in CraftBridge.

CraftBridge is currently a small, independently managed engineering project. Contributions are welcome when they improve the technical quality, reproducibility or practical usefulness of the project.

## Project scope

Contributions must relate directly to the CraftBridge architecture:

- Passive SmartCraft CAN reception
- Verified SmartCraft signal decoding
- ESP32 firmware
- ESP-NOW communication
- NMEA 2000 output
- Garmin/ECHOMAP interoperability
- Hardware, wiring, protection and testing
- Original CAN captures and independently derived observations

The following are outside the scope of this repository:

- CDS/DDT service-port work
- Diacom analysis
- Diagnostic protocol reverse engineering
- ECU programming, configuration or fault clearing
- Proprietary Mercury software, firmware or databases
- Copyrighted service manuals or licensed standards
- Decompiled third-party source code

## Evidence status

Technical claims must use one of these status labels:

- **Verified** — repeatedly confirmed by reproducible evidence
- **Candidate** — plausible and selected for further testing
- **TBD** — not yet decided or insufficiently tested
- **Rejected** — tested or reviewed and not accepted

Do not present reverse-engineered information as an official Mercury, Garmin or NMEA specification.

A SmartCraft source mapping and its NMEA 2000 destination must be verified independently.

## Safety requirements

The engine-side SmartCraft interface must remain physically receive-only.

Contributions must not:

- Connect an ESP32 transmit GPIO to the SmartCraft CAN transceiver
- Enable SmartCraft CAN transmission or acknowledgement
- Add termination to the existing SmartCraft network
- Electrically connect SmartCraft CAN-H/CAN-L to NMEA CAN-H/CAN-L
- Emit Candidate, TBD or Unknown SmartCraft values as valid production data
- Replace stale or invalid data with zero
- Bypass the staged test and approval process

Any proposed change that affects these rules requires an explicit architecture and safety review.

## Reporting observations

When reporting CAN data, hardware results or interoperability findings, include where possible:

- Engine model and year
- Test conditions and engine state
- Hardware and firmware versions
- CAN bitrate
- Capture tool and configuration
- Raw capture or minimal reproducible sample
- Expected result
- Observed result
- Number of repetitions
- Evidence status

Remove personal information, serial numbers and other identifiers unless they are necessary and intentionally shared.

## Code and document contributions

Keep changes focused and easy to review.

A contribution should:

- Address one subject at a time
- Explain the purpose of the change
- Preserve existing scope and safety rules
- Include tests or reproducible evidence where relevant
- Update related documentation
- Avoid unrelated formatting changes
- Avoid adding large binary files without prior discussion

## Pull requests

Before submitting a pull request:

1. Review the current architecture, safety and test documents.
2. Confirm that the change is within repository scope.
3. Test the change using replay, simulation or bench equipment where possible.
4. Describe the test method and result.
5. Identify remaining limitations or unknowns.

Pull requests may be accepted, modified, deferred or declined at the maintainer's discretion.

A technically interesting contribution may remain classified as **Candidate** until it can be reproduced independently.

## Issues

Use issues for:

- Reproducible faults
- New SmartCraft observations
- Garmin interoperability results
- Hardware design concerns
- Documentation corrections
- Proposed features within project scope

For general questions, first check the README, architecture, wiring, mapping and test documents.

## Licensing

By submitting a contribution, you agree that it may be distributed under the repository's Apache License 2.0.

Only submit material that you created yourself or are legally permitted to contribute.

Do not submit confidential, proprietary, copyrighted or license-restricted third-party material.

## Conduct

Keep discussion technical, factual and respectful.

Disagreement is acceptable. Unsupported certainty is not.

The objective is reproducible engineering, not consensus by assertion.
