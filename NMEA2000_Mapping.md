# NMEA 2000 Mapping

## Evidence rule

This document is authoritative for NMEA destination selection, PGN decisions and Garmin verification status. It does not define SmartCraft CAN inputs.

- [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md), version 1.0.0, is the sole authority for concrete SmartCraft input definitions.
- The choice of NMEA PGN, field, units, rate, instance and Garmin presentation remains **Candidate** until independently tested.
- Signals not authorized by the current SmartCraft input contract are withheld from production NMEA output.

## Initial mapping matrix

| Information | SmartCraft source status | Normalized field | Candidate NMEA destination | Gateway status |
|---|---|---|---|---|
| Engine speed | Authorized by SmartCraft Input Contract v1.0.0 | `rpm` | PGN 127488 Engine Parameters, Rapid Update | Candidate; replay and Garmin verification required |
| Engine temperature | Authorized by SmartCraft Input Contract v1.0.0 | `engine_temp_centi_c` | PGN 127489 Engine Parameters, Dynamic | Candidate; exact temperature field/library API and Garmin presentation TBD |
| Engine runtime | Authorized by SmartCraft Input Contract v1.0.0 | `engine_runtime_minutes` | PGN 127489 Engine Parameters, Dynamic | Candidate; source-instance choice and unit conversion TBD |
| Oil pressure | Not authorized by SmartCraft Input Contract v1.0.0 | Not present in v1 packet | PGN 127489 is a possible future destination | Withheld |
| Fuel rate / engine load | Not authorized by SmartCraft Input Contract v1.0.0 | Not present in v1 packet | PGN 127489 is a possible future destination | Withheld |
| Battery voltage | Not authorized by SmartCraft Input Contract v1.0.0 | Not present in v1 packet | PGN 127489 is a possible future destination | Withheld |
| Throttle, gear, IAC | Not authorized by SmartCraft Input Contract v1.0.0 | Not present | TBD | Withheld |

The PGN names/numbers above are design candidates and do not claim Garmin Echomap compatibility for this installation.

## Unit conversion candidates

Exact calling conventions depend on the selected NMEA 2000 library and must be confirmed against its versioned API.

| Normalized input | Candidate conversion | Status |
|---|---|---|
| RPM | Preserve numeric RPM; let library encode PGN resolution | Candidate |
| Temperature in 0.01 degree C | Convert to degrees C or kelvin as required by library (`K = C + 273.15`) | Candidate |
| Runtime minutes | Convert to the library's required duration unit, likely seconds | Candidate |

Do not apply a second scaling factor to values already normalized according to the SmartCraft input contract.

## NMEA node behavior

The helm firmware must support the selected stack's required network-management behavior, including address claim and product/device information. Candidate support includes:

- ISO Address Claim, PGN 60928.
- Product Information, PGN 126996.
- Configuration Information, PGN 126998, if required by the stack/profile.

Manufacturer code, product code, model ID, software version, device class/function, unique number, preferred source address and engine instance are TBD. Do not copy Mercury identity or claim Mercury/NMEA certification.

## Scheduling

- PGN periods must be selected from source update rates, NMEA conventions, bus load and Garmin behavior.
- RPM is expected to be the fastest output; temperature and engine hours should be slower.
- Exact periods remain TBD until replay and bus-load tests.
- The scheduler must not transmit stale values merely to maintain a period.

## Unavailable and stale values

For each destination field, determine whether the selected library provides an explicit NMEA unavailable encoding. If it does, use that encoding when required by the PGN; otherwise suppress the PGN until fresh data returns.

Never use these substitutions:

- stale RPM -> `0 RPM`
- stale temperature -> `0 degrees C`
- stale engine hours -> `0 h`

Those are plausible real values and would hide a link failure.

## Verification procedure

1. Generate known normalized values without live SmartCraft.
2. Capture the NMEA segment independently and decode emitted PGNs.
3. Confirm field, instance, scaling, unavailable encoding and update rate.
4. Confirm Garmin receives and labels each value correctly.
5. Exercise stale, reboot, packet loss and out-of-range cases.
6. Promote a mapping only after repeated agreement between injected normalized values, captured NMEA PGNs and Garmin display.

## References

- SmartCraft source definitions and upstream evidence control are maintained exclusively in [`SMARTCRAFT_INPUT_CONTRACT.md`](SMARTCRAFT_INPUT_CONTRACT.md).
- Garmin NMEA 2000 technical reference: https://www8.garmin.com/manuals/webhelp/GUID-1415AAD0-FE63-42A6-8F8D-DB713D616122/EN-US/Technical_Reference_for_Garmin_NMEA_2000_Products_EN-US.pdf
- The normative NMEA 2000 standard is not included in this repository and may require licensed access.

