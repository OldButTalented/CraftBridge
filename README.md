# CraftBridge

CraftBridge is an independent open-source implementation for accessing selected Mercury SmartCraft engine data.

## Variants

| Variant | Firmware/software | Hardware | Status |
|---|---|---|---|
| [Option 3A](releases/3A/README.md) | Released | Shared hardware platform in development | **Released firmware/software** |
| Option 3B | Motor Node v2 released separately; Instrument Node/NMEA 2000 pending | Shared hardware platform in development | **Not a complete release** |
| Option 3C | Motor Node v2 released separately; Instrument Node/NMEA 2000 pending | Shared hardware platform in development | **Not a complete release** |

Option 3A provides the validated Motor Node A firmware/software release: SmartCraft session handling, six verified engine inputs, a local Wi-Fi access point, Web UI, diagnostics and JSON output. It is bench validated and real-ECU/boat validated.

The standalone [CraftBridge Motor Node Web v2.0.0](releases/Motor_Node_Web_v2_0_0/README.md) firmware release preserves the Option 3A SmartCraft/Web behavior and adds ESP-NOW output of normalized engine values. It is physically verified and frozen, and is prepared for Instrument Node pairing/communication. Instrument Node NMEA 2000 remains under development; this does not constitute a complete Option 3B or 3C release.

The shared technical reference for the session startup is [SmartCraft Session Authorization](SMARTCRAFT_SESSION_AUTHORIZATION.md).

The complete 3A hardware product is **not released**. Options 3A, 3B and 3C will use one shared hardware platform, which will be finalized after the 3B and 3C firmware requirements are known. Schematics, PCB, CAD, BOM and production files will be published later when validated.

## Choose a path

### Just want one?

Assembled PCBA, flashing and installation material are not available yet because the shared hardware platform remains under development. Use the published 3A firmware/software now only with compatible development hardware and the documented safety constraints.

### Want to build or modify it?

The complete Option 3A firmware source and reproducible software tests are available under [`releases/3A/`](releases/3A/README.md). Open hardware and production sources will be added after the shared platform is validated. The future hardware release will include standard manufacturing files and will not require a specific PCB manufacturer.

## Test your Mercury / Add your ECU

Test CraftBridge on your engine and [submit a compatibility report](https://github.com/OldButTalented/CraftBridge/issues/new?template=test-your-mercury.yml). See the maintainer-curated [compatibility list](COMPATIBILITY.md) for reviewed results.

Reports are reviewed before they are added to the official list; submitting an issue does not update it automatically. Compatibility is evidence-based and general Mercury compatibility is not claimed.

## License

CraftBridge uses scope-specific open-source licences for software, hardware and documentation. Commercial use, building and sale remain permitted subject to the applicable terms. See the canonical [licensing overview](LICENSE.md).

## Safety

CraftBridge is experimental, uncertified equipment and must not be the sole source of safety-critical engine information. See the [disclaimer](DISCLAIMER.md).
