"""View setup 4208e0 (with 420860, 403e60, 788240, 402ed0, 402f10/780c00, 77ff60,
77ff70, 402f40, 77ed50 and, on the reset path, 407ab0/407330/4030d0/4128f0) against
original instructions. Fixtures: 4071c0, 7815c0, 403180 (recorded callee, site,
first two arguments) and the D3D8 device's SetRenderState (recorded site, state,
value); one call can fail. x87 status flags are reported separately.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
HEAP=0x30000000;BUCKETS=0x31000000;DEVICE=DATA+0x100;VTABLE=DATA+0x400;METHOD=CODE+0x300
PAGES=(0x892000,0x3abd000,0x3b28000,0x3d0f000,0x3aad000,0x3ab9000,0x3b2c000,0x3ce7000,0x3b18000,0x3b11000,0x3b12000,0x900000,0x3d12000)
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p in PAGES:
            try:m.mem_map(p,0x1000)
            except unicorn.UcError:pass
        m.mem_map(HEAP,0x90000);m.mem_map(BUCKETS,0x4000)
        m.mem_write(METHOD,b'\x31\xc0\xc2\x0c\x00');m.mem_write(VTABLE+0xc8,struct.pack('<I',METHOD));m.mem_write(DEVICE,struct.pack('<I',VTABLE))
        self.clean=m.context_save()
        for a in(0x4071c0,0x7815c0,0x403180,METHOD):m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4)
        if address==METHOD:self.trace.append([0xc8,ret-6,get(m,esp+8,4),get(m,esp+12,4)])
        else:self.trace.append([address,ret-5,get(m,esp+4,4),get(m,esp+8,4)])
        if self.c[15] and len(self.trace)==self.c[15]:self.stopped=True;m.emu_stop();return
        if address!=METHOD:m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.trace=[];self.stopped=False
        for p in PAGES:m.mem_write(p,bytes(0x1000))
        m.mem_write(HEAP,b'\xcd'*0x90000);m.mem_write(BUCKETS,bytes(0x4000))
        put(m,0x8928c0,4,c[2]);put(m,0x8928c4,4,c[3]);put(m,0x8928c8,4,c[14]);put(m,0x3b28094,4,c[4]);put(m,0x3aad0bc,4,c[5]);put(m,0x3b280a8,1,c[1]);put(m,0x3b28114,4,7)
        m.mem_write(0x3b280ac,struct.pack('<4I',*c[6:10]));m.mem_write(0x3b18230,struct.pack('<4I',*c[10:14]))
        put(m,0x3abd960,4,c[16]);put(m,0x3abd958,4,0x88000);put(m,0x90083c,4,BUCKETS);put(m,0x3d128b0,4,DEVICE);put(m,0x3ab9890,1,0)
        m.mem_write(0x900840,struct.pack('<3I',1,2,3));m.mem_write(0x900858,struct.pack('<3I',4,5,6))
        for a in(0x3b28098,0x3b2809c,0x3b280a0,0x3b280a4,0x3d0f9e0,0x3d0f9ec,0x3d0f9f0,0x3ab98ec,0x3b2cbb4,0x3abd9cc,0x3ab98ac,0x3d0f9c8,0x3d0f840,0x3d0f9c4):put(m,a,4,0xdeadbeef)
        m.mem_write(0x3ce7174,b'\xef\xbe\xad\xde'*4);m.mem_write(0x3d0f7f0,b'\xef\xbe\xad\xde'*4)
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x4208e0,STOP,count=2000000)
        if not self.stopped and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=[0]*120;out[1]=int(not self.stopped);out[0]=0 if not self.stopped else -1
        out[2]=get(m,0x3ab98ac,4);out[3]=get(m,0x3b28114,4);out[4]=get(m,0x3abd9cc,4)
        for i,a in enumerate((0x3b28098,0x3b2809c,0x3b280a0,0x3b280a4,0x3d0f9e0,0x3d0f9ec,0x3d0f9f0)):out[5+i]=get(m,a,4)
        out[12]=get(m,0x3aad0bc,4);out[13]=get(m,0x3ab98ec,4);out[14]=get(m,0x3b2cbb4,4)
        for i in range(4):out[15+i]=get(m,0x3ce7174+4*i,4);out[19+i]=get(m,0x3ab9864+4*i,4);out[23+i]=get(m,0x3d0f7f0+4*i,4)
        out[27]=get(m,0x3d0f9c8,4);out[28]=get(m,0x3d0f840,4);out[29]=get(m,0x3d0f9c4,4);out[30]=get(m,0x3ab9890,1);out[31]=len(self.trace)
        for i,e in enumerate(self.trace[:20]):out[32+4*i:36+4*i]=e
        out[112]=m.reg_read(UC_X86_REG_FPSW)&0x3f
        return out
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures():
    rng=random.Random(0x4208e0);cases=[]
    for n in range(500):
        c=[0]*20;c[0]=rng.choice([0x7f,0x27f,0x37f,0xc7f]);c[1]=rng.choice([0,0,1]);c[2]=rng.choice([f32(1.0),f32(2.0),f32(rng.uniform(0.5,4)),0x7f800000,0x7fc00000]);c[3]=rng.choice([f32(1.0),f32(rng.uniform(0.5,4))])
        c[4]=rng.getrandbits(32);c[5]=rng.choice([0x31c7,0,0x2000]);c[6:10]=[f32(rng.uniform(-2,2)) for _ in range(4)];c[10:14]=[f32(rng.uniform(-2,2)) for _ in range(4)]
        c[14]=rng.choice([0,3,7]);c[15]=0 if n%5 else rng.randrange(1,9);c[16]=HEAP;cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    k=b''.join(image.read(x,4) for x in(0x7dcb28,0x7dcb24,0x7dcc98,0x7dd594,0x7dd590,0x7dcb10,0x7dcb0c))+image.read(0x88ad9c,32)+image.read(0x88ad64,88)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_view;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];flags=0;outcomes={}
    for n,c in enumerate(fixtures()):
        e=source.run(c);out=(ctypes.c_uint32*120)()
        if fn((ctypes.c_ubyte*len(k)).from_buffer_copy(k),len(k),(ctypes.c_uint32*20)(*c),20,out,120)!=1:raise RuntimeError('Native rejected')
        got=list(out);got[0]=0 if got[1] else -1;e[0]=e[0]  # compare completion, not failure kind
        key=f"reset{c[1]}:completed{e[1]}:calls{e[31]}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(120) if i!=112 and got[i]!=e[i]]
        if diff:bad.append(dict(case=n,input=[hex(x) for x in c[:17]],fields=diff[:10],source=[hex(e[i]&0xffffffff) for i in diff[:10]],native=[hex(got[i]&0xffffffff) for i in diff[:10]]))
        if got[112]!=e[112]:flags+=1
    names=['src/game/game_view.hpp','src/game/game_view.cpp','tests/probes/native_game_view_probe.cpp','tools/oracles/verify_original_game_view.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256=sha(image.read(0x4208e0,0x114)),cases=500,source_outcomes=outcomes,state_mismatch_count=len(bad),fp_flag_mismatches=flags,mismatches=bad[:20],stores_trace_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=500,mismatches=len(bad),fp_flag_mismatches=flags,outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
