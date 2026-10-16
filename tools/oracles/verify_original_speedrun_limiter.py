"""Speedrun 2.0 frame limiter (sadx-speedrun-mod.dll +0x10f0, the live 7899e0 hook) against its original instructions.

The pinned DLL runs at its preferred base with its own __allmul/__alldiv and
security-cookie code. Only the kernel32 imports are fixtures: the QPF slot
(+0x1000c064) writes a scripted frequency or fails, and the QPC slot
(+0x1000c060) returns start + k*step. sonic.exe globals 3b11188 (FrameMultiplier),
3d08538 (frequency) and 3d08550 (last counter) are mapped data. Cases are chosen
to finish within the read cap; native must make the same number of counter reads.
This is the wait condition and stores only, not host wall-clock pacing.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn import UC_PROT_ALL,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
sha=lambda b:hashlib.sha256(b).hexdigest()
SPEEDRUN_SHA256='f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802'
STUB=0x20000000;QPF=STUB;QPC=STUB+0x10;STOP=STUB+0x100;STACK=0x21000000+0xf000
def put(m,a,size,v):m.mem_write(a,(v&((1<<(8*size))-1)).to_bytes(size,'little'))
def get(m,a,size):return int.from_bytes(m.mem_read(a,size),'little')
def s64(v):v&=(1<<64)-1;return v-(1<<64) if v>>63 else v
class Source:
    def __init__(self,dll):
        m=self.m=unicorn.Uc(UC_ARCH_X86,UC_MODE_32)
        for s in dll.sections:
            base=dll.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,dll.slice(s['file_offset'],min(s['raw_size'],size)))
        for base in(STUB,0x21000000,0x3b11000,0x3d08000):m.mem_map(base,0x10000 if base in(STUB,0x21000000) else 0x1000,UC_PROT_ALL)
        m.mem_write(QPF,b'\xc2\x04\x00');m.mem_write(QPC,b'\xc2\x04\x00');put(m,0x1000c064,4,QPF);put(m,0x1000c060,4,QPC)
        m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=STUB,end=STUB+0x20);self.clean=m.context_save()
    def code(self,m,address,size,user):
        c=self.c;esp=m.reg_read(UC_X86_REG_ESP);arg=get(m,esp+4,4)
        if address==QPF:
            if c[2]:put(m,arg,4,c[0]);put(m,arg+4,4,c[1])
            m.reg_write(UC_X86_REG_EAX,1 if c[2] else 0)
        elif address==QPC:
            if self.reads>=c[11]:self.capped=True;m.emu_stop();return
            start=(c[8]<<32)|c[7];step=(c[10]<<32)|c[9];v=(start+step*self.reads)&((1<<64)-1);self.reads+=1
            put(m,arg,8,v);m.reg_write(UC_X86_REG_EAX,1)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.reads=0;self.capped=False
        put(m,0x3b11188,4,c[3]);put(m,0x1001001c,4,c[4]);put(m,0x3d08550,4,c[5]);put(m,0x3d08554,4,c[6]);put(m,0x3d08538,4,0xa5a5a5a5);put(m,0x3d0853c,4,0x5a5a5a5a)
        put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x100010f0,STOP,count=200000)
        except unicorn.UcError as e:
            if e.errno!=unicorn.UC_ERR_EXCEPTION:raise
            return [4,0x1000113d,0,c[5],c[6],get(m,0x3d08538,4),get(m,0x3d0853c,4),0]  # divide fault inside __alldiv
        if self.capped:return None
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        return [0,0,self.reads,get(m,0x3d08550,4),get(m,0x3d08554,4),get(m,0x3d08538,4),get(m,0x3d0853c,4),1]
def fixtures():
    cases=[]
    def interval(freq,fm,cad):
        p=s64(((fm&0xffffffff)-(1<<32 if fm>>31 else 0))*freq)
        if cad==0:return None
        q=abs(p)//abs(cad);return q if (p<0)==(cad<0) else -q
    for freq in(10_000_000,3_579_545,1,(1<<32)+5,0):
        for fm in(1,2,3,0,0xffffffff,0x80000000):
            for cad in(63,60,1,7,0xffffffff,0):
                for last,start in((0,0),(1000,999),(0xfffffffffffffff0,0x10),(5,0x7fffffffffffffff),(1<<40,(1<<40)+3)):
                    for qpf in(1,0):
                        csigned=cad-(1<<32) if cad>>31 else cad;iv=interval(freq,fm,csigned)
                        step=max(1,(abs(iv)//4) if iv is not None else 1)
                        c=[0]*16;c[0]=freq&0xffffffff;c[1]=freq>>32;c[2]=qpf;c[3]=fm;c[4]=cad;c[5]=last&0xffffffff;c[6]=last>>32;c[7]=start&0xffffffff;c[8]=start>>32;c[9]=step&0xffffffff;c[10]=(step>>32)&0xffffffff;c[11]=64
                        cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('speedrun_dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    dll=PEImage(a.speedrun_dll)
    if dll.sha256!=SPEEDRUN_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned mod/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_speedrun_limiter;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(dll);bad=[];outcomes={};used=[];skipped=0
    for c in fixtures():
        expected=source.run(c)
        if expected is None:skipped+=1;continue
        used.append(c);out=(ctypes.c_uint32*12)()
        if fn((ctypes.c_uint32*16)(*c),16,out,12)!=1:raise RuntimeError('Native fixture rejected')
        got=list(out)[:8]
        if expected[0]==4:got[1]=got[1];  # native divide_fault reports the __alldiv call site
        key=f"failure{expected[0]}:reads{min(expected[2],9)}";outcomes[key]=outcomes.get(key,0)+1
        if got!=expected:bad.append(dict(input=c[:12],source=expected,native=got))
    blob=b''.join(struct.pack('<16I',*c) for c in used);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/engine/speedrun.hpp','src/engine/speedrun.cpp','src/engine/game_frame_control.hpp','src/engine/game_frame_control.cpp','src/engine/game_frame_control_internal.hpp','src/engine/frame_control_fwd.hpp','src/guest/frame_control_reference.hpp','src/guest/frame_control_reference.cpp','tests/probes/native_speedrun_limiter_probe.cpp','tools/oracles/verify_original_speedrun_limiter.py']
    report=dict(speedrun_dll_sha256=dll.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},limiter_bytes_sha256=sha(dll.read(0x100010f0,0xd0)),
        cases=len(used),capped_fixtures_excluded=skipped,case_sha256=sha(blob),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:30],reads_stores_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(used),excluded=skipped,mismatches=len(bad),outcomes=outcomes)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
