from __future__ import annotations
import json,random
from pathlib import Path
from slcan import CanFrame

SIGNAL_PAGE={'rpm':(0x170,0x00),'fuel':(0x170,0x01),'hours':(0x1A0,0x02),'oil':(0x1A0,0x05),'temperature':(0x1A0,0x07),'battery':(0x1A0,0x09)}

def transform(value:int,multiplier:int,xor:int)->int:return ((value*multiplier)&0xffffffff)^xor

class SignalModel:
    def __init__(self,seed:int):
        self.rng=random.Random(seed);self.rpm=900;self.temperature=72;self.hours=223.0;self.oil_pressure_kpa=398.19;self.battery=13.8;self.fuel=0.5;self.last_update=None
    def update(self,now:float):
        if self.last_update is None:self.last_update=now;return
        elapsed=max(0,now-self.last_update);self.last_update=now;self.hours+=elapsed/3600.0
        self.rpm=max(700,min(3500,self.rpm+self.rng.choice((-20,-10,0,0,10,20))))
        self.temperature=max(60,min(95,self.temperature+self.rng.choice((-1,0,0,0,1))))
        self.battery=max(12.0,min(14.6,self.battery+self.rng.choice((-0.02,-0.01,0,0.01,0.02))))
        target=max(0.2,min(18.0,0.2+self.rpm*0.00035));self.fuel=max(0,min(20,self.fuel+(target-self.fuel)*0.2+self.rng.choice((-0.01,0,0.01))))
    def expected(self):return {'rpm':self.rpm,'temperature_c':self.temperature,'hours':round(self.hours,4),'oil_pressure_kpa':round(self.oil_pressure_kpa,2),'battery_v':round(self.battery,3),'fuel_lph':round(self.fuel,2)}
    def set_value(self,name:str,value:str):
        if name=='rpm':self.rpm=max(0,min(6500,int(value)))
        elif name=='temperature':self.temperature=max(0,min(255,int(value)))
        elif name=='hours':self.hours=max(self.hours,float(value))
        elif name=='battery':self.battery=max(0,min(65.535,float(value)))
        elif name=='fuel':self.fuel=max(0,min(655.35,float(value)))
        elif name=='oil':
            raw=round(float(value)*100)
            if not 0<=raw<=0xffff:raise ValueError('oil value outside u16 representable domain')
            self.oil_pressure_kpa=raw/100.0
        else:raise ValueError('unknown signal or invalid value')

