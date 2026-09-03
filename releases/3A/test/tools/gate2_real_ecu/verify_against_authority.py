#!/usr/bin/env python3
"""Compare the Gate-2 sketch with the authoritative Python replay specifications."""
from __future__ import annotations
import argparse, csv, json, re, sys
from pathlib import Path

TRANSFORMS={"Fa0401":(0xD379A9C8,0x1B4610CA,0xF9,0xFE3A8C49),"Fa0206":(0xCF88B813,0x4353E4D3,0xF9,0xE31AFA69),"Selector8004":(0xAB20FA1B,0x208FB01A,0x81,0x792ECFD1)}
ROW=re.compile(r'^\s*\{(?P<id>kClientId|0x[0-9A-Fa-f]+),(?P<dlc>\d+),B\((?P<data>[^)]*)\),Transform::(?P<transform>\w+),GateKind::(?P<gate>\w+),(?P<rdlc>\d+),B\((?P<response>[^)]*)\),"[^"]+"\},\s*$',re.M)

def bytes_arg(text):
    if not text.strip(): return bytes()
    return bytes(int(x.strip(),0) for x in text.split(','))
def fnv(frames):
    h=2166136261
    for ident,data in frames:
        for x in ident.to_bytes(4,'big')+bytes([len(data)])+data: h=((h^x)*16777619)&0xffffffff
    return h
def calc(name):
    m,x,p,e=TRANSFORMS[name]; r=(((e*m)&0xffffffff)^x).to_bytes(4,'big'); return bytes([p])+r


