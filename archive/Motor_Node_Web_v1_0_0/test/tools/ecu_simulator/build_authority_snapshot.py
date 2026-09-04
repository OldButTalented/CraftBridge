#!/usr/bin/env python3
"""Build or verify the ECU-simulator snapshot from immutable authority files."""
from __future__ import annotations
import argparse,csv,json,statistics
from collections import Counter,defaultdict
from pathlib import Path

TRANSFORMS={"fa0401":{"multiplier":0xD379A9C8,"xor":0x1B4610CA,"prefix":0xF9},"fa0206":{"multiplier":0xCF88B813,"xor":0x4353E4D3,"prefix":0xF9},"selector8004":{"multiplier":0xAB20FA1B,"xor":0x208FB01A,"prefix":0x81}}
DYNAMIC_BY_TX={7:"fa0401",28:"fa0206",30:"selector8004"}

def build(authority:Path,raw_path:Path,contract_path:Path):
    contract=contract_path.read_text(encoding="utf-8")
    required=("Input-contract version: **1.3.0**","CAN ID: `0x170`","CAN ID: `0x1A0`","rpm = raw","coolant_temperature_c = raw","runtime_hours = raw / 60.0","Normalized field: `oil_pressure_kpa`","Scale: `0.01 kPa/bit`","Protocol-global valid range: **Unknown**","No invalid or sentinel raw value is documented.","`0xFFFF` was not observed","battery_voltage_v = raw * 0.001","fuel_flow_lph = raw * 0.01")
    missing=[token for token in required if token not in contract]
    if missing: raise ValueError(f"input contract mismatch: {missing}")
    rows=list(csv.DictReader((authority/'CONNECT_TX_SEQUENCE.csv').open(encoding='utf-8-sig')))
    gates=json.loads((authority/'REPLAY_RESPONSE_GATES.json').read_text(encoding='utf-8-sig'))['gates']
    gate_by_source={int(g['source_sequence_number']):g for g in gates}; steps=[]
    for row in rows:
        n=int(row['TX_number']); dynamic=row['fixed_dynamic']=='dynamic'; gate=gate_by_source.get(int(row['source_sequence_number']))
        steps.append({'number':n,'id':int(row['CAN_ID'],16),'extended':True,'payload':None if dynamic else row['payload_pattern'].replace(' ',''),'dynamic_transform':DYNAMIC_BY_TX.get(n),'gate':None if not gate else {'challenge':gate['expected_payload_or_pattern']=='<E32>','response':None if gate['expected_payload_or_pattern']=='<E32>' else gate['expected_payload_or_pattern'].replace(' ','')}})
    captures=[]
    with raw_path.open(encoding='utf-8') as f:
        for line in f:
            r=json.loads(line)
            if r['direction']=='RX' and not r['extended'] and r['state']=='PASSIVE_5MIN_OBSERVATION' and r['CAN_ID'] in ('0x170','0x1A0'):captures.append(r)
    grouped=defaultdict(list)
    for r in captures:grouped[(int(r['CAN_ID'],16),r['payload'].split()[0])].append(r)
    templates=[]
    for (ident,page),items in sorted(grouped.items()):
        common=Counter(r['payload'].replace(' ','') for r in items).most_common(1)[0][0]
        times=[r['monotonic_ns']/1e6 for r in items];period=round(statistics.median(b-a for a,b in zip(times,times[1:])))
        templates.append({'id':ident,'page':page,'extended':False,'payload':common,'period_ms':period})
    return {'schema':1,'authority':{'startup':'FULL_STARTUP_REPLAY_TOOL_5MIN_PASSIVE','s3_capture':'execute_20260823_094341/raw_can.jsonl','input_contract':'SMARTCRAFT_INPUT_CONTRACT.md v1.3.0'},'can_bitrate':250000,'response_timeout_ms':250,'s0_observation_ms':8000,'steps':steps,'transforms':TRANSFORMS,'signal_mappings':{'rpm':{'id':0x170,'page':0,'bytes':[1,2],'encoding':'u16be','scale':1.0},'temperature':{'id':0x1A0,'page':7,'bytes':[2],'encoding':'u8','scale':1.0},'hours':{'id':0x1A0,'page':2,'bytes':[3,4],'encoding':'u16be','scale':1/60},'oil':{'id':0x1A0,'page':5,'bytes':[3,4],'encoding':'u16be','scale':0.01,'offset':0.0,'unit':'kPa','evidence_status':'verified_on_tested_ecu','observed_raw_range':[0,39847],'observed_range_kpa':[0.0,398.47],'protocol_global_valid_range':None,'sentinel':None,'ffff_observed':False,'physical_source':'switch-derived_on_tested_ecu','physical_measurement':'not_verified_analog_hydraulic_pressure'},'battery':{'id':0x1A0,'page':9,'bytes':[4,5],'encoding':'u16be','scale':0.001},'fuel':{'id':0x170,'page':1,'bytes':[1,2],'encoding':'u16be','scale':0.01}},'s3_templates':templates}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--authority',type=Path,required=True);ap.add_argument('--raw',type=Path,required=True);ap.add_argument('--contract',type=Path,required=True);ap.add_argument('--output',type=Path,default=Path('authority_snapshot.json'));ap.add_argument('--check',action='store_true');a=ap.parse_args();built=build(a.authority,a.raw,a.contract)
    text=json.dumps(built,indent=2)+"\n"
    if a.check:
        if not a.output.exists() or a.output.read_text(encoding='utf-8')!=text:print('AUTHORITY_SNAPSHOT_MISMATCH');return 2
        print(f"AUTHORITY_SNAPSHOT_PASS steps={len(built['steps'])} gates={sum(x['gate'] is not None for x in built['steps'])} templates={len(built['s3_templates'])}");return 0
    a.output.write_text(text,encoding='utf-8');print(a.output);return 0
if __name__=='__main__':raise SystemExit(main())