class EcuSimulator:
    def __init__(self,snapshot_path:Path,mode:str,seed:int,send,log):
        self.authority=json.loads(snapshot_path.read_text(encoding='utf-8'));self.mode=mode;self.seed=seed;self.send_callback=send;self.log=log;self.model=SignalModel(seed);self.challenge_rng=random.Random(seed^0xEC0555)
        self.started=False;self.failed=False;self.stopped=False;self.phase='safe_idle';self.failure=None;self.failure_traceback=None;self.step_index=0;self.deadline=None;self.pending_dynamic={};self.paused=set();self.pause_s3=False;self.next_model=0.0
        self.templates={(x['id'],int(x['page'],16)):bytes.fromhex(x['payload']) for x in self.authority['s3_templates']};self.family_index={0x170:0,0x1A0:0};self.next_family={0x170:0.0,0x1A0:0.0}
    @property
    def tx_allowed(self):return self.started and not self.failed and not self.stopped
    def start(self,now:float):
        if self.started or self.failed or self.stopped:return False
        self.started=True;self.phase='waiting_startup' if self.mode=='fresh-startup' else 's3';self.deadline=now+12.0 if self.mode=='fresh-startup' else None;self.next_model=now
        self.next_family={0x170:now,0x1A0:now};self.log('state',{'state':self.phase,'seed':self.seed});return True
    def stop(self):self.stopped=True;self.phase='stopped'
    def fail(self,reason:str,traceback_text:str|None=None):
        self.failed=True;self.phase='failed';self.failure=reason;self.failure_traceback=traceback_text;self.deadline=None
        event={'reason':reason}
        if traceback_text:event['traceback']=traceback_text
        self.log('failure',event)
    def _send(self,frame:CanFrame,now:float,reason:str):
        if not self.tx_allowed:return
        self.send_callback(frame);self.log('can_tx',{'id':frame.identifier,'extended':frame.extended,'dlc':len(frame.data),'payload':frame.data.hex().upper(),'reason':reason,'monotonic':now})
    def _challenge(self,name:str)->bytes:
        value=self.challenge_rng.getrandbits(32);cfg=self.authority['transforms'][name];expected=bytes([cfg['prefix']])+transform(value,cfg['multiplier'],cfg['xor']).to_bytes(4,'big');self.pending_dynamic[name]=expected;return value.to_bytes(4,'big')
    def on_frame(self,frame:CanFrame,now:float):
        self.log('can_rx',{'id':frame.identifier,'extended':frame.extended,'dlc':len(frame.data),'payload':frame.data.hex().upper(),'monotonic':now})
        if not self.started:self.fail('Motor Node TX received before simulator START');return
        if self.phase=='s3':self.fail('unexpected Motor Node application TX during S3');return
        if self.phase!='waiting_startup' or self.step_index>=len(self.authority['steps']):self.fail('unexpected Motor Node TX state');return
        step=self.authority['steps'][self.step_index];expected=bytes.fromhex(step['payload']) if step['payload'] is not None else self.pending_dynamic.get(step['dynamic_transform'])
        if frame.identifier!=step['id'] or frame.extended!=step['extended'] or expected is None or frame.data!=expected:
            self.fail(f"startup TX {self.step_index+1} mismatch");return
        self.step_index+=1;gate=step['gate']
        if gate:
            payload=self._challenge(step['dynamic_transform'] or {6:'fa0401',27:'fa0206',29:'selector8004'}[step['number']]) if gate['challenge'] else bytes.fromhex(gate['response'])
            self._send(CanFrame(0x730B,True,payload),now,f"gate {sum(1 for x in self.authority['steps'][:self.step_index] if x['gate'])}")
        self.deadline=now+self.authority['response_timeout_ms']/1000.0
        if self.step_index==len(self.authority['steps']):
            self.phase='s3';self.deadline=None;self.next_family={0x170:now,0x1A0:now};self.log('startup_pass',{'tx_steps':30,'gates':27})
    def _payload(self,ident:int,page:int)->bytes:
        data=bytearray(self.templates[(ident,page)])
        if (ident,page)==SIGNAL_PAGE['rpm']:
            raw=int(round(self.model.rpm));data[1:3]=raw.to_bytes(2,'big')
        elif (ident,page)==SIGNAL_PAGE['fuel']:
            raw=int(round(self.model.fuel*100));data[1:3]=raw.to_bytes(2,'big')
        elif (ident,page)==SIGNAL_PAGE['temperature']:data[2]=int(round(self.model.temperature))
        elif (ident,page)==SIGNAL_PAGE['hours']:
            raw=int(self.model.hours*60);data[3:5]=raw.to_bytes(2,'big')
        elif (ident,page)==SIGNAL_PAGE['oil']:
            raw=round(self.model.oil_pressure_kpa*100);data[3:5]=raw.to_bytes(2,'big')
        elif (ident,page)==SIGNAL_PAGE['battery']:
            raw=int(round(self.model.battery*1000));data[4:6]=raw.to_bytes(2,'big')
        return bytes(data)
    def _pages(self,ident:int):
        if self.phase=='waiting_startup':return [0x00,0x03,0x06,0xFF] if ident==0x170 else [0x01,0xFF]
        return list(range(7))+[0xFF] if ident==0x170 else list(range(13))+[0xFF]
    def tick(self,now:float):
        if not self.tx_allowed:return
        if self.deadline is not None and now>self.deadline:self.fail(f'startup timeout waiting for TX {self.step_index+1}');return
        if now>=self.next_model:
            self.model.update(now);self.log('expected_values',self.model.expected()|{'monotonic':now});self.next_model=now+1.0
        if self.pause_s3 and self.phase=='s3':return
        for ident in (0x170,0x1A0):
            pages=self._pages(ident);interval=(0.100/len(pages)) if ident==0x170 else (0.250/len(pages))
            while now>=self.next_family[ident]:
                page=pages[self.family_index[ident]%len(pages)];self.family_index[ident]+=1;self.next_family[ident]+=interval
                signal=next((name for name,key in SIGNAL_PAGE.items() if key==(ident,page)),None)
                if signal not in self.paused:self._send(CanFrame(ident,False,self._payload(ident,page)),now,'S0' if self.phase=='waiting_startup' else 'S3')
    def command(self,line:str,now:float)->str:
        parts=line.strip().split()
        if not parts:return 'IGNORED'
        if parts==['START']:return 'START accepted' if self.start(now) else 'START rejected: already started/terminal'
        cmd=parts[0].upper()
        if cmd=='STATUS':return json.dumps(self.status(),sort_keys=True)
        if cmd in ('PAUSE','RESUME') and len(parts)==2:
            name=parts[1].lower()
            if name=='s3':self.pause_s3=cmd=='PAUSE'
            elif name in SIGNAL_PAGE:
                (self.paused.add if cmd=='PAUSE' else self.paused.discard)(name)
            else:return 'ERROR unknown signal'
            return f'{cmd} {name} accepted'
        if cmd=='SET' and len(parts)==3:
            try:self.model.set_value(parts[1].lower(),parts[2].lower())
            except ValueError as exc:return f'ERROR {exc}'
            return 'SET accepted'
        if cmd=='STOP':self.stop();return 'STOP accepted'
        return 'IGNORED'
    def status(self):return {'mode':self.mode,'phase':self.phase,'started':self.started,'failed':self.failed,'step':self.step_index,'paused':sorted(self.paused),'s3_paused':self.pause_s3,'expected':self.model.expected(),'failure':self.failure,'failure_traceback':self.failure_traceback}