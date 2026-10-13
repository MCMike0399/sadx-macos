"""Actual403d30/403f60/403e80 allocator/header/sort instructions.

Explicit source buffer/bucket/global fixtures, not full renderer initialization.
The source warning flag is known1 in this oracle; logging failures have distinct
owner tests. No local callee is replaced: original CRT_ftol executes unchanged.
"""
import argparse,ctypes,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,DATA,CODE
from verify_original_task_work import call,sha
ARENA=0x20000000;SIZE=65536;BUCKETS=0x21000000

def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures():
    rng=random.Random(0x403f60403e80);sequences=[]
    for bias in(0,5.5,-50000):
        records=[(flags,0x12345678,word(depth),policy,3)for flags in(0,1,3,4,8,16,255)for depth in(-50000,-40000,-30000,-20000,-10000,-100.9,-100.5,-100.1,0,20000)for policy in(0,1)]
        sequences.append(dict(name='source flag and bucket thresholds',bias=word(bias),records=records))
    for _ in range(5):sequences.append(dict(name='stable random same/different buckets',bias=word(rng.uniform(-100,100)),records=[(rng.randrange(256),rng.getrandbits(32),word(rng.choice([-100.5,-100.1,-100.9,rng.uniform(-60000,30000)])),rng.randrange(2),rng.randrange(16))for i in range(120)]))
    sequences.append(dict(name='buffer capacity nullable return',bias=word(0),records=[(0,i,word(-100.5),0,3)for i in range(430)]))
    sequences.append(dict(name='nonfinite and source NaN ordering',bias=word(0),records=[(flags,i,raw,0,3)for i,raw in enumerate((0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345))for flags in(0,1)]))
    return sequences
class QueueOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x7f);m=self.machine
        for p,n in((ARENA,SIZE),(BUCKETS,16384),(0x3ab9000,0x1000),(0x3abd000,0x1000),(0x3d0f000,0x1000),(0x3b12000,0x1000),(0x900000,0x1000)):m.mem_map(p,n)
        m.mem_write(0x90083c,struct.pack('<I',BUCKETS))
    def run(self,sequence,cw):
        m=self.machine;m.mem_write(ARENA,bytes([0xcd])*SIZE);m.mem_write(BUCKETS,b''.join(struct.pack('<2I',BUCKETS+i*8,BUCKETS+i*8)for i in range(2048)))
        for at,value in((0x3abd954,ARENA),(0x3ab9874,ARENA+SIZE),(0x3abd9c8,1),(0x3ab98ac,0),(0x3abd950,0x12345678),(0x3d0f9c8,11),(0x3d0f840,22),(0x3d0f9c4,33),(0x3b12208,2),(0x3abd9c0,sequence['bias'])):m.mem_write(at,struct.pack('<I',value))
        m.mem_write(0x3abdcfe,b'\x03');m.mem_write(0x3ab9890,b'\xa8');m.mem_write(0x3abd9a8,struct.pack('<4f',1,2,3,4));m.mem_write(0x3ab9864,struct.pack('<4f',5,6,7,8));m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);trace=[];records=[];record_buckets={}
        for flags,eax,depth,policy,kind in sequence['records']:
            m.reg_write(UC_X86_REG_EAX,eax);record=call(m,0x403f60,[152,kind,flags]);bucket=0xffffffff;stored=0
            if record:
                records.append(record);m.reg_write(UC_X86_REG_ESI,record);call(m,0x403e80,[depth,policy]);stored=struct.unpack('<I',m.mem_read(record+4,4))[0]
                # The new record's actual source next edge identifies its
                # bucket: either its sentinel or a previously inserted record.
                next_node=struct.unpack('<I',m.mem_read(record,4))[0]
                if BUCKETS<=next_node<BUCKETS+16384 and(next_node-BUCKETS)%8==0:bucket=(next_node-BUCKETS)//8
                elif next_node in record_buckets:bucket=record_buckets[next_node]
                else:raise RuntimeError('Unexpected original queue edge')
                record_buckets[record]=bucket

            cursor=struct.unpack('<I',m.mem_read(0x3abd954,4))[0]-ARENA;trace.extend((0,0,record,cursor,bucket,stored,m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)))
        return bytes(m.mem_read(ARENA,SIZE)),bytes(m.mem_read(BUCKETS,16384)),trace

