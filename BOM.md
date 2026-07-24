# Bill of Materials

## Status legend

- **Selected architecture** — part family required by the objective; exact ordering code may remain TBD.
- **Candidate** — requires electrical/environmental review.
- **TBD** — requirement known, component not selected.

## Core electronics

| Qty | Item | Use | Status / notes |
|---:|---|---|---|
| 2 | ESP32-WROOM development board | One per node | Selected architecture; exact board revision and GPIO exposure TBD |
| 1 | SN65HVD230 CAN transceiver | Engine-side passive SmartCraft receiver | Selected architecture; 3.3 V supply; D/TXD hard-wired recessive |
| 1 | MCP2562 CAN transceiver | Helm-side NMEA transmitter/receiver | Selected architecture; VDD 5 V, VIO 3.3 V |
| 2 | 12 V-to-5 V buck converter | One per ESP32 node | Candidate; input transients, current, ripple and marine suitability TBD |
| 2 | Local fuse and holder | One per node supply branch | Rating and type TBD |
| 1 | NMEA network fuse and holder | External `NET-S` power injection | Rating per final network load and Garmin guidance TBD |

## CAN and NMEA interconnect

| Qty | Item | Use | Status / notes |
|---:|---|---|---|
| 2 | 120 ohm NMEA 2000 terminator | One at each physical end | Required; exactly two total |
| 1 set | NMEA 2000-compatible T/connectors or labelled prototype harness | Gateway/Garmin/network power | Exact topology and connector gender TBD |
| As required | Twisted CAN pair | Dedicated CAN-H/CAN-L | Length intentionally short; gauge/type TBD |
| As required | `NET-S` / `NET-C` conductors | Network power | Gauge and color per chosen connector/harness standard TBD |
| 1 | SmartCraft sacrificial inline tap harness | Non-destructive passive connection | Existing project preference; exact connector/pinout must be verified |

## Protection and implementation

| Qty | Item | Status / notes |
|---:|---|---|
| 2 sets | Reverse-polarity and supply-transient protection | TBD |
| 2 sets | CAN-line TVS protection | TBD after capacitance/common-mode review |
| 2 sets | IC decoupling capacitors | Required; values/layout from datasheets |
| Optional | Common-mode choke | Candidate only after signal-integrity review |
| 2 | Enclosure with strain relief | TBD; moisture, vibration and serviceability review required |
| As required | Waterproof connectors and wiring | TBD |
| 2 | Status LED or service indicator | Candidate; must not compromise safety or sealing |
| 2 | Programming/service connector | Candidate; engine-side connector must expose no CAN TX path |

## Procurement warnings

- Many SN65HVD230 breakout modules include a 120 ohm termination resistor and expose TXD. Such a board is not acceptable on SmartCraft unless termination is verifiably absent and TXD is permanently isolated/hard-recessive.
- Generic buck modules and ESP32 development boards are not automatically marine-, transient- or ignition-protected.
- Verify actual MCP2562 rather than MCP2561 when VIO is required.
- Do not select fuse values until measured load, wire gauge and fault-current path are known.

## Not yet included

- PCB fabrication and assembly.
- NMEA 2000 certification/testing costs.
- Licensed NMEA documentation or commercial firmware libraries.
- Installation hardware specific to the boat and Garmin model.

