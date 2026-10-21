"""Actual 40f350 plus pinned DLL keyboard event/poll traces, without FP status claims."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
from verify_original_native_input import Source,KEYS,put,get,PAD,STACK,STOP
from pe_image import PEImage
from unicorn.x86_const import UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBP

def fixtures(seed):
    traces=[[(0,13,1),(3,0,0),(0,13,0),(2,0,0)],
            [(0,13,1),(3,0,0),(2,0,0),(3,0,0),(2,0,0),(0,13,0)],
            [(4,255,0),(3,0,0),(2,0,0),(4,0,0)],
            [(0,160,1),(0,69,1),(0,39,1),(3,0,0),(2,0,0),(1,0,0),(2,0,0)]]
    traces += [[(0,key,1),(3,0,0),(2,0,0),(0,key,0),(3,0,0),(2,0,0)] for key in KEYS]
    rng=random.Random(seed)
    for _ in range(40):
        traces.append([(rng.choices([0,1,2,3,4],[7,1,2,3,1])[0],rng.choice(KEYS),rng.randrange(2)) for _ in range(80)])
    return traces

def source_trace(dll,exe,operations,cursor):
    setup=Source(dll,KEYS).m
    setup.reg_write(UC_X86_REG_EBP,STACK);setup.reg_write(UC_X86_REG_ESP,STACK-0x354)
    setup.emu_start(0x1000105e,0x1000131e,count=1000)
    assert setup.reg_read(UC_X86_REG_EIP)==0x1000131e
    pairs=struct.unpack('<204I',setup.mem_read(STACK-0x340,816))
    source=Source(dll,KEYS,exe,dict(zip(pairs[::2],pairs[1::2])));m=source.m
    m.mem_map(0x40f000,4096);m.mem_write(0x40f350,exe.read(0x40f350,74))
    for i,value in enumerate(cursor):put(m,0x3b0e990+4*i,4,value)
    def call(entry):
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(entry,STOP,count=5000)
        assert m.reg_read(UC_X86_REG_EIP)==STOP
    outputs=[]
    for kind,key,down in operations:
        if kind==0:
            m.reg_write(UC_X86_REG_ECX,key);m.reg_write(UC_X86_REG_EDX,down);call(0x10005170)
        elif kind==1:m.emu_start(0x10005852,0x100058fa,count=500)
        elif kind==3:call(0x40f350)
        elif kind==4:put(m,0x1000e944,1,key)
        if kind!=3:
            call(0x10004db0);m.reg_write(UC_X86_REG_ECX,PAD);call(0x10004080)
        row=[get(m,PAD+a,4) for a in (0,4,8,12,16,20)]
        row += [get(m,PAD+a,2) for a in (24,26)]
        row += [int.from_bytes(m.mem_read(PAD+a,2),'little',signed=True)&0xffffffff for a in (28,30,32,34)]
        row += [get(m,PAD+a,4) for a in (44,68,72)]
        row += list(m.mem_read(0x3b0e3e0,768))+[get(m,0x3b0e3b8,1)]
        row += [get(m,0x3b0e990+4*i,4) for i in range(5)]
        outputs.append(row)
    return outputs

def main():
    p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('exe',type=pathlib.Path);p.add_argument('probe',type=pathlib.Path);p.add_argument('--seed',type=int,default=20261021);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    dll,exe=PEImage(a.dll),PEImage(a.exe)
    assert hashlib.sha256(a.dll.read_bytes()).hexdigest()=='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
    assert hashlib.sha256(a.exe.read_bytes()).hexdigest()=='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_input_reset_trace
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    lib=ctypes.CDLL(str(a.probe.resolve()))
    seeded=lib.sadx_native_input_reset_seeded
    seeded.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    m=Source(dll,KEYS,exe).m
    m.mem_map(0x40f000,4096);m.mem_write(0x40f350,exe.read(0x40f350,74))
    seeded_bad=[];rng=random.Random(a.seed)
    for case in range(512):
        values=[rng.randrange(256) for _ in range(771)]+[rng.randrange(2**32) for _ in range(5)]
        m.mem_write(0x3b0e3e0,bytes(values[:768]));m.mem_write(0x3b0e3b8,bytes(values[768:771]))
        for i,value in enumerate(values[771:]):put(m,0x3b0e990+4*i,4,value)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40f350,STOP,count=5000)
        assert m.reg_read(UC_X86_REG_EIP)==STOP
        expected=list(m.mem_read(0x3b0e3e0,768))+list(m.mem_read(0x3b0e3b8,3))
        expected += [get(m,0x3b0e990+4*i,4) for i in range(5)]
        output=(ctypes.c_uint32*776)();seeded((ctypes.c_uint32*776)(*values),output)
        if list(output)!=expected:seeded_bad.append(case)
    mismatches=[];frames=0;resets=0;rng=random.Random(a.seed)
    traces=fixtures(a.seed)
    for index,operations in enumerate(traces):
        cursor=[rng.randrange(-2**31,2**31) for _ in range(5)]
        expected=source_trace(dll,exe,operations,cursor)
        output=(ctypes.c_uint32*(len(operations)*789))()
        flat=[v for op in operations for v in op]
        assert fn((ctypes.c_uint32*24)(*KEYS),(ctypes.c_uint32*len(flat))(*flat),len(operations),(ctypes.c_int32*5)(*cursor),output)==1
        for frame,row in enumerate(expected):
            actual=list(output[frame*789:(frame+1)*789])
            if actual!=row:
                fields=[i for i,(x,y) in enumerate(zip(actual,row)) if x!=y]
                mismatches.append({'trace':index,'frame':frame,'fields':fields[:20],'native':[actual[i] for i in fields[:20]],'source':[row[i] for i in fields[:20]]})
        frames+=len(operations);resets+=sum(op[0]==3 for op in operations)
    result={'source_ranges':{'0x40f350-0x40f39a':{'sha256':hashlib.sha256(exe.read(0x40f350,74)).hexdigest()}},'seed':a.seed,'seeded_cases':512,'seeded_mismatches':len(seeded_bad),'traces':len(traces),'frames':frames,'resets':resets,'mismatches':len(mismatches),'first':mismatches[:10],'values_match':not mismatches and not seeded_bad,'limits':['FP exception flags are outside this reset-only integer/store claim; underlying native_input remains FP-partial.','Physical backend unchanged; no new hardware qualification.']}
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(mismatches or seeded_bad)
if __name__=='__main__':raise SystemExit(main())
