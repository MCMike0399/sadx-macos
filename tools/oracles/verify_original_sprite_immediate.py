"""Immediate Sprite2D producer vs original instructions, with explicit renderer/matrix receipts."""
import argparse, ctypes, hashlib, json, pathlib, random, struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP,bits

class Source:
    def __init__(self,image):
        self.m=Original(image,127).machine
        self.m.mem_map(0x3d0f000,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def event(self,n):self.out[17+self.out[16]]=n;self.out[16]+=1
    def ret(self,pop=0):
        s=self.m.reg_read(UC_X86_REG_ESP);self.m.reg_write(UC_X86_REG_EIP,self.word(s));self.m.reg_write(UC_X86_REG_ESP,s+4+pop)
    def hook(self,m,a,size,user):
        s=m.reg_read(UC_X86_REG_ESP)
        if a==0x77f3d0:self.event(1);self.ret()
        elif a==0x7850f0:self.event(2);self.ret()
        elif a==0x784fb0:self.event(3);self.out[36]=m.reg_read(UC_X86_REG_EDX);self.ret()
        elif a==0x784d60:
            self.event(4);p=self.word(s+4);q=m.reg_read(UC_X86_REG_EDX)
            self.out[40+self.transforms*3:43+self.transforms*3]=struct.unpack('<3I',m.mem_read(p,12))
            m.mem_write(q,struct.pack('<3I',*self.v[24+self.transforms*3:27+self.transforms*3]));self.transforms+=1;self.ret(4)
        elif a==0x7816e0:self.event(5);self.ret()
        elif a==0x77dd90:self.event(6);self.out[35]=self.word(s+4);self.ret()
        elif a==0x78d3e0:self.event(7);self.out[13]=m.reg_read(UC_X86_REG_ECX);self.ret()
        elif a==0x78d140:self.event(8);self.out[14]=m.reg_read(UC_X86_REG_ECX);self.ret()
        elif a==0x77de10:
            self.event(9);q=struct.unpack('<14I',m.mem_read(self.word(s+4),56));self.out[:13]=q[:7]+q[8:];self.ret()
        elif a==0x78b800:self.event(10);self.ret()
    def run(self,v):
        self.v=v;self.out=[0]*64;self.transforms=0
        self.m.mem_write(DATA,struct.pack('<6I',*v[4:10])+struct.pack('<II',0 if v[0]==2 else DATA+256,DATA+512))
        self.m.mem_write(DATA+512,struct.pack('<10H',*(n&65535 for n in v[10:20])))
        self.m.mem_write(0x3d0f7f0,struct.pack('<4I',*v[20:24]))
        self.m.mem_write(STACK,struct.pack('<5I',STOP,0 if v[0]==1 else DATA,0,v[2],v[1]))
        self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.reg_write(UC_X86_REG_FPCW,v[3]);self.m.reg_write(UC_X86_REG_FPSW,0)
        self.m.emu_start(0x77e050,STOP,timeout=1000000)
        self.out[62]=self.m.reg_read(UC_X86_REG_FPCW);self.out[63]=self.m.reg_read(UC_X86_REG_FPSW)&63
        return self.out

class QuadSource:
    def __init__(self,image):
        self.m=Original(image,127).machine
        for p in [0x3d12000,0x3d0f000]:self.m.mem_map(p,4096)
        self.put(0x3d128ac,DATA+2048);self.put(0x3d128b0,DATA+4096);self.put(DATA+4096,DATA+4200);self.put(DATA+4200+0x120,0x10000300)
        self.m.mem_write(0x10000300,b'\xc3')
        self.m.mem_write(0x78dbe0,b'\xd9\x05'+struct.pack('<I',DATA+1024)+b'\xc2\x04\x00')
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def put(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        s=m.reg_read(UC_X86_REG_ESP)
        if a==0x78dbe0:self.out[28]=self.word(s+4);self.out[29]+=1
        elif a==0x10000300:
            assert [self.word(s+i*4) for i in [1,2,3,5]]==[DATA+4096,5,2,28]
            self.out[:28]=struct.unpack('<28I',m.mem_read(self.word(s+16),112));self.out[30]+=1
            m.reg_write(UC_X86_REG_EIP,self.word(s));m.reg_write(UC_X86_REG_ESP,s+24)
    def run(self,v):
        self.out=[0]*40;self.m.mem_write(DATA,struct.pack('<14I',*v[:14]));self.put(DATA+1024,v[14]);self.put(0x3d0fce4,v[15]);self.m.mem_write(DATA+2048,bytes(112))
        self.m.mem_write(STACK,struct.pack('<II',STOP,DATA));self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.reg_write(UC_X86_REG_FPCW,v[16]);self.m.reg_write(UC_X86_REG_FPSW,0)
        self.m.emu_start(0x77de10,STOP,timeout=1000000)
        self.out[38]=self.m.reg_read(UC_X86_REG_FPCW);self.out[39]=self.m.reg_read(UC_X86_REG_FPSW)&63;return self.out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--seed',type=int,default=776050);p.add_argument('--cases',type=int,default=600);a=p.parse_args()
    image=PEImage(a.exe)
    if image.sha256 != SOURCE_SHA256 or unicorn.__version__ != "2.1.4":
        raise RuntimeError("Pinned source/Unicorn mismatch")
    source=Source(image);lib=ctypes.CDLL(a.library);probe=lib.sadx_native_sprite_immediate;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    rng=random.Random(a.seed);mismatches=[];flags=0;cases=[]
    for i in range(a.cases):
        v=[0]*33;v[0]=i%3 if i<6 else 0;v[1]=i%64;v[2]=bits(rng.choice([-1000.,-1.,.5,100.,0.]));v[3]=rng.choice([127,639,895,1151,2175,3199])
        v[4:9]=[bits(rng.uniform(-1000,1000)) for _ in range(5)];v[9]=rng.getrandbits(32)
        v[10:20]=[rng.randrange(65536) for _ in range(10)]
        v[20:24]=[bits(rng.uniform(-2,2)) for _ in range(4)];v[24:33]=[bits(rng.uniform(-1000,1000)) for _ in range(9)]
        cases.append(v)
    # Real movie dimensions/material, plus integer bounds and selected IEEE special values.
    for flag in range(64):
        v=[0,flag,bits(-1000.),127]+[bits(f) for f in [320.,240.,0.,1.,1.]]+[0]+[640,480,320,240,0,0,160,120,0,0]+[bits(1.)]*4+[bits(0.)]*9;cases.append(v)
    real = list(cases[-2])
    for word in [0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000, 0x7fc12345]:
        for field in [2,4,5,7,8,20,21,22,23]:
            v=list(real);v[field]=word;cases.append(v)
    for i,v in enumerate(cases):
        expected=source.run(v);out=(ctypes.c_uint32*64)();probe((ctypes.c_uint32*33)(*v),out);actual=list(out)
        bad=[n for n in range(63) if expected[n]!=actual[n]]
        flags+=expected[63]!=actual[63]
        if bad:mismatches.append({'case':i,'indices':bad,'expected':[expected[n] for n in bad],'actual':[actual[n] for n in bad]})
    producer_cases=len(cases)
    quad_source=QuadSource(image);quad_probe=lib.sadx_native_sprite_quad
    quad_probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    for i in range(a.cases*2):
        v=[bits(rng.uniform(-10000,10000)) for _ in range(15)]+[rng.getrandbits(32),rng.choice([127,639,895,1151,2175,3199])]
        expected=quad_source.run(v);out=(ctypes.c_uint32*40)();quad_probe((ctypes.c_uint32*17)(*v),out);actual=list(out)
        bad=[n for n in range(39) if expected[n]!=actual[n]];flags+=expected[39]!=actual[39]
        if bad:mismatches.append({'quad_case':i,'indices':bad,'expected':[expected[n] for n in bad],'actual':[actual[n] for n in bad]})
    report={'cases':producer_cases+a.cases*2,'producer_cases':producer_cases,'quad_cases':a.cases*2,'seed':a.seed,'mismatches':len(mismatches),'first_mismatches':mismatches[:8],'flag_mismatches':flags,'values_match':not mismatches,'limits':['Producer renderer and matrix services are explicit fixtures. Quad executes original vertex writes and primitive-UP arguments with an explicit projected-depth FLD fixture; actual projected depth judged separately. Device pixels not certified.','FP exception flags remain separate from compared values.']}
    root=pathlib.Path(__file__).resolve().parents[2]
    names=['src/engine/sprite_immediate.hpp','src/engine/sprite_immediate.cpp','src/cpu/fidelity_float.hpp','tests/probes/native_sprite_immediate_probe.cpp','tools/oracles/verify_original_sprite_immediate.py']
    report['source_files']={n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in names}
    report['source_sha256']=image.sha256
    report['native_library_sha256']=hashlib.sha256(pathlib.Path(a.library).read_bytes()).hexdigest()
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='first_mismatches'}));return bool(mismatches)
if __name__=='__main__':raise SystemExit(main())
