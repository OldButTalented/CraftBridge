# CraftBridge

CraftBridge is an independent open-source implementation for accessing selected Mercury SmartCraft engine data.

## Current releases

| Artifact | Status | Scope |
|---|---|---|
| [Motor Node Web v2.0.0](releases/Motor_Node_Web_v2_0_0/README.md) | **Released / Frozen** | SmartCraft/Web plus ESP-NOW output for Instrument Node communication |
| [Instrument node](releases/Instrument_Node/README.md)| **Development** | ESP-NOW input, display and NMEA 2000 output |
| [CraftBridge Node Identity](releases/CraftBridge_Node_Identity/README.md) | **Released / Frozen** | Reads ESP32-S3 Base, STA and SoftAP MAC addresses for compile-time ESP-NOW pairing |

## Archived releases

[Motor Node Web v1.0.0](archive/Motor_Node_Web_v1_0_0/README.md) remains available as a frozen historical release and rollback reference. Its existing Git tag and GitHub Release are preserved, but v1.0.0 is no longer the recommended current Motor Node firmware.

## Product status

Motor Node Web v2.0.0 is the current Motor Node firmware. Instrument Node ESP-NOW reception and OLED display are physically validated; Instrument Node NMEA 2000 remains under development.

The shared technical reference for the session startup is [SmartCraft Session Authorization](SMARTCRAFT_SESSION_AUTHORIZATION.md).

The shared hardware platform is not released. Schematics, PCB, CAD, BOM and production files will be published later when complete and validated.
## Choose a path

### Just want one?

Assembled PCBA, flashing and installation material are not available yet because the shared hardware platform remains under development. Use the current published Motor Node firmware only with compatible development hardware and the documented safety constraints.

### Want to build or modify it?

Current Motor Node firmware is available under [`releases/Motor_Node_Web_v2_0_0/`](releases/Motor_Node_Web_v2_0_0/README.md), and the pairing identity tool is under [`releases/CraftBridge_Node_Identity/`](releases/CraftBridge_Node_Identity/README.md). The previous Motor Node v1.0.0 release is retained under [`archive/Motor_Node_Web_v1_0_0/`](archive/Motor_Node_Web_v1_0_0/README.md). Open hardware and production sources will be added after the shared platform is validated.

## Test your Mercury / Add your ECU

Test CraftBridge on your engine and [submit a compatibility report](https://github.com/OldButTalented/CraftBridge/issues/new?template=test-your-mercury.yml). See the maintainer-curated [compatibility list](COMPATIBILITY.md) for reviewed results.

Reports are reviewed before they are added to the official list; submitting an issue does not update it automatically. Compatibility is evidence-based and general Mercury compatibility is not claimed.

## License

CraftBridge uses scope-specific open-source licences for software, hardware and documentation. Commercial use, building and sale remain permitted subject to the applicable terms. See the canonical [licensing overview](LICENSE.md).

## Safety

CraftBridge is experimental, uncertified equipment and must not be the sole source of safety-critical engine information. See the [disclaimer](DISCLAIMER.md).
