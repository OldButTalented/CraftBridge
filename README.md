# CraftBridge

CraftBridge is an independent open-source implementation for accessing selected Mercury SmartCraft engine data.

## Variants

| Variant | Firmware/software | Hardware | Status |
|---|---|---|---|
| [Option 3A](releases/3A/README.md) | Released | Shared hardware platform in development | **Released firmware/software** |
| Option 3B | Not released | Shared hardware platform in development | **Planned / coming later** |
| Option 3C | Not released | Shared hardware platform in development | **Planned / coming later** |

Option 3A provides the validated Motor Node A firmware/software release: SmartCraft session handling, six verified engine inputs, a local Wi-Fi access point, Web UI, diagnostics and JSON output. It is bench validated and real-ECU/boat validated.

The complete 3A hardware product is **not released**. Options 3A, 3B and 3C will use one shared hardware platform, which will be finalized after the 3B and 3C firmware requirements are known. Schematics, PCB, CAD, BOM and production files will be published later when validated.

## Choose a path

### Just want one?

Assembled PCBA, flashing and installation material are not available yet because the shared hardware platform remains under development. Use the published 3A firmware/software now only with compatible development hardware and the documented safety constraints.

### Want to build or modify it?

The complete Option 3A firmware source and reproducible software tests are available under [`releases/3A/`](releases/3A/README.md). Open hardware and production sources will be added after the shared platform is validated. The future hardware release will include standard manufacturing files and will not require a specific PCB manufacturer.

## Compatibility

SmartCraft behavior is verified only on the documented test platform. General Mercury compatibility is not claimed. A standardized **Test your Mercury / Add your ECU** workflow and evidence-based positive compatibility list will be added later.

## License and safety

Repository content is provided under the [Apache License 2.0](LICENSE). CraftBridge is experimental, uncertified equipment and must not be the sole source of safety-critical engine information. See the [disclaimer](DISCLAIMER.md).
