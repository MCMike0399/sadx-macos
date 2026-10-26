"""Execute E102's real mode-0 instructions and common tail with concrete lower receipts.

No lower implementation is claimed. Each receipt records its actual arguments and
the current source publications/mutations before supplying explicitly declared
work, collision, animation, matrix and shadow outputs.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import sys

ROOT = Path(os.environ.get('SADX_SOURCE_ROOT', Path(__file__).resolve().parents[2]))
sys.path[:0] = [str(ROOT/'tools/lib'), str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import *
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

TASK=DATA+0x1000
PRIMARY=DATA+0x1100
MOTION=DATA+0x1200
PLAYERS=[DATA+0x2000,DATA+0x2400]
EXTRAS=[DATA+0x3000,DATA+0x3400]
COLLISION=DATA+0x4000
INFOS=DATA+0x4100
FLOOR_TASK=DATA+0x4300
FLOOR_PRIMARY=DATA+0x4400
MATERIAL_TABLE=DATA+0x4800
MATERIALS=[DATA+0x5000+n*0x100 for n in range(6)]
CALLS=[0,0x42fb00,0x426690,0x4c3780,0x4258f0,0x481460,0x47fb90,
       0x442750,0x41caf0,0x421280,0x40b2d0,0x4c2e10,0x4c5420,0x40b940,
       0x49fb10,0x44a800,0x43fa90,0x4c3870,0x4b82d0,0x784fb0,0x4c2e90,
       0x47fd50,0x4c4da0,0x4c44a0,0x481180,0x49ee30,0x4c2f50,0x4c33c0,
       0x41bac0,0x41c280,0x446f10,0x40b420]
MATRICES=[0x3c53ad8,0x3c53c48,0x3c53b78,0x3c53b20,0x3c53c00]
WORDS=68
def bits(f): return struct.unpack('<I',struct.pack('<f',f))[0]
def sha(b): return hashlib.sha256(b).hexdigest()
def identity(address, values): return 0 if address==0 else values.index(address)+1

class Source:
    def __init__(self,image):
        self.m=Original(image,127).machine
        for s in image.sections:
            if s['name'] in ('.text','.rdata'): continue
            size=(max(s['virtual_size'],s['raw_size'])+4095)&~4095
            self.m.mem_map(image.base+s['rva'],size)
            if s['raw_size']:
                self.m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],s['raw_size']))
        self.initial=self.m.context_save()
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.m.hook_add(unicorn.UC_HOOK_MEM_INVALID,self.invalid)
    def get(self,a,size=4): return int.from_bytes(self.m.mem_read(a,size),'little')
    def put(self,a,v,size=4): self.m.mem_write(a,int(v&((1<<(size*8))-1)).to_bytes(size,'little'))
    def snapshot(self):
        s=[0]*64
        p=self.get(MOTION) or PLAYERS[1]
        s[:14]=[identity(self.get(0x3abdf60),[PRIMARY]),identity(self.get(0x3abdf64),PLAYERS),
                self.get(0x3c53c40,1),self.get(0x3c53c41,1),self.get(0x3c53c42,1),
                self.get(0x3c53b18,1),self.get(PRIMARY,1),self.get(PRIMARY+2,1),
                self.get(PRIMARY+8,1),self.get(PRIMARY+9,1),self.get(PRIMARY+4,2),
                self.get(PRIMARY+0x14),self.get(PRIMARY+0x1c),identity(self.get(MOTION),PLAYERS)]
        s[14:30]=[int(self.get(p+0x140)==0x3c53fa8),self.get(p+0x124,2),self.get(p+0x126,2),
                  int(self.get(p+0x144)==0x3c54558),identity(self.get(p+0x7c),EXTRAS),
                  self.get(p+0x80,2),self.get(TASK+0x18),self.get(TASK+0x14),
                  self.get(EXTRAS[0]+0x240),int(self.get(0x3c53b60)==0x3c53f68),
                  self.get(0x3c53b68),self.get(0x3c53b6c),int(self.get(0x3c53b64)==0x91c648),
                  2 if self.get(0x3c54608) else 0,self.get(0x3c53bf8,1),
                  identity(self.get(0x3c53b70),EXTRAS)]
        for n,row in enumerate([1,5,4,3,2]):
            actual_row=2 if self.w[31]&256 and row>1 else row
            s[30+n]=self.get(MATERIALS[actual_row]+(6 if n==0 else 2)*20+16)
            if n: s[34+n]=self.get(MATERIALS[actual_row]+2*20)
        s[39:44]=[self.get(a) for a in MATRICES]
        s[44:49]=[self.get(INFOS+3,1),self.get(INFOS+48+3,1),self.get(INFOS+100),
                  self.get(INFOS+20),self.get(p+0x110)]
        for n,e in enumerate(EXTRAS):
            s[49+n*4:53+n*4]=[self.get(e+0x234),self.get(e+0x238),self.get(e+8,2),self.get(e+0x23c)]
        return s
    def receipt(self,k,a=0,b=0,c=0):
        self.trace += [k,a,b,c]+self.snapshot()
        if k:
            self.calls+=1
            if self.calls==self.w[12]:
                self.boundary=CALLS[k]; self.m.emu_stop(); return False
        return True
    def invalid(self,m,access,address,size,value,user):
        self.boundary=m.reg_read(UC_X86_REG_EIP)
        return False
    def hook(self,m,address,size,user):
        if address==0x4835ca and self.get(PRIMARY,1)!=0:
            self.boundary=address; m.emu_stop(); return
        if address not in CALLS[1:]: return
        k=CALLS.index(address); sp=m.reg_read(UC_X86_REG_ESP)
        args=[self.get(sp+4+n*4) for n in range(4)]
        p=self.get(MOTION)
        pid=identity(p,PLAYERS)
        a=b=c=0
        if k==3: a,b,c=identity(args[0],EXTRAS),args[1],args[2]
        elif k in (5,16,27,30): a=identity(args[2],PLAYERS)
        elif k==6: a=identity(m.reg_read(UC_X86_REG_EBX),PLAYERS)
        elif k==15: a=identity(args[0]-0x120,PLAYERS)
        elif k==24: a=identity(m.reg_read(UC_X86_REG_EAX),PLAYERS)
        elif k==7: a=args[0]
        elif k==8: a,b=args[2],args[3]; assert args[:2]==[TASK,0x91c5b8]
        elif k==9: a=args[0]
        elif k==10: a,b=args[:2]
        elif k==11:
            a=identity(args[0]-0xb0,EXTRAS); b,c=0xb0,0xd0
            assert args[1]==args[0]+0x20
        elif k==13: a,b=args[:2]; assert args[2]==TASK
        elif k==17: a,b=identity(args[0],EXTRAS),args[1]
        elif k==18: a=args[0]
        elif k==19: a=m.reg_read(UC_X86_REG_EDX); assert m.reg_read(UC_X86_REG_ECX)==MATRICES[2]
        elif k in (20,22,23): a,b=identity(args[1],PLAYERS),identity(args[2],EXTRAS)
        elif k==25: a=args[1]; assert args[0]==PRIMARY
        if not self.receipt(k,a,b,c): return
        result=0
        if k==1: result=self.w[6]
        elif k==2: result=self.w[7]
        elif k==4: result=self.w[8]
        elif k==7:
            self.put(MOTION,PLAYERS[1]); self.put(PRIMARY+8,self.w[23],1); self.put(PRIMARY+2,self.w[24],1)
        elif k==8:
            self.put(PRIMARY+0x38,COLLISION)
            if self.w[13]&16:self.put(p+0x10c,self.w[26]^0x80000000)
        elif k==10:
            result=0 if self.w[11] else EXTRAS[0]
            if result: m.mem_write(result,bytes(580))
        elif k==15:
            self.put(p+0x124,self.w[14],2); self.put(p+0x126,self.w[15],2)
            if self.w[13]&8:self.put(PRIMARY+8,self.w[23]^7,1)
            if self.w[13]&64:self.put(MOTION,PLAYERS[0])
        elif k==18: self.put(args[1],a)
        elif k==19: self.put(MATRICES[2],a)
        elif k==21 and self.w[30]: self.put(p+0x7c,EXTRAS[1])
        elif k==25:
            # Concrete binary32 Shadow receipt, converted to an x87 return.
            sign=(self.w[18]>>16)&0x8000
            raw=self.w[18]&0x7fffffff
            exp=raw>>23; fraction=raw&0x7fffff
            if exp==255: sig=(0x800000|fraction)<<40; se=sign|0x7fff
            elif exp:
                sig=(0x800000|fraction)<<40; se=sign|(exp-127+16383)
            elif fraction:
                shift=23-(fraction.bit_length()-1)
                sig=(fraction<<shift)<<40; se=sign|(1-127-shift+16383)
            else: sig=0; se=sign
            if self.w[13]&2:self.put(p+0x7c,EXTRAS[1])
            if self.w[13]&32:sig=0x8000008000000000;se=0x3fff
            sw=m.reg_read(UC_X86_REG_FPSW); top=((sw>>11)-1)&7
            m.reg_write(UC_X86_REG_FP0+top,(sig,se));m.reg_write(UC_X86_REG_FPSW,(sw&~0x3800)|(top<<11))
            tag=m.reg_read(UC_X86_REG_FPTAG);m.reg_write(UC_X86_REG_FPTAG,tag&~(3<<(top*2)))
        elif k==26:
            result=self.w[29]
            if self.w[13]&4:self.put(p+0x7c,EXTRAS[0])
        self.put_result(result,sp)
    def put_result(self,result,sp):
        self.m.reg_write(UC_X86_REG_EAX,result)
        self.m.reg_write(UC_X86_REG_EIP,self.get(sp))
        self.m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,w,cw):
        m=self.m; m.context_restore(self.initial); self.w=w; self.trace=[]; self.calls=0; self.boundary=0
        m.mem_write(TASK,bytes(0x6000))
        for start,end in [(0x3c53ad8,0x3c5460c),(0x3abdf60,0x3abdf68)]:m.mem_write(start,bytes(end-start))
        self.put(TASK+32,PRIMARY);self.put(TASK+36,MOTION)
        self.put(MOTION,PLAYERS[0] if w[4] else 0);self.put(MOTION+8,w[5])
        self.put(PRIMARY,w[1],1);self.put(PRIMARY+2,w[2],1);self.put(PRIMARY+4,w[0],2)
        self.put(PRIMARY+8,w[3],1);self.put(PRIMARY+9,99,1)
        for at,v in [(0x14,111),(0x18,222),(0x1c,333),(0x24,w[17])]:self.put(PRIMARY+at,v)
        self.put(PRIMARY+0x38,COLLISION if w[25] else 0)
        self.put(COLLISION+12,INFOS);self.put(COLLISION+0x9c,FLOOR_TASK);self.put(FLOOR_TASK+32,FLOOR_PRIMARY)
        self.put(FLOOR_PRIMARY+0x14,w[27]);self.put(FLOOR_PRIMARY+0x1c,w[28])
        self.put(INFOS+3,0xa5,1);self.put(INFOS+51,0x5a,1);self.put(INFOS+100,0x11223344)
        for n,p in enumerate(PLAYERS):
            self.put(p+0x7c,EXTRAS[1]);self.put(p+0x80,0x1234,2)
            self.put(p+6,w[10],2);self.put(p+0x124,77,2);self.put(p+0x126,12,2)
            self.put(p+0x38,w[16]);self.put(p+0x10c,w[26]);self.put(p+0x68,1 if w[13]&1 else 0)
        for e in EXTRAS:
            self.put(e,w[9],1);self.put(e+8,w[22],2);self.put(e+0x240,0x12345678)
        for a,v in [(0x3c53c40,0x7b),(0x3c53c41,0x6a),(0x3c53c42,0x59),(0x3c53b18,0x48),(0x3c53bf8,0x37)]:self.put(a,v,1)
        self.put(0x3aad0ec,MATERIAL_TABLE)
        for row,a in enumerate(MATERIALS):
            actual=MATERIALS[2] if w[31]&256 and row>1 else a
            self.put(MATERIAL_TABLE+row*4,0 if (w[31]&255)==row else actual)
            for n in range(7):m.mem_write(a+n*20,struct.pack('<5I',0x10203040,2,3,4,w[21]))
        self.put(0x3b22dcc,w[19],2);self.put(0x3b22dec,w[20],2)
        self.put(STACK,STOP);self.put(STACK+4,TASK)
        m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw)
        m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        try:m.emu_start(0x483430,STOP,count=20000)
        except unicorn.UcError:
            if not self.boundary:raise
        if not self.boundary and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source did not return')
        self.receipt(0,self.boundary)
        return self.trace,[len(self.trace),self.boundary,m.reg_read(UC_X86_REG_FPSW)&63,self.calls]

def fixtures(holdout=False):
    rng=random.Random(0x48343091+(1 if holdout else 0))
    def base():
        return [0,0,2,0,0,bits(1),0,0,0,3,0,0,0,0,0,0,bits(4),bits(9),bits(2),0,1,
                0xffffffff,65535,0,2,0,bits(15),12345,54321,0xaabbccdd,0,0]
    cases=[]
    def add(**changes):
        w=base()
        for key,value in changes.items():w[int(key[1:])]=value
        cases.append(w)
    if not holdout:
        add()
        for stop in range(1,34):add(w12=stop)
        for row in range(1,6):add(w31=row)
        add(w11=1);add(w4=1,w25=1,w0=32,w5=bits(-1));add(w25=1)
        for mask in [1,2,4,8,16,32,63,64,127]:add(w13=mask,w17=bits(2))
        add(w31=256)
        for raw in [0,0x80000000,bits(-1),bits(1),0x7f800000,0xff800000,0x7fc12345]:
            add(w0=32,w5=raw)
        for mode in [1,51,53,56,60,128,255]:add(w1=mode,w4=1,w25=1)
        for cancel in [0,1,2,0xffffffff]:
            for stage in [0,25,26,255,32768,65535]:add(w2=1,w24=1,w6=cancel,w19=stage)
        for request in [0,5,53]:
            for current in [0,67,68]:
                for speed in [bits(3.5),bits(4),0x7fc12345]:add(w14=request,w15=current,w16=speed)
    for _ in range(180 if holdout else 120):
        w=base();w[0]=rng.randrange(65536);w[4]=rng.randrange(2);w[5]=bits(rng.choice([-4,-0.0,0,3]))
        w[6]=rng.choice([0,1,2,0xffffffff]);w[7]=rng.randrange(256);w[9]=rng.randrange(5)
        w[10]=rng.getrandbits(16);w[13]=rng.randrange(128);w[14]=rng.choice([0,5,53])
        w[15]=rng.choice([0,67,68]);w[16]=bits(rng.uniform(-8,8));w[17]=bits(rng.uniform(-500,500))
        w[18]=bits(rng.uniform(-100,100));w[19]=rng.randrange(65536);w[20]=rng.randrange(65536)
        w[21]=rng.getrandbits(32);w[22]=rng.randrange(65536);w[23]=rng.randrange(8)
        w[24]=rng.choice([1,2,9]);w[25]=rng.randrange(2) if w[4] else 0
        w[26]=bits(rng.uniform(-25,25));w[27]=rng.getrandbits(32);w[28]=rng.getrandbits(32)
        w[29]=rng.getrandbits(32);w[30]=rng.randrange(2)
        w[31]=rng.choice([0,256])
        cases.append(w)
    return cases

def main():
    ap=argparse.ArgumentParser();ap.add_argument('exe',type=Path);ap.add_argument('library',type=Path)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--holdout',action='store_true')
    a=ap.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('source/tool identity')
    lib=ctypes.CDLL(str(a.library.resolve()));fn=lib.sadx_native_gamma_selection
    U=ctypes.c_uint32;fn.argtypes=[ctypes.POINTER(U),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(U),ctypes.c_size_t,ctypes.POINTER(U)]
    source=Source(image);cases=fixtures(a.holdout);rows=[];failed=partial=0
    for cw in [0x7f,0x27f,0x37f,0x47f,0x87f,0xc7f]:
        for n,w in enumerate(cases):
            expected,meta=source.run(w,cw);out=(U*8192)();actual_meta=(U*4)()
            rc=fn((U*32)(*w),32,cw,out,8192,actual_meta)
            actual=list(out[:actual_meta[0]]);m=list(actual_meta)
            mismatch=rc!=0 or actual!=expected or m[:2]!=meta[:2] or m[3]!=meta[3]
            if mismatch:
                failed+=1
                first=next((i for i,(x,y) in enumerate(zip(expected,actual)) if x!=y),min(len(expected),len(actual)))
                rows.append({'case':n,'cw':cw,'rc':rc,'source_meta':meta,'native_meta':m,'first_word':first,
                             'source':expected[max(0,first-4):first+5],'native':actual[max(0,first-4):first+5],'input':w})
            elif m[2]!=meta[2]:partial+=1
    report={'profile':'original E102 mode0 plus mandatory common tail; concrete lower receipts',
            'source_sha256':image.sha256,'original_ranges_sha256':{'0x483430-0x4841cf':sha(image.read(0x483430,0xd9f))},
            'case_count':len(cases)*6,'value_failures':failed,'fp_flag_partials':partial,
            'passed':failed==0 and partial==0,'values_match':failed==0,'flag_mismatches':partial,
            'holdout':a.holdout,'native_library_sha256':sha(a.library.read_bytes()),
            'failures':rows[:12],'limits':['Lower callees are concrete receipt contracts, not recovered implementations.',
                'Only mode0 is recovered; other modes stop at explicit dispatch boundary.',
                'No rendering, full gate, route or story claim.',
                'Unicorn reports zero where native x87 records precision or invalid flags; partials remain failures of strict FP fidelity.',
                'Shadow receipts include binary32 and the retained 1+2^-24 return; other lower return values remain bounded fixtures.',
                'Hardware unmasked exception delivery is unverified.']}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ['case_count','value_failures','fp_flag_partials','holdout']}))
    return 1 if failed else 2 if partial else 0
if __name__=='__main__':sys.exit(main())
