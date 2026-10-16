"""Render-queue lifecycle 408630 -> 407ab0 -> (4128f0/402f40, 407330) against original instructions.

Real source code runs for 408630, 407ab0, 4128f0, 402f40, 407330, 401420, 77ed00,
77ed20, 77ece0, 412400, 401440 and 77ff90. Fixtures: 780f60 (HeapAlloc) returns
the case's heap address, 780fb0 (HeapFree) is a no-op, and the D3D8 device's
SetRenderState (vtable +0xc8) records (site, state, value) and may fail at call N.
Compared: buffer cursor offset and zeroing, overflow flag, colours, captured
words, mode/count stores, bucket sentinels and the render-state trace. FP status
flags are reported separately (Unicorn's x87 flag behaviour is not hardware).
"""
import argparse,ctypes,hashlib,json,math,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
HEAP=0x30000000;BUCKETS=0x31000000;DEVICE=DATA+0x100;VTABLE=DATA+0x400;METHOD=CODE+0x300
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p,n in((HEAP,0x90000),(BUCKETS,0x4000),(0x3ab9000,0x1000),(0x3abd000,0x1000),(0x3b11000,0x1000),(0x3b12000,0x1000),(0x3b18000,0x1000),(0x3d0f000,0x1000),(0x3d12000,0x1000),(0x900000,0x1000),(0x892000,0x1000)):
            try:m.mem_map(p,n)
            except unicorn.UcError:pass
        m.mem_write(METHOD,b'\x31\xc0\xc2\x0c\x00');m.mem_write(VTABLE+0xc8,struct.pack('<I',METHOD));m.mem_write(DEVICE,struct.pack('<I',VTABLE))
        self.clean=m.context_save()
        for a in(0x780f60,0x780fb0,METHOD):m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4)
        if address==METHOD:
            state,value=get(m,esp+8,4),get(m,esp+12,4);self.trace.append([ret-6,state,value])
            if self.c[23] and len(self.trace)==self.c[23]:self.stop=(2,ret-6);m.emu_stop()
            return
        if address==0x780f60:m.reg_write(UC_X86_REG_EAX,self.c[0])
        m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.trace=[];self.stop=None
        m.mem_write(HEAP,b'\xcd'*0x90000);m.mem_write(BUCKETS,bytes(0x4000));put(m,0x3abd960,4,0x12345678);put(m,0x3abd9c8,4,1);put(m,0x90083c,4,BUCKETS);put(m,0x3d128b0,4,DEVICE)
        m.mem_write(0x3b18230,struct.pack('<4I',*c[2:6]));put(m,0x8928c8,4,c[6]);m.mem_write(0x900840,struct.pack('<3I',*c[7:10]));m.mem_write(0x900858,struct.pack('<3I',*c[10:13]))
        put(m,0x3b12208,4,c[13]);put(m,0x3abdbc0,1,c[14]);put(m,0x3ab98ac,4,c[15]);put(m,0x3b1117c,4,c[16]);put(m,0x3abd9d4,4,c[17]);put(m,0x3abd9dc,4,c[18]);m.mem_write(0x3ab9864,struct.pack('<4I',*c[19:23]))
        m.mem_write(0x3d0f7f0,struct.pack('<4I',*[0xdeadbeef]*4));m.mem_write(0x3ab9898,b'\xef'*12);m.mem_write(0x90084c,b'\xef'*12)
        m.reg_write(UC_X86_REG_FPCW,c[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x408630,STOP,count=1000000)
        if not self.stop and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=[0]*80;f,b=self.stop or(0,0);out[0:3]=[f,b,int(self.stop is None)]
        heap=bytes(m.mem_read(c[0],0x88000))
        out[3]=get(m,0x3abd954,4)-c[0] if get(m,0x3abd954,4) else 0;out[4]=int(heap==bytes(0x88000));out[5]=get(m,0x3abd9c8,4)&0xff
        out[6:10]=struct.unpack('<4I',m.mem_read(0x3ab9864,16));out[10:14]=struct.unpack('<4I',m.mem_read(0x3d0f7f0,16))
        out[14:17]=struct.unpack('<3I',m.mem_read(0x3ab9898,12));out[17:20]=struct.unpack('<3I',m.mem_read(0x90084c,12))
        out[20]=get(m,0x3b12208,4);out[21]=get(m,0x3abdbc0,1);out[22]=get(m,0x3ab98ac,4);out[23]=get(m,0x3b1117c,4);out[24]=get(m,0x3abd9d4,4);out[25]=get(m,0x3abd9dc,4)
        out[26]=int(all(get(m,BUCKETS+8*i,4)==BUCKETS+8*i and get(m,BUCKETS+8*i+4,4)==BUCKETS+8*i for i in range(2048)));out[27]=len(self.trace)
        for i,e in enumerate(self.trace[:8]):out[28+3*i:31+3*i]=e
        out[52]=m.reg_read(UC_X86_REG_FPSW)&0x3f
        return out
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures():
    rng=random.Random(0x408630);cases=[];specials=[0,0x80000000,0x3f800000,0xbf800000,0x3f800001,0xbf800001,0x7f800000,0xff800000,0x7fc00000,0x7fa00000,0x00000001,0x3f7fffff]
    for n in range(600):
        c=[0]*24;c[0]=HEAP+rng.choice([0,8,16,24,0x20,0x28]);c[1]=rng.choice([0x7f,0x27f,0x37f,0x47f,0x87f,0xc7f])
        c[2:6]=[rng.choice(specials+[f32(rng.uniform(-3,3))]) for _ in range(4)];c[6]=rng.choice([0,1,3,7,8,0xffffffff])
        c[7:13]=[rng.getrandbits(32) for _ in range(6)];c[13]=rng.getrandbits(32);c[14]=rng.randrange(256);c[15]=rng.choice([0,1,2]);c[16]=rng.choice([0,1,5]);c[17]=rng.randrange(100);c[18]=rng.randrange(100)
        c[19:23]=[f32(rng.uniform(-2,2)) for _ in range(4)];c[23]=0 if n%5 else rng.randrange(1,5);cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    tables=image.read(0x88ad9c,32)+image.read(0x7dcb10,4)+image.read(0x7dcb0c,4)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_queue_lifecycle;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];flag_bad=0;cases=fixtures();outcomes={}
    for n,c in enumerate(cases):
        e=source.run(c);out=(ctypes.c_uint32*80)()
        if fn((ctypes.c_ubyte*40).from_buffer_copy(tables),40,(ctypes.c_uint32*24)(*c),24,out,80)!=1:raise RuntimeError('Native rejected')
        got=list(out);key=f"failure{e[0]}:calls{e[27]}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(80) if i!=52 and got[i]!=e[i]]
        if diff:bad.append(dict(case=n,input=[hex(x) for x in c],fields=diff,source=[hex(x) for x in e[:53]],native=[hex(x) for x in got[:53]]))
        if got[52]!=e[52]:flag_bad+=1
    blob=b''.join(struct.pack('<24I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/engine/ninja_queue_lifecycle.hpp','src/guest/ninja_queue_lifecycle.cpp','src/engine/ninja_queue_consumer.cpp','src/engine/ninja_sprite_queue.cpp','tests/probes/native_ninja_queue_lifecycle_probe.cpp','tools/oracles/verify_original_ninja_queue_lifecycle.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={k:sha((pathlib.Path(__file__).resolve().parents[2]/k).read_bytes()) for k in names},
        original_ranges_sha256={h:sha(image.read(int(h,16),n)) for h,n in(('408630',0x5f),('407ab0',0x50),('407330',0x76),('402f40',0x124),('401420',0xd),('412400',0x17),('77ff90',0x2c))},
        cases=len(cases),case_sha256=sha(blob),source_outcomes=outcomes,state_mismatch_count=len(bad),fp_flag_mismatches=flag_bad,mismatches=bad[:20],stores_trace_boundaries_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad),fp_flag_mismatches=flag_bad,outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
