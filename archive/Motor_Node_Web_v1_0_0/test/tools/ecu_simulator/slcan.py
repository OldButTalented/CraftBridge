from __future__ import annotations
from dataclasses import dataclass

@dataclass(frozen=True)
class CanFrame:
    identifier:int
    extended:bool
    data:bytes

    def __post_init__(self):
        limit=0x1FFFFFFF if self.extended else 0x7FF
        if not 0<=self.identifier<=limit:raise ValueError("CAN identifier out of range")
        if len(self.data)>8:raise ValueError("CAN payload exceeds 8 bytes")

def format_frame(frame:CanFrame)->str:
    prefix='T' if frame.extended else 't';width=8 if frame.extended else 3
    return f"{prefix}{frame.identifier:0{width}X}{len(frame.data):X}{frame.data.hex().upper()}"

def encode_frame(frame:CanFrame)->bytes:
    return (format_frame(frame)+'\r').encode('ascii')

def parse_frame(raw_line:bytes|bytearray|memoryview)->CanFrame|None:
    if not isinstance(raw_line,(bytes,bytearray,memoryview)):
        raise TypeError('SLCAN input must be bytes-like')
    line=bytes(raw_line).decode('ascii','strict').strip('\r\n\a')
    if not line or line[0] not in 'tT':return None
    extended=line[0]=='T';width=8 if extended else 3
    if len(line)<1+width+1:raise ValueError("short SLCAN frame")
    ident=int(line[1:1+width],16);dlc=int(line[1+width],16)
    if dlc>8:raise ValueError("invalid SLCAN DLC")
    payload=line[2+width:]
    if len(payload)!=dlc*2:raise ValueError("SLCAN payload length mismatch")
    try:data=bytes.fromhex(payload)
    except ValueError as exc:raise ValueError("invalid SLCAN hex") from exc
    return CanFrame(ident,extended,data)

class SlcanStreamDecoder:
    def __init__(self):self.buffer=bytearray()
    def feed(self,chunk:bytes|bytearray|memoryview)->list[CanFrame]:
        if not isinstance(chunk,(bytes,bytearray,memoryview)):
            raise TypeError('serial read must return bytes-like data')
        self.buffer.extend(chunk);out=[]
        while b'\r' in self.buffer:
            raw_line,_,rest=self.buffer.partition(b'\r');self.buffer=bytearray(rest)
            parsed=parse_frame(raw_line)
            if parsed is not None:out.append(parsed)
        return out

class SlcanPort:
    def __init__(self,port:str,baud:int,timeout:float=0.05,serial_port=None):
        if serial_port is None:
            import serial
            serial_port=serial.Serial(port,baudrate=baud,timeout=timeout,write_timeout=1)
        self.serial=serial_port;self.decoder=SlcanStreamDecoder();self.opened=False
    def _write_ascii_line(self,text:str):
        if not isinstance(text,str):raise TypeError('SLCAN output must be str before serial boundary')
        if '\r' in text or '\n' in text:raise ValueError('SLCAN line must not contain CR/LF')
        payload=(text+'\r').encode('ascii','strict')
        self.serial.write(payload);self.serial.flush()
    def command(self,text:str):self._write_ascii_line(text)
    def open_can(self):
        self.command('C');self.command('S5');self.command('O');self.opened=True
    def send(self,frame:CanFrame):
        if not self.opened:raise RuntimeError('SLCAN channel is closed')
        self._write_ascii_line(format_frame(frame))
    def receive(self)->list[CanFrame]:
        waiting=getattr(self.serial,'in_waiting',0);chunk=self.serial.read(waiting or 1)
        return self.decoder.feed(chunk)
    def close(self):
        try:
            if self.opened:self.command('C')
        finally:self.opened=False;self.serial.close()
