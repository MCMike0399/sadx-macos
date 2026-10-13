"""Audio streaming init 410510 against original instructions.

Fixtures: operator new 6443e7 (returns a mapped 4-byte object), the 44de80
device init (records this/window/format arguments, checks the 513e60-zeroed
device field, returns the case HRESULT; thiscall RET 0x14), and the
CreateEventA / CreateThread / InitializeCriticalSection import slots
(0x7db098/94/90). SEH frames use fs:[0] at linear page 0. Compared: return
value, init flag, the four 36-word tables, thread handle/id, the critical
section call, device-call count and its arguments. The 40ff50 stream thread
body and the real DirectSound device are not part of this comparison.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
OBJECT=0x32000000;EVENT=CODE+0x400;THREAD=CODE+0x410;CS=CODE+0x420
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for p,n in((0,0x1000),(OBJECT,0x1000),(0x3a8b000,0x1000),(0x3b0e000,0x1000)):m.mem_map(p,n)
        m.mem_protect(0x7db000,0x1000,unicorn.UC_PROT_READ|unicorn.UC_PROT_WRITE)
        for slot,stub,pop in((0x7db098,EVENT,0x10),(0x7db094,THREAD,0x18),(0x7db090,CS,0x4)):m.mem_write(stub,b'\xc2'+struct.pack('<H',pop));put(m,slot,4,stub)
        self.clean=m.context_save()
        for a in(0x6443e7,0x44de80,EVENT,THREAD,CS):m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=a,end=a)
    def code(self,m,address,size,user):
        c=self.c;esp=m.reg_read(UC_X86_REG_ESP);ret=get(m,esp,4)
        if address==0x6443e7:m.reg_write(UC_X86_REG_EAX,OBJECT);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
        elif address==0x44de80:
            this=m.reg_read(UC_X86_REG_ECX);args=[get(m,esp+4+4*i,4) for i in range(5)];self.device_calls+=1;self.window=args[0]
            self.format=args[3]|args[2]<<20|args[4]<<24|args[1]<<28
            if this!=OBJECT or get(m,OBJECT,4)!=0:raise RuntimeError('44de80 object not the constructed one')
            m.reg_write(UC_X86_REG_EAX,c[2]);m.reg_write(UC_X86_REG_ESP,esp+4+0x14);m.reg_write(UC_X86_REG_EIP,ret)
        elif address==EVENT:m.reg_write(UC_X86_REG_EAX,c[3]+self.events);self.events+=1
        elif address==THREAD:put(m,get(m,esp+0x18,4),4,c[5]);m.reg_write(UC_X86_REG_EAX,c[4])
        elif address==CS:self.cs=int(get(m,esp+4,4)==0x3b0ecc4)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.events=0;self.device_calls=0;self.window=0;self.format=0;self.cs=0
        put(m,OBJECT,4,0xcdcdcdcd);put(m,0x3b0ef2c,4,c[0]);put(m,0x3a8b088,4,c[1])
        for base in(0x3b0ec30,0x3b0ece0,0x3b0ee98,0x3b0ed70):m.mem_write(base,b'\xab'*144)
        put(m,0x3b0ee00,4,0xabababab);put(m,0x3b0ec28,4,0xabababab);put(m,0x3b0ecc0,4,0xabababab)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x410510,STOP,count=20000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source return ABI')
        out=[0]*160;out[1]=m.reg_read(UC_X86_REG_EAX);out[2]=get(m,0x3b0ef2c,4);out[3]=int(get(m,0x3b0ecc0,4)==OBJECT)
        out[4]=self.device_calls;out[5]=self.window if self.device_calls else 0;out[6]=self.format if self.device_calls else 0
        out[7]=get(m,0x3b0ee00,4) if get(m,0x3b0ee00,4)!=0xabababab else 0;out[8]=get(m,0x3b0ec28,4) if get(m,0x3b0ec28,4)!=0xabababab else 0;out[9]=self.cs
        for i in range(36):out[10+i]=get(m,0x3b0ec30+4*i,4);out[46+i]=get(m,0x3b0ece0+4*i,4);out[82+i]=get(m,0x3b0ee98+4*i,4);out[118+i]=get(m,0x3b0ed70+4*i,4)
        return out
def fixtures():
    rng=random.Random(0x410510);cases=[]
    for init in(0,1,2,0xffffffff):
        for hr in(0,1,0x7fffffff,0xffffffff,0x88780078,0x80004005):
            for _ in range(8):cases.append([init,rng.getrandbits(32),hr,rng.getrandbits(24)<<4,rng.getrandbits(32),rng.getrandbits(32),0,0])
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_audio_stream;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];cases=fixtures()
    for n,c in enumerate(cases):
        e=source.run(c);out=(ctypes.c_uint32*160)()
        if fn((ctypes.c_uint32*8)(*c),8,out,160)!=1:raise RuntimeError('Native rejected')
        got=list(out);diff=[i for i in range(160) if got[i]!=e[i]]
        if diff:bad.append(dict(case=n,input=[hex(x) for x in c],fields=diff[:10],source=[hex(e[i]) for i in diff[:10]],native=[hex(got[i]) for i in diff[:10]]))
    names=['src/game/audio_stream.hpp','src/game/audio_stream.cpp','tests/probes/native_audio_stream_probe.cpp','tools/oracles/verify_original_audio_stream.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={k:sha((pathlib.Path(__file__).resolve().parents[2]/k).read_bytes()) for k in names},
        original_ranges_sha256={'410510':sha(image.read(0x410510,0x113)),'513e60':sha(image.read(0x513e60,9))},cases=len(cases),mismatch_count=len(bad),mismatches=bad[:20],stores_owner_calls_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad))));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
