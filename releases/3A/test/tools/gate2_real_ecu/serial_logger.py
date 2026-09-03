#!/usr/bin/env python3
"""Interactive capture of a complete Gate-2 USB serial session."""
from __future__ import annotations
import argparse, json, os, queue, sys, threading, time
from datetime import datetime, timezone
from pathlib import Path

FIRMWARE_PREFIX="Firmware/version identifier:"

def utc(): return datetime.now(timezone.utc).isoformat()
def atomic_json(path:Path,obj):
    tmp=path.with_suffix(path.suffix+".tmp"); tmp.write_text(json.dumps(obj,indent=2)+"\n",encoding="utf-8"); os.replace(tmp,path)

def stdin_reader(lines:queue.Queue[str]):
    while True:
        line=sys.stdin.readline()
        if line=="": return
        lines.put(line)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("port",help="USB serial port, for example COM9")
    ap.add_argument("--baud",type=int,default=115200)
    ap.add_argument("--output",type=Path,default=Path("test_runs"))
    ap.add_argument("--timeout",type=float,default=420.0,help="overall seconds after START; 0 disables it")
    args=ap.parse_args()
    try: import serial
    except ImportError: print("Install dependency: python -m pip install pyserial",file=sys.stderr); return 2

    stamp=datetime.now().strftime("%Y%m%d_%H%M%S")
    run=args.output/f"gate2_{stamp}"; run.mkdir(parents=True,exist_ok=False)
    raw_path=run/"raw_serial.log"; meta_path=run/"metadata.json"
    meta={"schema":2,"start_utc":utc(),"port":args.port,"baud":args.baud,"firmware_id":None,"result":"INCOMPLETE","raw_log":"raw_serial.log","forwarded_lines":0,"start_forwarded_utc":None}
    atomic_json(meta_path,meta)
    lines:queue.Queue[str]=queue.Queue(); terminal_seen_at=None; active_deadline=None; line_buffer=bytearray()
    try:
        with serial.Serial(args.port,args.baud,timeout=0.10) as ser, raw_path.open("wb") as raw:
            print(f"Serial open: {args.port} at {args.baud} baud")
            print("Wait for 'Waiting for START command', then type START and press Enter.")
            threading.Thread(target=stdin_reader,args=(lines,),daemon=True).start()
            while True:
                while not lines.empty() and terminal_seen_at is None:
                    line=lines.get_nowait(); wire=line.rstrip("\r\n")+"\n"
                    ser.write(wire.encode("utf-8")); ser.flush(); meta["forwarded_lines"]+=1
                    if wire.strip(" \t\r\n")=="START" and meta["start_forwarded_utc"] is None:
                        meta["start_forwarded_utc"]=utc()
                        if args.timeout>0: active_deadline=time.monotonic()+args.timeout
                chunk=ser.read(ser.in_waiting or 1)
                if chunk:
                    raw.write(chunk); raw.flush(); os.fsync(raw.fileno()); sys.stdout.buffer.write(chunk); sys.stdout.buffer.flush()
                    line_buffer.extend(chunk)
                    while b"\n" in line_buffer:
                        line,_,rest=line_buffer.partition(b"\n"); line_buffer=bytearray(rest)
                        text=line.rstrip(b"\r").decode("utf-8",errors="replace")
                        if text.startswith(FIRMWARE_PREFIX): meta["firmware_id"]=text.split(":",1)[1].strip()
                        if text.strip()=="GATE 2 PASS": meta["result"]="PASS"; terminal_seen_at=time.monotonic()
                        elif text.strip()=="GATE 2 FAIL": meta["result"]="FAIL"; terminal_seen_at=time.monotonic()
                if terminal_seen_at and time.monotonic()-terminal_seen_at>=1.0: break
                if active_deadline and time.monotonic()>=active_deadline: break
    except KeyboardInterrupt: meta["result"]="INTERRUPTED"
    except Exception as exc: meta["result"]="LOGGER_ERROR"; meta["error"]=repr(exc)
    meta["end_utc"]=utc(); atomic_json(meta_path,meta)
    (run/"result.txt").write_text(meta["result"]+"\n",encoding="ascii")
    print(f"\nEvidence folder: {run.resolve()}\nResult: {meta['result']}")
    return 0 if meta["result"]=="PASS" else 2

if __name__=="__main__": raise SystemExit(main())