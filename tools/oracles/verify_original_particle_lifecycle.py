"""Compare the complete particle pool lifecycle and animation leaf to pinned PC code.

Only DisplayParticle, custom updates and FreeMemory are intercepted. Real native
scheduler/arena destruction and the unbound renderer stop are checked separately.
"""
from collections import Counter
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
import unicorn as uc
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from pe_image import PEImage
TASK, WORK, REPLACEMENT = 0x21000000, 0x22000000, 0x22000100
FREE, ACTIVE, PARTICLES = 0x23000000, 0x23001000, 0x23002000
FRAMES, CUSTOM, STOP, STACK = 0x24000000, 0x25000000, 0x26000000, 0x27000800
DRAW, RELEASE = 0x4cc540, 0x40b310
RANGES = [(0x4cc040, 63), (0x4cc080, 203), (0x4cc150, 131), (0x4cc1e0, 42), (0x4cc470, 142)]
MASK = 0xffffffff

def pack(v): return struct.pack('<' + 'I' * len(v), *(x & MASK for x in v))
def words(v): return list(struct.unpack('<' + 'I' * (len(v)//4), v))
def bits(v): return words(struct.pack('<f', v))[0]

class Source:
    def __init__(self, image):
        assert image.sha256 == '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
        self.ranges = {f'{a:#x}-{a+n:#x}': hashlib.sha256(image.read(a,n)).hexdigest() for a,n in RANGES}
        self.m = m = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
        for p in {a & ~4095 for a,n in RANGES} | {TASK, WORK, FREE, ACTIVE, PARTICLES, FRAMES, CUSTOM, STOP, STACK & ~4095, DRAW & ~4095, RELEASE & ~4095}:
            m.mem_map(p,4096)
        m.mem_map(PARTICLES+4096,4096)
        for a,n in RANGES: m.mem_write(a,image.read(a,n))
        m.hook_add(uc.UC_HOOK_CODE,self.instruction)
    def word(self,a): return words(bytes(self.m.mem_read(a,4)))[0]
    def put(self,a,v): self.m.mem_write(a,pack([v]))
    def count(self,a): return self.word(a) & 65535
    def event(self,kind,identity,frames=0):
        self.events.extend([kind,identity,frames,self.count(WORK),self.count(REPLACEMENT)])
    def reenter(self):
        self.calls += 1
        if self.calls != 1: return
        if self.case[5] & 1: self.put(WORK,self.word(WORK) & 0xffff0000)
        if self.case[5] & 2:
            self.put(WORK,self.word(WORK) & 0xffff0000)
            self.put(TASK+44,REPLACEMENT)
        if self.case[5] & 4:
            free,active=self.word(WORK+4),self.word(WORK+8)
            self.put(WORK+4,active);self.put(WORK+8,free)
    def instruction(self,m,a,size,user):
        if a not in (DRAW,CUSTOM,RELEASE): return
        sp=m.reg_read(UC_X86_REG_ESP)
        result=0
        if a==DRAW:
            p=self.word(sp+4)
            assert self.word(sp+8)==p and self.word(sp+12)==(p+32)&MASK
            self.event(2,p,self.word(sp+16)); self.reenter()
        elif a==CUSTOM:
            p=self.word(sp+4)
            assert self.word(sp+8)==WORK+20
            self.event(1,p,self.word(sp+12))
            self.events.extend([self.word(WORK+20),self.word(WORK+24)])
            self.reenter()
            result=0 if self.case[4] & (1 << ((p-(0 if self.case[3]&4 else PARTICLES))//48)) else 7
        else:
            pointer=self.word(sp+4)
            self.event(3,{0:0,FREE:1,ACTIVE:2,PARTICLES:3}[pointer]);self.reenter()
        m.reg_write(UC_X86_REG_EAX,result)
        m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,case):
        self.case=case;self.calls=0;self.events=[]
        self.particle_base=PARTICLES+4096-(case[6]-1) if case[6] else PARTICLES
        m=self.m
        m.mem_protect(PARTICLES+4096,4096,uc.UC_PROT_ALL)
        m.mem_write(TASK,bytes(52));self.put(TASK+44,0 if case[3]&32 else WORK)
        pointers=[0 if case[3] & (1<<i) else p for i,p in enumerate((FREE,ACTIVE,self.particle_base))]
        body=[case[2],*pointers,0 if case[3]&16 else CUSTOM,case[8],case[9],0 if case[3]&8 else FRAMES]
        m.mem_write(WORK,pack(body));m.mem_write(REPLACEMENT,pack([(case[2]&0xffff0000)|3,*body[1:]]))
        m.mem_write(FREE,pack(case[16:24]));m.mem_write(ACTIVE,pack(case[24:32]));m.mem_write(self.particle_base,pack(case[32:80]));m.mem_write(FRAMES,pack(case[80:124]))
        for r in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):m.reg_write(r,0)
        m.reg_write(UC_X86_REG_FPCW,case[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,65535)
        m.reg_write(UC_X86_REG_ESP,STACK)
        op=case[0];entry=[0x4cc080,0x4cc150,0x4cc040,0x4cc1e0,0x4cc470][op]
        m.mem_write(STACK,pack([STOP,self.particle_base+32 if op==4 else TASK,body[-1]]))
        boundary=field=result=0
        if case[6]:m.mem_protect(PARTICLES+4096,4096,uc.UC_PROT_READ)
        try:
            m.emu_start(entry,STOP,count=30000)
            if m.reg_read(UC_X86_REG_EIP)!=STOP: raise RuntimeError('source instruction budget exhausted')
            if op==4:result=m.reg_read(UC_X86_REG_EAX)
        except uc.UcError:
            pc=m.reg_read(UC_X86_REG_EIP)
            stores={0x4cc0b6:0,0x4cc0bf:4,0x4cc0c8:8,0x4cc0d4:12,0x4cc0e3:16,0x4cc0ef:20,0x4cc49c:32,0x4cc4bc:40,0x4cc4d3:44,0x4cc4f5:40,0x4cc4fb:32}
            if case[6] and pc in stores:boundary,field=5,stores[pc]
            elif case[3]&32 and pc in (0x4cc08c,0x4cc15c,0x4cc04b,0x4cc1e8):boundary=1
            elif case[3]&2 and pc in (0x4cc099,0x4cc168,0x4cc053,0x4cc116,0x4cc19e,0x4cc128,0x4cc1b0):boundary=3
            elif case[3]&4 and pc==0x4cc0af:boundary,field=4,0
            elif case[3]&8 and pc==0x4cc485:boundary=2
            elif case[3]&16 and pc==0:boundary=8
            elif case[3]&1 and pc in (0x4cc11d,0x4cc1a5):boundary=6
            else: raise RuntimeError(f'unclassified source fault {pc:#x}, case {case[:10]}')
        data=[boundary,field,result,self.count(WORK),self.count(REPLACEMENT),self.word(TASK+44)]
        for a,n in ((FREE,32),(ACTIVE,32),(self.particle_base,192),(FRAMES,176),(WORK,32),(REPLACEMENT,32)):
            data.extend(words(bytes(m.mem_read(a,n))))
        data.extend([len(self.events),*self.events,m.reg_read(UC_X86_REG_FPSW)&63])
        return data

def case(rng, op=None):
    c=[0]*124
    c[0]=rng.randrange(5) if op is None else op;c[1]=rng.choice((0x7f,0x47f,0x87f,0xc7f,0x27f,0x37f))
    c[2]=(rng.randrange(4)<<16)|rng.randrange(5)
    c[5]=rng.choice((0,0,1,2)) if c[0] in (1,2,3) and c[2]&65535 else 0
    c[4]=rng.randrange(16);c[8]=bits(rng.choice((0.,0.03,0.125,1.,-1.)))
    c[9]=bits(rng.choice((0.,-0.1,0.25)))
    c[16:24]=[0x11001100+i for i in range(8)]
    indices=list(range(4));rng.shuffle(indices)
    c[24:32]=[indices[0]|(indices[1]<<16),indices[2]|(indices[3]<<16),0x12341234,0x56785678,0x99aa99aa,0xbbccbbcc,0xddeeddee,0xff11ff11]
    for p in range(4):
        start=32+p*12;c[start:start+12]=[rng.getrandbits(32) for _ in range(12)]
        c[start+8]=bits(rng.choice((-1.,0.,0.25,1.,3.,float('nan'))))
        c[start+9]=bits(rng.choice((-0.5,0.,0.25,1.,4.)))
        c[start+10]=rng.choice((MASK,0,1));c[start+11]=rng.choice((0,1,2,MASK))
    for f in range(4):
        start=80+11*f;c[start:start+11]=[rng.getrandbits(32) for _ in range(11)]
        c[start]=(c[start]&0xffff0000)|(f if f<3 else rng.choice((65535,65534)))
        c[start+1]=rng.choice((0,1,2,3,MASK));c[start+6]=bits(rng.choice((0.,0.5,1.,2.)))
    return c

def cases(seed,count,holdout):
    rng=random.Random(seed)
    if not holdout:
        for op in range(4):
            for active in (0,1,2,3,4,32767,32768,65535):
                if active==32767:continue # no fixture backing for 32767 active entries
                c=case(rng,op);c[2]=(40<<16)|active
                if active>4:c[3]=15 # source signed counts skip all unchecked buffers
                yield c
        for op in (0,1,2):
            for null in (1,2,4,8,16,32):
                c=case(rng,op);c[2]=1;c[3]=null;c[4]=15;c[24]=(c[24]&0xffff0000)|0
                # Display and custom callbacks do not dereference particle/frame pointers.
                if op==0 and null==1:c[32+10]=1;c[80+33]=(c[80+33]&0xffff0000)|65535;c[32+8]=bits(4.)
                yield c
        for op in (1,2,3):
            for behavior in (1,2,4):
                c=case(rng,op);c[2]=1;c[5]=behavior;c[4]=15
                # Free-index alias contains valid replacement index 0.
                c[16]=0
                yield c
        for field in (0,4,8,12,16,20,32,40,44):
            c=case(rng,0);c[2]=1;c[24]=0;c[6]=field+1
            c[32+8]=bits(10.);c[32+9]=bits(1.);c[32+10]=1 if field==44 else 0
            if field==44:c[80+33]=(c[80+33]&0xffff0000)|65534
            yield c
        for frame_index in (MASK, 0, 1, 0x40000000, 0x7fffffff, 0x80000000, 0xc0000000):
            for loop_count in (0, 1, MASK):
                c=case(rng,4);c[32+10]=frame_index;c[32+11]=loop_count
                yield c
    for _ in range(count):yield case(rng)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True);p.add_argument('--probe',default='build/native-particle-lifecycle-probe.dylib')
    p.add_argument('--output',default='build/oracles/particle_lifecycle.json');p.add_argument('--seed',type=int,default=476210)
    p.add_argument('--count',type=int,default=1024);p.add_argument('--holdout',action='store_true');a=p.parse_args()
    source=Source(PEImage(a.exe));lib=ctypes.CDLL(str(Path(a.probe).resolve()))
    probe=lib.particle_lifecycle_case;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    failures=[];partials=[];stream=hashlib.sha256();coverage=Counter()
    for index,c in enumerate(cases(a.seed,a.count,a.holdout)):
        original=source.run(c);out=(ctypes.c_uint32*4096)();n=probe((ctypes.c_uint32*len(c))(*c),out);native=list(out)[:max(0,n)]
        stream.update(pack(original))
        coverage[f"routine_{c[0]}"]+=1;coverage[f"boundary_{original[0]}_field_{original[1]}"]+=1
        if c[0]==4:coverage[f"animation_result_{original[2]}"]+=1
        if c[5]:coverage[f"reentry_{c[5]}"]+=1
        if n!=len(original) or original[:-1]!=native[:-1]:failures.append(dict(case_id=index,input=c,original=original,native=native))
        elif original[-1]!=native[-1]:partials.append(dict(case_id=index,original_flags=original[-1],native_flags=native[-1]))
    report=dict(oracle='particle_lifecycle',cases=index+1,value_failures=len(failures),fp_partials=partials,mismatches=failures,passed=not failures and not partials,seed=a.seed,holdout=a.holdout,stream_sha256=stream.hexdigest(),source_ranges=source.ranges,probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),coverage=dict(coverage),limits=['DisplayParticle/custom updates/FreeMemory intercepted; renderer and native allocator ownership tested separately.','Mapped well-formed four-slot buffers and four-frame tables; arbitrary cross-allocation aliases remain unsupported.','All payload bits, counts, callback identities and observed prefixes compared; no actor/N3b claim.'])
    Path(a.output).parent.mkdir(parents=True,exist_ok=True);Path(a.output).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases','value_failures')}),'FP partials',len(partials))
    return 1 if failures else 2 if partials else 0
if __name__=='__main__':raise SystemExit(main())
