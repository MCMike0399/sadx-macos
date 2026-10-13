"""Actual442750 source prefix with real task/work/object/event callees.

Stops before required auxiliary-effect callback binding in this prefix fixture at44297b, or earlier
explicit ownership boundary. No callee callback is substituted or executed.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP
from verify_original_task_work import ARENA,SIZE,GLOBALS,call,sha
MAIN=ARENA+32;PARAMETERS=0x9154e8;EVENTS=0x3b18808
SEED=0x442750417e80
FIRST_EVENT={0:0x44281a,2:0x442859,3:0x44288c,5:0x4428bf,6:0x4428eb,7:0x442915}
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class StartupOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x27f);m=self.machine;m.mem_map(ARENA,0x1001000)
        for page in(GLOBALS,0x38f6000,0x915000,0x3b18000,0x3b42000,0x3b36000,0x3b3c000):m.mem_map(page,0x1000)
        m.reg_write(UC_X86_REG_EDX,ARENA);call(m,0x40b1ce,[])
        m.mem_write(STACK,struct.pack('<2I',0,STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40b466,STOP,count=100000)
        self.initial=bytes(m.mem_read(ARENA,SIZE));self.initial_globals=bytes(m.mem_read(GLOBALS,0x1000));self.current=None;self.stop=0;self.player=0;self.mask=0;self.event_reads=[]
        def code(machine,address,size,user):
            w=self.current
            if not w:return
            if address==0x44276f:self.player=machine.reg_read(UC_X86_REG_EAX)
            selected=None
            if not w[8]:selected=0x44276f
            elif w[1]>=8 or not w[5]:selected=0x44278c
            elif not w[9]:selected=0x442793
            elif w[0]>=8:selected=0x4427b1
            elif not w[3]:selected=0x4427bf
            elif not w[4] and w[1]in FIRST_EVENT:selected=FIRST_EVENT[w[1]]
            elif w[2]>255 and w[10]:selected=0x44294c
            if address==selected or address==0x44297b:
                self.stop=address;machine.emu_stop();return
            if address==0x442781 and self.player==0:
                self.stop=address;machine.emu_stop();return
            if address==0x417e9b and machine.reg_read(UC_X86_REG_EAX)==0:
                self.stop=address;machine.emu_stop();return
            if address in(0x4427b1,0x4427b8,0x4427bf,0x4427c6):self.mask|=1<<((0x4427b1,0x4427b8,0x4427bf,0x4427c6).index(address))
            if address==0x412d20:self.event_reads.append(struct.unpack('<I',machine.mem_read(machine.reg_read(UC_X86_REG_ESP)+4,4))[0])
        m.hook_add(unicorn.UC_HOOK_CODE,code)
    def run(self,w,event_bytes,parameter_bytes,cw):
        m=self.machine;self.current=None;m.mem_write(ARENA,self.initial);m.mem_write(GLOBALS,self.initial_globals)
        for page in(0x3b42000,0x3b36000,0x3b3c000):m.mem_write(page,bytes(0x1000))
        main=call(m,0x40b860,[3,1,0])
        if main!=MAIN:raise RuntimeError('Main task pool fixture identity')
        primary,motion=struct.unpack('<2I',m.mem_read(MAIN+32,8))
        twp=bytearray(64);twp[2]=w[2]&255;struct.pack_into('<H',twp,4,w[11]);twp[8]=w[0]&255;twp[9]=w[1];struct.pack_into('<9I',twp,0x14,*w[12:21]);m.mem_write(primary,bytes(twp))
        mwp=bytearray(64);struct.pack_into('<6I',mwp,4,*w[21:27]);struct.pack_into('<I',mwp,0x20,w[27]);struct.pack_into('<3I',mwp,0x34,*w[28:31]);m.mem_write(motion,bytes(mwp))
        if w[6]:
            existing=call(m,0x40b2d0,[1,124]);m.mem_write(existing,b'\x09');m.mem_write(primary+0x3c,struct.pack('<I',existing))
        if w[7]:
            head=struct.unpack('<I',m.mem_read(0x3abdbe8,4))[0];remaining=ARENA+0x1000000-head;allocated=call(m,0x40b220,[remaining-w[7]-32])
            if not allocated:raise RuntimeError('Arena capacity fixture failed')
        if not w[8]:m.mem_write(MAIN+32,bytes(4))
        if not w[9]:m.mem_write(MAIN+36,bytes(4))
        params=bytearray(parameter_bytes)
        if w[31] and w[1]<8:struct.pack_into('<I',params,w[1]*132+0x70,w[32])
        m.mem_write(PARAMETERS,bytes(params));m.mem_write(EVENTS,event_bytes)
        self.current=w;self.stop=0;self.player=0;self.mask=0;self.event_reads=[]
        m.mem_write(STACK,struct.pack('<3I',STOP,w[0],MAIN));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(0x442750,STOP,count=200000)
        if not self.stop:raise RuntimeError('Source unexpectedly passed required auxiliary-effect owner boundary')
        player=self.player;obj=struct.unpack('<I',m.mem_read(primary+0x3c,4))[0];contact=struct.unpack('<I',m.mem_read(player+0x64,4))[0]if player else 0;extra=struct.unpack('<I',m.mem_read(player+0x78,4))[0]if player else 0
        snapshot=bytearray(b'\xcd'*1376)
        for address,at,size in((primary,0,64),(motion,64,64),(player,128,512),(obj,640,124),(contact,764,132),(extra,896,480)):
            if address:snapshot[at:at+size]=m.mem_read(address,size)
        return dict(snapshot=bytes(snapshot),boundary=self.stop,mask=self.mask,player=player,contact=contact,extra=extra,object=obj,motion_player=struct.unpack('<I',m.mem_read(motion,4))[0],free_head=struct.unpack('<I',m.mem_read(0x3abdbe8,4))[0],free_slots=4095,cw=m.reg_read(UC_X86_REG_FPCW),flags=m.reg_read(UC_X86_REG_FPSW)&63,event_reads=self.event_reads)
def fixtures():
    cases=[];rng=random.Random(SEED)
    def add(name,row=0,index=0,events=0,**changes):
        w=[index&0xffffffff,row,0,1,1,1,0,0,1,1,1,0x1234,111,222,333,*map(word,(3,4,5)),*map(word,(7,8,9)),*map(word,(9,8,7)),*map(word,(1,2,3)),0x12345678,*map(word,(.25,.5,.75)),0,0]
        for k,v in changes.items():w[int(k[1:])]=v
        if events==0:ev=bytes(512)
        elif events==1:ev=bytes([2])*512
        else:ev=bytes(rng.getrandbits(8)if rng.randrange(3)==0 else 0 for _ in range(512))
        cases.append(dict(name=name,words=w,events=ev))
    for row in range(8):
        for index in(0,1,7):
            for events in(0,1,2):add('source rows/registration/events',row,index,events)
    for _ in range(70):add('live task/motion facts',rng.randrange(8),rng.randrange(8),2,w2=rng.randrange(1,256),w6=rng.randrange(2),w10=rng.randrange(2),w11=rng.randrange(65536),w12=rng.getrandbits(32),w13=rng.getrandbits(32),w14=rng.getrandbits(32))
    for raw in(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345):add('radius source-f32 special',w31=1,w32=raw)
    add('missing events prefix',w4=0);add('no source events needed',row=1,w4=0)
    add('unknown task ID stays unknown',row=1,w2=256,w10=1)
    add('missing live motion registration prefix',row=1,w3=0)
    add('unknown parameters',w5=0);add('unsupported row',row=8)
    add('unsupported registration',row=1,index=8);add('unsupported negative registration',row=1,index=-1)
    add('null primary after allocation',w8=0);add('null motion after parameter copy',w9=0)
    add('player calloc null',w7=32)
    add('object calloc null after motion binding',w7=544)
    add('nullable contact and auxiliary',row=1,w6=1,w7=544)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('RequiresUnicorn2.1.4')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity changed')
    parameters=image.read(PARAMETERS,1056);cases=fixtures();blob=b''.join(struct.pack('<33I',*c['words'])+c['events']for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_player_startup;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t]*2+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t];fn.restype=ctypes.c_int;param=(ctypes.c_ubyte*1056).from_buffer_copy(parameters)
    root=pathlib.Path(__file__).resolve().parents[2]
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),parameter_table_sha256=sha(parameters),case_blob=dict(path=str(a.output.with_suffix('.cases.bin')),sha256=sha(blob),cases=len(cases),record='33 LE uint32words +512 exact event bytes'),source_functions={hex(x):dict(size=y-x,sha256=sha(image.read(x,y-x)))for x,y in((0x442750,0x442993),(0x40b2d0,0x40b303),(0x417e80,0x417ea0),(0x412d20,0x412d30))},native_sources={n:sha((root/n).read_bytes())for n in('src/player/player_startup.cpp','src/player/player_startup.hpp','src/player/player_state.cpp','src/engine/task.cpp','src/player/player_task.hpp','src/player/player_task.cpp','src/engine/task_work.cpp','tests/probes/native_player_startup_probe.cpp','tools/oracles/verify_original_player_startup.py')},profiles=[],mismatches=[],limits=['Actual source prefix only: stop before44297b because native4a2b60 owner remains missing; no original return or running game claim','Heap/task-pool initialization is an explicit fixture after unlifted CRT/Win32 acquisition','No source task/work/object/event callees are replaced','Canonical probe snapshot reads authoritative typed task/player/motion views; allocated bytes are initialization provenance','Caller-selected parameter/save/FP/resource fixtures, not actual initialized mods or hardware captures','Source crash/corrupt-registration/unknown-owner cases stop before the unsafe instruction and retain native partial failures'])
    original=StartupOriginal(image)
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300)for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,boundary_mismatches=0,resource_mismatches=0,control_mismatches=0,producer_fact_mismatches=0,flag_mismatches=0,examples=[],flag_examples=[])
        for index,c in enumerate(cases):
            w=c['words'];expected=original.run(w,c['events'],parameters,cw);out=(ctypes.c_uint32*24)();snapshot=(ctypes.c_ubyte*1376)();status=fn(param,1056,(ctypes.c_ubyte*512).from_buffer_copy(c['events']),512,(ctypes.c_uint32*33)(*w),33,cw,out,24,snapshot,1376)
            if status:raise RuntimeError(f'Native probe failedcase{index}:status{status}')
            actual=list(out);boundary=actual[2]if actual[0]==4 and actual[2]in(0x417e85,0x417e95,0x417e9b)else actual[1];different=[]
            for field,x,y in(('state',expected['snapshot'],bytes(snapshot)),('boundary',expected['boundary'],boundary),('resource',[expected[k]for k in('mask','player','contact','extra','motion_player','object','free_slots','free_head')],[actual[6],*actual[9:14],actual[15],actual[16]]),('producer_fact',[(1|(expected['snapshot'][0]<<8))if actual[19]else 0,15 if expected['player']else 0,0,3 if expected['player']else 0],actual[20:24]),('control',expected['cw'],actual[8]),('flag',expected['flags'],actual[7])):
                if x!=y:profile[field+'_mismatches']+=1;different.append(field)
            if any(d!='flag'for d in different):
                byte_diffs=[j for j,(x,y)in enumerate(zip(expected['snapshot'],bytes(snapshot)))if x!=y]
                item=dict(case=index,name=c['name'],fields=different,source_boundary=hex(expected['boundary']),native=actual,byte_examples=[dict(offset=hex(j),source=expected['snapshot'][j],native=snapshot[j])for j in byte_diffs[:32]],source_event_reads=list(map(hex,expected['event_reads'])),source_state_sha256=sha(expected['snapshot']),native_state_sha256=sha(bytes(snapshot)));profile['examples'].append(item);report['mismatches'].append(dict(cw=hex(cw),**item))
            if 'flag'in different and len(profile['flag_examples'])<5:profile['flag_examples'].append(dict(case=index,source=hex(expected['flags']),native=hex(actual[7])))
        report['profiles'].append(profile);print(json.dumps({k:v for k,v in profile.items()if k not in('examples','flag_examples')}),flush=True)
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['Flag difference: the native x87 model sets the precision bit (PE 0x20) of the status word on inexact results; Unicorn 2.1.4 does not raise x87 exception bits for these instructions (source 0x0). All compared values match; the hardware flags need the Intel capture (M4).']
    report['prefix_state_resources_boundaries_control_match']=all(not any(p[k]for k in('state_mismatches','boundary_mismatches','resource_mismatches','control_mismatches','producer_fact_mismatches'))for p in report['profiles']);report['original_return_observed']=False;report['full_emulator_match']=report['prefix_state_resources_boundaries_control_match']and all(not p['flag_mismatches']for p in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['prefix_state_resources_boundaries_control_match'])
if __name__=='__main__':raise SystemExit(main())