def immediate_cases(image,library):
    original=QueueOriginal(image);m=original.machine;m.mem_map(0x3d12000,0x1000);device=DATA+0x100;vtable=DATA+0x200;method=CODE+0x200;m.mem_write(device,struct.pack('<I',vtable));m.mem_write(vtable+0xc8,struct.pack('<I',method));m.mem_write(method,bytes.fromhex('c20c00'));m.mem_write(0x3d128b0,struct.pack('<I',device));trace=[]
    def hook(machine,address,size,user):
        if address==method:
            esp=machine.reg_read(UC_X86_REG_ESP);state,value=struct.unpack('<2I',machine.mem_read(esp+8,8));trace.append((state,value));machine.reg_write(UC_X86_REG_EAX,0xffffffff)
    m.hook_add(unicorn.UC_HOOK_CODE,hook);fn=library.sadx_native_ninja_sprite_queue_immediate;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int;table=struct.unpack('<8I',image.read(0x88ad9c,32));mismatches=[]
    for flags in range(256):
        trace.clear();m.mem_write(0x3abd9a8,struct.pack('<4f',1,2,3,4));m.mem_write(0x3ab9864,struct.pack('<4f',5,6,7,8));call(m,0x404240,[flags]);color=struct.unpack('<4I',m.mem_read(0x3d0f7f0,16));out=(ctypes.c_uint32*24)();status=fn((ctypes.c_uint32*8)(*table),8,flags,out,24)
        if status:raise RuntimeError('Immediate native probe rejected fixture')
        actual=[(out[4+i*3],out[5+i*3])for i in range(out[2])]
        if actual!=trace or tuple(out[16:20])!=color or out[0]:mismatches.append(dict(flags=hex(flags),source_trace=list(trace),native_trace=actual,source_color=list(map(hex,color)),native=list(out)))
    return dict(cases=256,mismatches=mismatches,commands_color_match=not mismatches,limits=['Actual404240/77ed00/77ed20/77ff90, only externalCOMSetRenderState is a declared test boundary','No actual GPU submission or sprite draw claimed'])

