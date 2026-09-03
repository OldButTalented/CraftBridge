import json,queue,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from ecu_core import EcuSimulator,SignalModel,transform,SIGNAL_PAGE
from ecu_simulator import exception_details,run_iteration
from slcan import CanFrame,SlcanPort,encode_frame,parse_frame

class SimulatorTests(unittest.TestCase):
    def setUp(self):self.snapshot=ROOT/'authority_snapshot.json';self.events=[];self.sent=[]
    def sim(self,mode='fresh-startup',seed=7):return EcuSimulator(self.snapshot,mode,seed,self.sent.append,lambda k,d:self.events.append((k,d)))
    def test_slcan_standard_extended_roundtrip(self):
        for f in (CanFrame(0x170,False,b'\x00\x01'),CanFrame(0xB73,True,b'\x55')):self.assertEqual(parse_frame(encode_frame(f)),f);self.assertEqual(parse_frame(b'\a'+encode_frame(f)),f)
        self.assertIsNone(parse_frame(b'z'))
        with self.assertRaises(ValueError):parse_frame(b't1703AA')
    def test_authority_30_27(self):
        a=json.loads(self.snapshot.read_text());self.assertEqual(len(a['steps']),30);self.assertEqual(sum(x['gate'] is not None for x in a['steps']),27);self.assertEqual(len(a['s3_templates']),22);self.assertEqual(len(a['signal_mappings']),6)
    def test_numeric_oil_authority_mapping(self):
        authority=json.loads(self.snapshot.read_text())
        self.assertEqual(authority['authority']['input_contract'],'SMARTCRAFT_INPUT_CONTRACT.md v1.3.0')
        oil=authority['signal_mappings']['oil']
        self.assertEqual(oil['id'],0x1A0);self.assertEqual(oil['page'],0x05);self.assertEqual(oil['bytes'],[3,4])
        self.assertEqual(oil['encoding'],'u16be');self.assertEqual(oil['scale'],0.01);self.assertEqual(oil['offset'],0.0);self.assertEqual(oil['unit'],'kPa')
        self.assertEqual(oil['observed_raw_range'],[0,39847]);self.assertEqual(oil['observed_range_kpa'],[0.0,398.47])
        self.assertIsNone(oil['protocol_global_valid_range']);self.assertIsNone(oil['sentinel']);self.assertFalse(oil['ffff_observed'])
        s=self.sim()
        for raw in (0x0001,0x9B8B,0x9B94,0x9BA7):
            s.model.set_value('oil',str(raw/100.0))
            payload=s._payload(*SIGNAL_PAGE['oil'])
            self.assertEqual(payload[3:5],raw.to_bytes(2,'big'))
            self.assertAlmostEqual(int.from_bytes(payload[3:5],'big')*oil['scale'],raw/100.0,places=2)
        s.model.set_value('oil','655.35');self.assertEqual(s._payload(*SIGNAL_PAGE['oil'])[3:5],b'\xff\xff')
    def test_transforms(self):
        a=json.loads(self.snapshot.read_text())['transforms'];vectors={'fa0401':(0xFE3A8C49,0xA69CDAC2),'fa0206':(0xE31AFA69,0x6E97E918),'selector8004':(0x792ECFD1,0x348DB511)}
        for name,(e,want) in vectors.items():self.assertEqual(transform(e,a[name]['multiplier'],a[name]['xor']),want)
    def test_full_startup_and_27_gates(self):
        s=self.sim();self.assertTrue(s.start(0));now=0.01
        for step in s.authority['steps']:
            payload=bytes.fromhex(step['payload']) if step['payload'] is not None else s.pending_dynamic[step['dynamic_transform']]
            s.on_frame(CanFrame(step['id'],True,payload),now);self.assertFalse(s.failed);now+=0.01
        self.assertEqual(s.step_index,30);self.assertEqual(s.phase,'s3');self.assertEqual(len(self.sent),27)
    def test_existing_s3_rejects_any_motor_tx(self):
        s=self.sim('existing-s3');s.start(0);s.on_frame(CanFrame(0xB73,True,b'\x55'),0.1);self.assertTrue(s.failed)
    def test_signal_encodings(self):
        s=self.sim();m=s.model;m.rpm=3500;m.temperature=83;m.hours=60;m.oil_pressure_kpa=398.19;m.battery=12.5;m.fuel=.5
        self.assertEqual(s._payload(*SIGNAL_PAGE['rpm'])[1:3],b'\x0d\xac');self.assertEqual(s._payload(*SIGNAL_PAGE['temperature'])[2],83)
        self.assertEqual(s._payload(*SIGNAL_PAGE['hours'])[3:5],b'\x0e\x10');self.assertEqual(s._payload(*SIGNAL_PAGE['oil'])[3:5],b'\x9b\x8b')
        self.assertEqual(s._payload(*SIGNAL_PAGE['battery'])[4:6],b'0\xd4');self.assertEqual(s._payload(*SIGNAL_PAGE['fuel'])[1:3],b'\x002')
        m.set_value('oil','0.01');self.assertEqual(s._payload(*SIGNAL_PAGE['oil'])[3:5],b'\x00\x01');m.set_value('oil','398.47');self.assertEqual(s._payload(*SIGNAL_PAGE['oil'])[3:5],b'\x9b\xa7')
    def test_deterministic_seed_and_hours_monotonic(self):
        a=SignalModel(123);b=SignalModel(123);start=a.hours
        for t in range(10):a.update(float(t));b.update(float(t))
        self.assertEqual(a.expected(),b.expected());self.assertGreaterEqual(a.hours,start)
    def test_challenge_seed_independent_of_signal_tick_timing(self):
        a=self.sim(seed=55);b=self.sim(seed=55)
        for t in range(20):a.model.update(float(t))
        self.assertEqual(a._challenge('fa0401'),b._challenge('fa0401'))
        self.assertEqual(a.command('random text',0),'IGNORED')
    def test_pause_set_status_stop(self):
        s=self.sim('existing-s3');self.assertEqual(s.command('START',0),'START accepted');self.assertIn('accepted',s.command('PAUSE rpm',0));self.assertIn('rpm',s.paused)
        self.assertIn('accepted',s.command('RESUME rpm',0));self.assertNotIn('rpm',s.paused);self.assertEqual(s.command('SET rpm 1500',0),'SET accepted');self.assertEqual(s.model.rpm,1500)
        self.assertIn('existing-s3',s.command('STATUS',0));self.assertEqual(s.command('STOP',0),'STOP accepted');self.assertTrue(s.stopped)
    def test_safe_idle_has_zero_tx(self):
        s=self.sim();s.tick(100);self.assertFalse(s.started);self.assertEqual(self.sent,[])
    def test_timeout_stops_tx(self):
        s=self.sim();s.start(0);s.tick(12.1);self.assertTrue(s.failed);count=len(self.sent);s.tick(20);self.assertEqual(len(self.sent),count)

