#!/usr/bin/env python3
"""Verify Motor Node behavior against the frozen startup reference."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
from pathlib import Path

TRANSFORMS = {
    "Fa0401": (0xD379A9C8, 0x1B4610CA, 0xF9, 0xFE3A8C49),
    "Fa0206": (0xCF88B813, 0x4353E4D3, 0xF9, 0xE31AFA69),
    "Sel8004": (0xAB20FA1B, 0x208FB01A, 0x81, 0x792ECFD1),
}
EXPECTED_PARTITION_SHA256 = (
    "4A6AAF11525DAB5D49A336AA52EF7E65"
    "F8E83D824C9A7A931C72943CBD40D63C"
)
ROW = re.compile(
    r"^\s*\{\s*(?P<id>kClientId|0x[0-9A-Fa-f]+)\s*,\s*"
    r"(?P<dlc>\d+)\s*,\s*P\((?P<data>[^)]*)\)\s*,\s*"
    r"(?P<gate>true|false)\s*,\s*(?P<rdlc>\d+)\s*,\s*"
    r"P\((?P<response>[^)]*)\)\s*,\s*"
    r"(?P<challenge>true|false)\s*,\s*"
    r"Dynamic::(?P<dynamic>\w+)\s*\},\s*$",
    re.MULTILINE,
)


def payload_bytes(text: str) -> bytes:
    if not text.strip():
        return bytes()
    return bytes(int(value.strip(), 0) for value in text.split(","))


def calculated_response(name: str) -> bytes:
    multiplier, xor_value, prefix, challenge = TRANSFORMS[name]
    value = ((challenge * multiplier) & 0xFFFFFFFF) ^ xor_value
    return bytes([prefix]) + value.to_bytes(4, "big")


def sequence_hash(frames: list[tuple[int, bytes]]) -> int:
    value = 2166136261
    for identifier, payload in frames:
        encoded = (
            identifier.to_bytes(4, "big")
            + bytes([len(payload)])
            + payload
        )
        for byte in encoded:
            value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--authority", type=Path, required=True)
    parser.add_argument(
        "--contract",
        type=Path,
        default=Path("docs/SMARTCRAFT_INPUT_CONTRACT.md"),
    )
    parser.add_argument(
        "--snapshot",
        type=Path,
        default=Path("test/tools/ecu_simulator/authority_snapshot.json"),
    )

    root = Path("firmware/CraftBridge_Motor_Node_Web_v1_0_0")
    parser.add_argument(
        "--session",
        type=Path,
        default=root / "SmartCraftSession.cpp",
    )
    parser.add_argument(
        "--header",
        type=Path,
        default=root / "SmartCraftSession.h",
    )
    parser.add_argument(
        "--engine-data",
        type=Path,
        default=root / "EngineData.cpp",
    )
    parser.add_argument(
        "--web-interface",
        type=Path,
        default=root / "WebInterface.cpp",
    )
    parser.add_argument(
        "--can-bus",
        type=Path,
        default=root / "CanBus.cpp",
    )
    parser.add_argument(
        "--sketch",
        type=Path,
        default=root / "CraftBridge_Motor_Node_Web_v1_0_0.ino",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=root / "Config.h",
    )
    parser.add_argument(
        "--partition",
        type=Path,
        default=root / "partitions.csv",
    )
    args = parser.parse_args()

    tx_rows = list(
        csv.DictReader(
            (args.authority / "CONNECT_TX_SEQUENCE.csv").open(
                encoding="utf-8-sig"
            )
        )
    )
    gates = json.loads(
        (args.authority / "REPLAY_RESPONSE_GATES.json").read_text(
            encoding="utf-8-sig"
        )
    )["gates"]
    gates_by_sequence = {
        int(gate["source_sequence_number"]): gate
        for gate in gates
    }

    session_source = args.session.read_text(encoding="utf-8")
    session_header = args.header.read_text(encoding="utf-8")
    engine_source = args.engine_data.read_text(encoding="utf-8")
    web_source = args.web_interface.read_text(encoding="utf-8")
    can_source = args.can_bus.read_text(encoding="utf-8")
    sketch_source = args.sketch.read_text(encoding="utf-8")
    config_source = args.config.read_text(encoding="utf-8")
    contract_source = args.contract.read_text(encoding="utf-8")
    snapshot = json.loads(args.snapshot.read_text(encoding="utf-8"))

    rows = []
    errors = []
    for match in ROW.finditer(session_source):
        values = match.groupdict()
        rows.append(
            {
                **values,
                "id": (
                    int(values["id"], 16)
                    if values["id"] != "kClientId"
                    else values["id"]
                ),
                "data_bytes": payload_bytes(values["data"]),
                "response_bytes": payload_bytes(values["response"]),
            }
        )

    if len(rows) != 30:
        errors.append(
            f"product TX table has {len(rows)}, expected 30"
        )
    if len(gates) != 27 or sum(
        row["gate"] == "true" for row in rows
    ) != 27:
        errors.append("response gate count differs")

    instantiated = []
    for index, (authority_row, source_row) in enumerate(
        zip(tx_rows, rows),
        1,
    ):
        identifier = int(authority_row["CAN_ID"], 16)
        source_identifier = (
            0xB73
            if source_row["id"] == "kClientId"
            else source_row["id"]
        )
        dynamic = authority_row["fixed_dynamic"] == "dynamic"
        expected_dlc = (
            5
            if dynamic
            else len(bytes.fromhex(authority_row["payload_pattern"]))
        )

        if (
            source_identifier != identifier
            or int(source_row["dlc"]) != expected_dlc
        ):
            errors.append(f"TX {index} ID/DLC differs")

        if dynamic:
            transform_name = {
                7: "Fa0401",
                28: "Fa0206",
                30: "Sel8004",
            }[index]
            if source_row["dynamic"] != transform_name:
                errors.append(f"TX {index} transform differs")
            payload = calculated_response(transform_name)
        else:
            payload = bytes.fromhex(authority_row["payload_pattern"])
            if (
                source_row["data_bytes"] != payload
                or source_row["dynamic"] != "None"
            ):
                errors.append(f"TX {index} payload differs")

        instantiated.append((identifier, payload))
        gate = gates_by_sequence.get(
            int(authority_row["source_sequence_number"])
        )
        if bool(gate) != (source_row["gate"] == "true"):
            errors.append(f"TX {index} gate presence differs")

        if gate:
            pattern = gate["expected_payload_or_pattern"]
            if pattern == "<E32>":
                if (
                    source_row["challenge"] != "true"
                    or int(source_row["rdlc"]) != 4
                ):
                    errors.append(
                        f"TX {index} challenge gate differs"
                    )
            else:
                expected = bytes.fromhex(pattern)
                if (
                    source_row["challenge"] != "false"
                    or source_row["response_bytes"] != expected
                ):
                    errors.append(f"TX {index} response differs")

    checksum = sequence_hash(instantiated)
    if checksum != 0x7B13B034:
        errors.append(
            f"known capture checksum is 0x{checksum:08X}"
        )

    session_tokens = (
        "kStartupWaitMs = 8000",
        "kS3FreshnessMs = 5000",
        "kResponseGateTimeoutMs = 250",
        "kBaselineLossTimeoutMs = 2000",
        "kRequired170Mask = 0x807F",
        "kRequired1A0Mask = 0x9FFF",
        "SessionState::SessionLost",
        "stopApplicationTx();",
    )
    for token in session_tokens:
        if token not in session_header and token not in session_source:
            errors.append(f"missing session invariant: {token}")

    forbidden_recovery_tokens = (
        "kRetryDelayMs",
        "RecoveryStarting",
        "beginStartup(now_ms, true)",
        "retryStartup",
        "automaticRecovery",
    )
    for token in forbidden_recovery_tokens:
        if token in session_source or token in session_header:
            errors.append(
                f"automatic recovery token present: {token}"
            )

    mapping_tokens = {
        "rpm": (
            "frame.id == 0x170 && page == 0x00",
            "readUint16BigEndian(&frame.data[1])",
        ),
        "fuel": (
            "frame.id == 0x170 && page == 0x01",
            "* 0.01f",
        ),
        "temperature": (
            "page == 0x07",
            "coolant_temperature_c = frame.data[2]",
        ),
        "runtime": (
            "page == 0x02",
            "/ 60.0f",
        ),
        "oil": (
            "page == 0x05",
            "oil_pressure_kpa =",
            "readUint16BigEndian(&frame.data[3]) * 0.01f",
        ),
        "battery": (
            "page == 0x09",
            "* 0.001f",
        ),
    }
    for name, tokens in mapping_tokens.items():
        if not all(token in engine_source for token in tokens):
            errors.append(f"missing decoder mapping: {name}")

    contract_tokens = (
        "Input-contract version: **1.3.0**",
        "Normalized field: `oil_pressure_kpa`",
        "Scale: `0.01 kPa/bit`",
        "Protocol-global valid range: **Unknown**",
        "No invalid or sentinel raw value is documented.",
        "`0xFFFF` was not observed",
    )
    for token in contract_tokens:
        if token not in contract_source:
            errors.append(f"missing numeric oil contract invariant: {token}")

    expected_oil_mapping = {
        "id": 0x1A0,
        "page": 0x05,
        "bytes": [3, 4],
        "encoding": "u16be",
        "scale": 0.01,
        "offset": 0.0,
        "unit": "kPa",
        "evidence_status": "verified_on_tested_ecu",
        "observed_raw_range": [0, 39847],
        "observed_range_kpa": [0.0, 398.47],
        "protocol_global_valid_range": None,
        "sentinel": None,
        "ffff_observed": False,
        "physical_source": "switch-derived_on_tested_ecu",
        "physical_measurement": "not_verified_analog_hydraulic_pressure",
    }
    if snapshot.get("authority", {}).get("input_contract") != (
        "SMARTCRAFT_INPUT_CONTRACT.md v1.3.0"
    ):
        errors.append("authority snapshot contract version differs")
    if snapshot.get("signal_mappings", {}).get("oil") != expected_oil_mapping:
        errors.append("authority snapshot numeric oil mapping differs")

    numeric_oil_tokens = (
        "oil_pressure_kpa",
        "readUint16BigEndian(&frame.data[3]) * 0.01f",
        "\"Oil pressure\"",
        "\"kPa\"",
    )
    runtime_sources = engine_source + session_header + web_source
    if not all(token in runtime_sources for token in numeric_oil_tokens):
        errors.append("numeric oil runtime mapping is incomplete")
    for token in ("oil_pressure_ok", "raw_value == 0x9B82"):
        if token in runtime_sources:
            errors.append(f"legacy binary oil runtime token present: {token}")

    endpoints = ("/", "/api/status")
    for endpoint in endpoints:
        registration = f'server_.on("{endpoint}", HTTP_GET'
        if registration not in web_source:
            errors.append(f"missing web endpoint: {endpoint}")

    config_tokens = (
        "CraftBridge-Motor-Node/1.0.0",
        "kCanTxGpio = 4",
        "kCanRxGpio = 5",
        "kCanBitrate = 250000",
        "kSerialMonitorBaud = 115200",
        "kWebSnapshotIntervalMs = 250",
    )
    for token in config_tokens:
        if token not in config_source:
            errors.append(f"missing fixed configuration: {token}")

    can_tokens = (
        "TWAI_TIMING_CONFIG_250KBITS()",
        "message.extd = 1",
        "twai_transmit(&message, pdMS_TO_TICKS(20))",
    )
    for token in can_tokens:
        if token not in can_source:
            errors.append(f"missing CAN safety behavior: {token}")

    sketch_tokens = (
        "SAFE IDLE: SmartCraft application TX = 0",
        "can_bus.begin()",
        "smartcraft_session.tick(now_ms)",
        "web_interface.update(",
    )
    for token in sketch_tokens:
        if token not in sketch_source:
            errors.append(f"missing application behavior: {token}")

    partition_bytes = args.partition.read_bytes()
    canonical_partition = partition_bytes.replace(
        b"\r\n", b"\n"
    ).replace(b"\r", b"\n")
    partition_hash = hashlib.sha256(
        canonical_partition
    ).hexdigest().upper()
    if partition_hash != EXPECTED_PARTITION_SHA256:
        errors.append(
            f"partition SHA-256 differs: {partition_hash}"
        )

    result = {
        "status": "PASS" if not errors else "FAIL",
        "tx_steps": len(rows),
        "response_gates": len(gates),
        "challenge_transforms": 3,
        "contract_signal_mappings": len(snapshot.get("signal_mappings", {})),
        "runtime_contract_mappings": len(mapping_tokens),
        "runtime_pending_mappings": [],
        "legacy_binary_oil_runtime_preserved": False,
        "web_endpoints": list(endpoints),
        "known_capture_checksum": f"0x{checksum:08X}",
        "gate_timeout_ms": 250,
        "s0_observation_ms": 8000,
        "s3_freshness_ms": 5000,
        "s3_masks": ["0x807F", "0x9FFF"],
        "automatic_recovery": False,
        "gpio": {"tx": 4, "rx": 5},
        "can_bitrate": 250000,
        "firmware_identity": "CraftBridge-Motor-Node/1.0.0",
        "partition_sha256": partition_hash,
        "errors": errors,
    }
    print(json.dumps(result, indent=2))
    return 0 if not errors else 2


if __name__ == "__main__":
    raise SystemExit(main())