def texture_gate_cases(image,library):
    source=QueueOriginal(image);m=source.machine;fn=library.sadx_native_ninja_sprite_queue_texture_gate
    fn.argtypes=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    fixtures=[(0,0,0,0)]+[(1,count,mask,sentinel)for count in range(5)for mask in range(1<<count)for sentinel in(0,1)]
    mismatches=[]
    for present,count,mask,sentinel in fixtures:
        m.mem_write(DATA+0x300,struct.pack('<2I',DATA+0x400,count))
        m.mem_write(DATA+0x400,b''.join(struct.pack('<3I',0xffffffff if sentinel else 0x1234,0,0 if mask&(1<<i)else DATA+0x800+i*4)for i in range(max(1,count))))
        m.reg_write(UC_X86_REG_EAX,DATA+0x300 if present else 0);expected=call(m,0x403250,[]);out=ctypes.c_uint32();status=fn(present,count,mask,sentinel,ctypes.byref(out))
        if status:raise RuntimeError('Native texture gate rejected known fixture')
        if out.value!=expected:mismatches.append(dict(present=present,count=count,null_mask=mask,sentinel=sentinel,source=expected,native=out.value))
    return dict(cases=len(fixtures),mismatches=mismatches,known_gate_match=not mismatches,limits=['Actual403250 executes without callee overlays','Known resource identity/null facts only; no texture upload/cache/GPU readiness claimed'])

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle mismatch')
    cases=fixtures();blob=b''.join(struct.pack('<2I',c['bias'],len(c['records']))+b''.join(struct.pack('<5I',*r)for r in c['records'])for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_sprite_queue;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t];fn.restype=ctypes.c_int;source=QueueOriginal(image);root=pathlib.Path(__file__).resolve().parent.parent
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),case_blob_sha256=sha(blob),source_ranges={hex(x):dict(bytes=y-x,sha256=sha(image.read(x,y-x)))for x,y in((0x403d30,0x403d7d),(0x403f60,0x404024),(0x403e80,0x403f57),(0x644340,0x6443bf),(0x403250,0x40328d),(0x404240,0x40428a),(0x77ed00,0x77ed20),(0x77ed20,0x77ed36),(0x77ff90,0x77ffbc))},native_sources={n:sha((root/n).read_bytes())for n in('src/ninja/ninja_sprite_queue.cpp','src/ninja/ninja_sprite_queue.hpp','src/ninja/ninja_render.cpp','src/ninja/ninja_render.hpp','tests/probes/native_ninja_sprite_queue_probe.cpp','tools/oracles/verify_original_ninja_sprite_queue.py')},profiles=[],limits=['Buffer and initialized2048 bucket sentinels are explicit source-bound fixtures, not claimed full renderer startup','Actual allocator/header/sort/CRT_ftol source executes without overlays','Known warning flag1 suppresses401000 logging in source; missing/unavailable logger cases remain native boundary tests','Canonical pointer bytes exist only in probe snapshot; native runtime uses owned allocations and weak/list references','All FP/nonfinite discrepancies retained; no full GPU/mod/hardware/runtime equality claim'])
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300)for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),sequences=len(cases),records=sum(len(c['records'])for c in cases),state_mismatches=0,bucket_mismatches=0,trace_mismatches=0,flag_mismatches=0,flag_examples=[],examples=[])
        for index,c in enumerate(cases):
            expected_arena,expected_buckets,expected_trace=source.run(c,cw);commands=[v for r in c['records']for v in r];trace=(ctypes.c_uint32*len(expected_trace))();arena=(ctypes.c_ubyte*SIZE)();buckets=(ctypes.c_ubyte*16384)();status=fn((ctypes.c_uint32*len(commands))(*commands),len(commands),cw,c['bias'],trace,len(expected_trace),arena,SIZE,buckets,16384)
            if status:raise RuntimeError(f'Native fixture rejected {status}')
            significant=[i for i,(x,y)in enumerate(zip(trace,expected_trace))if x!=y and i%8!=6];flags=[i for i,(x,y)in enumerate(zip(trace,expected_trace))if x!=y and i%8==6];different=[]
            if bytes(arena)!=expected_arena:profile['state_mismatches']+=1;different.append('state')
            if bytes(buckets)!=expected_buckets:profile['bucket_mismatches']+=1;different.append('buckets')
            if significant:profile['trace_mismatches']+=1;different.append('trace')
            profile['flag_mismatches']+=len(flags)
            for i in flags[:max(0,8-len(profile['flag_examples']))]:profile['flag_examples'].append(dict(sequence=index,record=i//8,source=hex(expected_trace[i]),native=hex(trace[i])))
            if different:profile['examples'].append(dict(sequence=index,name=c['name'],fields=different,trace_examples=[dict(index=i,source=hex(expected_trace[i]),native=hex(trace[i]))for i in significant[:16]],arena_byte_examples=[dict(offset=hex(i),source=x,native=y)for i,(x,y)in enumerate(zip(expected_arena,bytes(arena)))if x!=y][:24],bucket_byte_examples=[hex(i)for i,(x,y)in enumerate(zip(expected_buckets,bytes(buckets)))if x!=y][:24]))
        report['profiles'].append(profile);print(json.dumps({k:v for k,v in profile.items()if k not in ('examples','flag_examples')}),flush=True)
    report['immediate_state_composition']=immediate_cases(image,lib);report['texture_gate']=texture_gate_cases(image,lib);report['state_buckets_trace_control_match']=all(not any(x[k]for k in('state_mismatches','bucket_mismatches','trace_mismatches'))for x in report['profiles']);report['strict_emulator_match']=report['state_buckets_trace_control_match']and report['immediate_state_composition']['commands_color_match']and report['texture_gate']['known_gate_match']and all(not x['flag_mismatches']for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['state_buckets_trace_control_match']or not report['immediate_state_composition']['commands_color_match']or not report['texture_gate']['known_gate_match'])
if __name__=='__main__':raise SystemExit(main())