class StrictFakeSerial:
    def __init__(self,chunks=()):self.chunks=list(chunks);self.writes=[];self.closed=False;self.flushes=0
    @property
    def in_waiting(self):return len(self.chunks[0]) if self.chunks else 0
    def read(self,size):
        if not self.chunks:return b''
        chunk=self.chunks.pop(0)
        return chunk
    def write(self,data):
        if not isinstance(data,bytes):raise TypeError('strict serial write requires bytes')
        self.writes.append(data);return len(data)
    def flush(self):self.flushes+=1
    def close(self):self.closed=True

class SerialBoundaryRegressionTests(unittest.TestCase):
    def setUp(self):self.snapshot=ROOT/'authority_snapshot.json';self.events=[]
    def test_partial_multiple_and_empty_cr_reads(self):
        fake=StrictFakeSerial((b't1702AA',b'55\rt1A01FF\r\r'))
        port=SlcanPort('COM3',115200,serial_port=fake)
        self.assertEqual(port.receive(),[])
        self.assertEqual(port.receive(),[CanFrame(0x170,False,b'\xaa\x55'),CanFrame(0x1A0,False,b'\xff')])
    def test_serial_read_must_be_bytes(self):
        port=SlcanPort('COM3',115200,serial_port=StrictFakeSerial(('not-bytes',)))
        with self.assertRaisesRegex(TypeError,'serial read must return bytes-like'):port.receive()
    def test_physical_start_path_uses_bytes_and_single_cr(self):
        fake=StrictFakeSerial((b'\r',))
        port=SlcanPort('COM3',115200,serial_port=fake);port.open_can()
        sim=EcuSimulator(self.snapshot,'fresh-startup',20260825,port.send,lambda k,d:self.events.append((k,d)))
        commands=queue.Queue();commands.put('START')
        run_iteration(sim,port,commands,lambda k,d:self.events.append((k,d)),100.0)
        self.assertTrue(sim.started);self.assertFalse(sim.failed)
        self.assertEqual(fake.writes[:3],[b'C\r',b'S5\r',b'O\r'])
        self.assertGreater(len(fake.writes),3)
        self.assertTrue(all(isinstance(item,bytes) for item in fake.writes))
        self.assertTrue(all(item.endswith(b'\r') and item.count(b'\r')==1 for item in fake.writes))
        self.assertTrue(any(kind=='can_tx' for kind,_ in self.events))
    def test_failure_details_contains_full_traceback(self):
        try:raise RuntimeError('traceback regression')
        except RuntimeError as exc:details=exception_details(exc)
        self.assertEqual(details['reason'],"RuntimeError('traceback regression')")
        self.assertIn('Traceback (most recent call last)',details['traceback'])
        self.assertIn('RuntimeError: traceback regression',details['traceback'])
        sim=EcuSimulator(self.snapshot,'fresh-startup',1,lambda frame:None,lambda k,d:self.events.append((k,d)))
        sim.fail(details['reason'],details['traceback'])
        self.assertIn('Traceback (most recent call last)',sim.status()['failure_traceback'])
        self.assertIn('traceback',self.events[-1][1])

if __name__=='__main__':unittest.main()