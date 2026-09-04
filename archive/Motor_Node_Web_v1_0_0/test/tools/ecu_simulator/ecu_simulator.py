#!/usr/bin/env python3
from __future__ import annotations
import argparse,json,os,queue,sys,threading,time,traceback
from datetime import datetime,timezone
from pathlib import Path
from ecu_core import EcuSimulator
from slcan import CanFrame,SlcanPort

def utc():return datetime.now(timezone.utc).isoformat()
def stdin_worker(lines):
    while True:
        line=sys.stdin.readline()
        if line=='':return
        lines.put(line.rstrip('\r\n'))
def atomic_json(path,obj):
    tmp=path.with_suffix(path.suffix+'.tmp');tmp.write_text(json.dumps(obj,indent=2)+'\n',encoding='utf-8');os.replace(tmp,path)

def run_iteration(sim,port,lines,log,now:float):
    while not lines.empty():
        line=lines.get_nowait();log('command',{'line':line});reply=sim.command(line,now);print(reply)
    if port:
        for frame in port.receive():sim.on_frame(frame,now)
    sim.tick(now)

def exception_details(exc:Exception)->dict:
    return {'reason':repr(exc),'traceback':traceback.format_exc()}
def main():
    ap=argparse.ArgumentParser(description='CraftBridge desktop ECM-555 ECU simulator')
    ap.add_argument('--port',default='COM3');ap.add_argument('--serial-baud',type=int,default=115200)
    ap.add_argument('--mode',choices=('fresh-startup','existing-s3'),default='fresh-startup');ap.add_argument('--seed',type=int,default=1)
    ap.add_argument('--output',type=Path,default=Path('test_runs'));ap.add_argument('--dry-run',action='store_true',help='do not open CANable or any COM port')
    ap.add_argument('--snapshot',type=Path,default=Path(__file__).with_name('authority_snapshot.json'));a=ap.parse_args()
    if not a.dry_run and a.port.upper()=='COM9':print('REFUSED: COM9 is reserved for Motor Node logging/upload',file=sys.stderr);return 2
    stamp=datetime.now().strftime('%Y%m%d_%H%M%S');run=a.output/f"ecu_sim_{stamp}";run.mkdir(parents=True,exist_ok=False)
    raw_path=run/'raw_events.jsonl';summary_path=run/'summary.json';raw=raw_path.open('a',encoding='utf-8',buffering=1);start_mono=time.monotonic();start_utc=utc()
    def log(kind,data):
        event={'host_time_utc':utc(),'monotonic_s':round(time.monotonic()-start_mono,6),'event':kind}|data;raw.write(json.dumps(event,separators=(',',':'))+'\n')
        if kind=='expected_values':print('EXPECTED',json.dumps({k:v for k,v in data.items() if k!='monotonic'},sort_keys=True))
        elif kind in ('failure','startup_pass'):print(kind.upper(),json.dumps(data,sort_keys=True))
    port=None;dry_tx=[]
    try:
        if a.dry_run:
            send=lambda frame:dry_tx.append(frame)
        else:
            try:port=SlcanPort(a.port,a.serial_baud);port.open_can()
            except ImportError:print('Install dependency: py -m pip install pyserial',file=sys.stderr);return 2
            send=port.send
        sim=EcuSimulator(a.snapshot,a.mode,a.seed,send,log);lines=queue.Queue();threading.Thread(target=stdin_worker,args=(lines,),daemon=True).start()
        print('CraftBridge Desktop ECU Simulator')
        print(f"Port: {'NONE (dry-run)' if a.dry_run else a.port}")
        print(f'Serial baudrate: {a.serial_baud}\nCAN bitrate: 250000\nMode: {a.mode}\nSeed: {a.seed}')
        print('SAFE IDLE\nCAN application TX = 0\nWaiting for exact START command')
        print('Commands: START | STATUS | PAUSE <signal|s3> | RESUME <signal|s3> | SET <signal> <value> | STOP')
        while not sim.stopped and not sim.failed:
            now=time.monotonic()
            run_iteration(sim,port,lines,log,now);time.sleep(0.002)

    except KeyboardInterrupt:
        if 'sim' in locals():sim.stop()
    except Exception as exc:
        details=exception_details(exc)
        if 'sim' in locals():sim.fail(details['reason'],details['traceback'])
        else:log('failure',details)
    finally:
        if port:
            try:port.close()
            except Exception as exc:log('close_error',exception_details(exc))
        status=sim.status() if 'sim' in locals() else {'failed':True,'failure':'initialization failed','phase':'failed'}
        result='FAIL' if status.get('failed') else ('PASS' if status.get('started') and (a.mode=='existing-s3' or status.get('step')==30) else 'INCOMPLETE')
        summary={'schema':1,'start_utc':start_utc,'end_utc':utc(),'port':None if a.dry_run else a.port,'serial_baud':a.serial_baud,'can_bitrate':250000,'mode':a.mode,'seed':a.seed,'dry_run':a.dry_run,'result':result,'status':status,'raw_log':'raw_events.jsonl'}
        atomic_json(summary_path,summary);raw.close();print(f"Evidence folder: {run.resolve()}\nResult: {result}")
    return 2 if result=='FAIL' else 0
if __name__=='__main__':raise SystemExit(main())