def verify_safe_start(source):
    errors=[]
    required=(
        "bool applicationTxEnabled = false;", "bool attemptStarted = false;", "bool twaiReady = false;",
        'Serial.println("SAFE IDLE")', 'Serial.println("SmartCraft application TX = 0")',
        'Serial.println("Waiting for START command")',
        'last-first==5 && memcmp(commandBuffer+first,"START",5)==0',
        "attemptStarted=true;", 'Serial.println("START accepted")')
    for token in required:
        if token not in source: errors.append(f"missing safe-start token: {token}")
    setup=source[source.find("void setup()"):source.find("void loop()")]
    if "runGate2();" in setup: errors.append("setup automatically calls active sequence")
    if setup.find("twaiReady=true;")<setup.find("twai_start()!=ESP_OK"): errors.append("TWAI readiness can be asserted before successful initialization")
    command=source[source.find("void processCommand()"):source.find("void pollStartCommand()")]
    if command.find("attemptStarted=true;")<0 or command.find("runGate2();")<command.find("attemptStarted=true;"):
        errors.append("attempt is not locked before active sequence")
    run=source[source.find("void runGate2()"):source.find("char commandBuffer")]
    arm=run.find("armBaseline()"); enable=run.find("applicationTxEnabled=true;"); transmit=run.find("for (uint8_t i=0;i<30")
    if min(arm,enable,transmit)<0 or not arm<enable<transmit:
        errors.append("TX is not enabled only after successful S0 ARM")

    class Model:
        def __init__(self): self.started=False; self.tx=0; self.terminal=False
        def line(self,value):
            if self.started or self.terminal or value.strip(" \t")!="START": return False
            self.started=True; return True
        def finish(self): self.tx=0; self.terminal=True
    m=Model()
    tests={"boot_no_start_tx_zero":m.tx==0,
           "indefinite_wait_tx_zero":all(not m.line(x) for x in ("", "noise", "start", "STARTED", "XSTART")) and m.tx==0,
           "exact_start_once":m.line(" \tSTART\t ") and not m.line("START")}
    m.finish(); tests["pass_or_fail_terminal_tx_zero_no_restart"]=m.tx==0 and not m.line("START")
    for name,passed in tests.items():
        if not passed: errors.append(f"safe-start model failed: {name}")
    return tests,errors

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--authority",type=Path,required=True); ap.add_argument("--sketch",type=Path,default=Path("Gate2_Real_ECU/Gate2_Real_ECU.ino")); a=ap.parse_args()
    tx=list(csv.DictReader((a.authority/"CONNECT_TX_SEQUENCE.csv").open(encoding="utf-8-sig")))
    gates=json.loads((a.authority/"REPLAY_RESPONSE_GATES.json").read_text(encoding="utf-8-sig"))["gates"]
    gate_by_source={int(g["source_sequence_number"]):g for g in gates}
    source=a.sketch.read_text(encoding="utf-8"); rows=[]
    for m in ROW.finditer(source):
        rows.append({"id":0xB73 if m["id"]=="kClientId" else int(m["id"],16),"dlc":int(m["dlc"]),"data":bytes_arg(m["data"]),"transform":m["transform"],"gate":m["gate"],"rdlc":int(m["rdlc"]),"response":bytes_arg(m["response"])})
    errors=[]
    safe_start,safe_errors=verify_safe_start(source); errors.extend(safe_errors)
    if len(tx)!=30 or len(rows)!=30: errors.append(f"TX total authority={len(tx)} sketch={len(rows)}")
    if len(gates)!=27 or sum(r["gate"]!="None" for r in rows)!=27: errors.append("gate total mismatch")
    authority_frames=[]; sketch_frames=[]
    for i,(t,s) in enumerate(zip(tx,rows),1):
        ident=int(t["CAN_ID"],16); dynamic=t["fixed_dynamic"]=="dynamic"
        if ident!=s["id"] or int(t["TX_number"])!=i: errors.append(f"TX {i} ID/order mismatch")
        expected_gate=gate_by_source.get(int(t["source_sequence_number"]));
        if bool(expected_gate)!=(s["gate"]!="None"): errors.append(f"TX {i} gate-presence mismatch")
        if dynamic:
            mapping={7:"Fa0401",28:"Fa0206",30:"Selector8004"}; name=mapping[i]
            if s["transform"]!=name or s["dlc"]!=5: errors.append(f"TX {i} dynamic mapping mismatch")
            data=calc(name)
        else:
            data=bytes.fromhex(t["payload_pattern"])
            if s["transform"]!="None" or s["data"]!=data or s["dlc"]!=len(data): errors.append(f"TX {i} fixed payload/DLC mismatch")
        authority_frames.append((ident,data)); sketch_frames.append((s["id"],data))
        if expected_gate:
            pattern=expected_gate["expected_payload_or_pattern"]
            if pattern=="<E32>":
                if s["gate"]!="Challenge" or s["rdlc"]!=4: errors.append(f"TX {i} challenge gate mismatch")
            else:
                exp=bytes.fromhex(pattern)
                if s["gate"]!="Fixed" or s["rdlc"]!=len(exp) or s["response"]!=exp: errors.append(f"TX {i} fixed gate mismatch")
    for name,(mul,xor,prefix,e) in TRANSFORMS.items():
        for literal in (f"0x{mul:08X}",f"0x{xor:08X}"):
            if literal not in source: errors.append(f"missing transform literal {literal}")
    checksum=fnv(sketch_frames)
    if authority_frames!=sketch_frames: errors.append("instantiated authoritative/sketch sequence differs")
    if checksum!=0x7B13B034: errors.append(f"known-capture checksum {checksum:#010x} != 0x7B13B034")
    required_tokens=("kArmObservationMs = 8000","required170=0x8049","required1A0=0x8002","kRequired170 = 0x807F","kRequired1A0 = 0x9FFF","kPassiveDurationMs = 300000","stopApplicationTx();","if (m.rtr)")
    for token in required_tokens:
        if token not in source: errors.append(f"missing safety/S3 token: {token}")
    stop=source.find("stopApplicationTx();",source.find("void runGate2")); detect=source.find("detectS3()",source.find("void runGate2"))
    if stop<0 or detect<0 or stop>detect: errors.append("application TX is not disabled before S3/passive phase")
    run=source.find("void runGate2"); arm=source.find("armBaseline()",run); transmit=source.find("for (uint8_t i=0;i<30",run)
    if arm<0 or transmit<0 or arm>transmit: errors.append("fresh S0 ARM gate is not called before first application TX")
    out={"status":"PASS" if not errors else "FAIL","authoritative_tx":len(tx),"sketch_tx":len(rows),"response_gates":len(gates),"transforms":3,"known_capture_sequence_checksum":f"0x{checksum:08X}","checksum_scope":"captured challenge instance; not invariant across live challenges","s3_170_mask":"0x807F","s3_1A0_mask":"0x9FFF","passive_seconds":300,"safe_start_tests":safe_start,"errors":errors}
    print(json.dumps(out,indent=2)); return 0 if not errors else 2

if __name__=="__main__": raise SystemExit(main())