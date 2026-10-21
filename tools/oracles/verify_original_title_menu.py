"""Source-exact title setup/sequence allocation; dependency bodies are traced seams."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

ACTIONS = (0x3c566c0,0x3c566d0,0x3c53510,0x3c53520,0x3c559c0,
           0x3c559d0,0x3c559e0,0x3c55a00,0x3c559f0)
GLOBALS = (0x3c5e868,0x3b2a2f2,0x3b2a2fb,0x3b1118c,0x3c5e8d0,0x3b2c588)
TASKS = DATA+0x1000
WORK = DATA+0x2000
CHILD_WORK = DATA+0x2100
DIALOG = DATA+0xb000
CANONICAL_DIALOG = DATA+0xc000
SEQ = DATA+0x3000
FLAGS = DATA+0x4000
CALLS = {0x425670:(1,0),0x505850:(2,0),0x411e30:(3,2),0x77ed40:(4,1),
  0x40b1a0:(5,0),0x40b460:(6,0),0x402f10:(7,3),0x42c4e0:(8,1),
  0x503480:(9,0),0x420ef0:(10,0),0x406f00:(11,0),0x432ea0:(12,0),
  0x40aeb0:(13,0),STOP+16:(14,3),0x437240:(15,1),0x403180:(16,2),
  0x781250:(17,1),0x4212e0:(18,1),0x422680:(19,1),0x420a00:(20,1),
  0x420a50:(21,1),0x420a70:(22,4),0x420c50:(23,4),
  0x40b860:(24,3),0x40b2d0:(25,2),0x505fb0:(26,0),0x5060e0:(27,0),
  0x432c60:(30,0),0x4238e0:(31,1),0x420a30:(32,1),0x421180:(33,2),0x40b570:(34,1),0x40cff0:(35,1),0x40b220:(37,1),0x4147f0:(38,1),0x505fe0:(39,0),0x5064d0:(40,0),0x505a80:(41,0),0x413920:(46,0),0x414570:(47,2),0x40aee0:(48,0),0x506010:(49,1),0x505e90:(51,0),0x50bd10:(52,0),0x510120:(53,0),0x506e10:(54,1),0x506e40:(55,1),0x50e300:(56,0),0x506040:(57,1),0x403290:(58,1),0x433170:(59,0),0x433010:(60,1),0x50e470:(61,0),0x401410:(62,1),0x40fdc0:(63,0),0x40ef40:(64,0),0x40b170:(65,0),0x411b40:(66,0),0x40b540:(67,0),0x40efe0:(68,1),0x40b730:(69,0),0x420f40:(70,0)}
FACTORIES=(0,0x5124a0,0x505600,0x50b490,0x511400,0x510390,0x50bf30,0x509860,0x50e270,0x50a6c0,0x50e020,0x50d470,0x5084a0,0x50c140)
CALLS.update({a:(45,1) for a in FACTORIES[1:]})

class Source:
    def __init__(self,image):
        self.m = Original(image,0x7f).machine
        addresses = list(ACTIONS)+list(GLOBALS)+[0x38f6e7c,0x3a7cf28,0x3aad104,
                    0x3d128b0,0x10d7b40,0x3c5e8c8,0x3c5e8cc,0x3c5e8d8,0x3c5e8dc,0x3c5e8e0,0x3b0f0e8,0x3b29d00,0x912698,0x91269c,0x9126a8,0x912694,0x3b29cf8,0x3b29cf4,0x3abdf70,0x3c5ff00,0x3b28114]
        mapped = [(a,b) for a,b,_ in self.m.mem_regions()]
        for page in sorted({a & ~4095 for a in addresses}):
            if not any(a<=page<=b for a,b in mapped):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.initial_context=self.m.context_save()

    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v & 0xffffffff))
    def byte(self,a):return self.m.mem_read(a,1)[0]
    def hook(self,m,a,size,user):
        if a not in CALLS:return
        if (self.v[0]==5 and a==0x505850) or (self.v[0]==6 and a==0x503480) or (self.v[0]==8 and a==0x505fb0) or (self.v[0]==9 and a==0x425670) or (self.v[0]==13 and a==0x432c60) or (self.v[0]==16 and a==0x510390) or (self.v[0]==17 and a==0x505a80) or (self.v[0]==18 and a==0x413920) or (self.v[0]==19 and a==0x510120) or (self.v[0]==27 and a==0x5060e0):return
        kind,argc=CALLS[a]
        stack=m.reg_read(UC_X86_REG_ESP)
        args=[self.word(stack+4+4*i) for i in range(argc)]
        ret=0
        if kind in (17,20,21,32):args=[]
        if kind==14:args=args[1:]
        if kind==45:args=[FACTORIES.index(a)]+[self.word(args[0]+4*i) for i in range(3)]
        if kind==34 and self.v[0]==15:kind=42;args=[1]
        if kind==34 and self.v[0] in (23,26):kind=71;args=[1]
        if kind==68:ret=self.v[2]
        if kind==41:args=[int(m.reg_read(UC_X86_REG_EAX)!=0)]
        if kind==33 and self.v[0]==19:
            assert args==[0x7efa80,0x10d9fb4]
            kind=50;args=[]
        if kind==58:
            assert args==[0x10d9fb4];args=[]
        if kind in (54,55):ret=self.v[15]
        if kind==33:
            texture=(args[0]-(DATA+0x9000))//16
            assert args==[DATA+0x9000+texture*16,DATA+0xa000+texture*16]
            args=[texture]
        if kind==8:args=[self.word(args[0])]
        if kind==23:
            assert bytes(m.mem_read(args[0],15))==b'Now Loading...\0'
            args=args[1:]
        if kind==24:
            callback=args.pop()
            if callback==0x505c20:
                ret=0 if self.v[4]&1 else TASKS
            elif callback==0x505b60:
                kind=28;ret=0 if self.v[4]&2 else TASKS+0x100
            elif callback==0x432720:
                kind=36;ret=0 if self.v[4]&4 else TASKS+0x300
            elif callback==0x5101a0:
                kind=43;ret=0 if self.v[4]&1 else TASKS+0x700
            else:raise RuntimeError(f'unexpected task callback {callback:x}')
        if kind==25:
            if args==[1,36]:ret=WORK
            elif args==[1,8]:kind=29;ret=CHILD_WORK
            elif args==[1,60]:kind=44;ret=DATA+0xe100
            else:raise RuntimeError(f'unexpected allocation {args}')
        if kind==37:ret=DIALOG
        if kind==30:ret=0 if self.v[4]&4 else TASKS+0x200
        self.trace += [kind]+args+[0]*(4-len(args))
        self.count+=1
        if self.count==self.v[16]:
            self.stopped=True;m.emu_stop();return
        if kind in (24,28,36,43) and ret:self.write(ret+0x10,callback)
        if kind==30:self.write(0x3b2c588,ret)
        m.reg_write(UC_X86_REG_EAX,ret)
        m.reg_write(UC_X86_REG_EIP,self.word(stack))
        m.reg_write(UC_X86_REG_ESP,stack+4+(12 if kind==14 else 0))

    def run(self,v):
        self.m.context_restore(self.initial_context)
        self.v=v;self.trace=[];self.count=0;self.stopped=False
        self.m.mem_write(DATA+0x1000,bytes(0x4000))
        for a in ACTIONS:self.write(a,0)
        for a in GLOBALS:self.write(a,0)
        for a,value in zip(GLOBALS,v[3:4]+v[5:8]+[0,0]):self.write(a,value)
        for a,offset in [(0x38f6e7c,0),(0x3a7cf28,0x300),(0x3aad104,0x600)]:
            self.write(a,DATA+0x5000+offset)
        for a,value in zip([DATA+0x5190,DATA+0x5194,DATA+0x53a0,DATA+0x53a4,
                            DATA+0x5610,DATA+0x5674,DATA+0x5678],v[8:15]):self.write(a,value)
        for a,value in zip([0x3b29d00,0x912698,0x91269c,0x9126a8,0x912694,0x3b29cf8,0x3b29cf4,0x3abdf70,0x3c5ff00,0x3b28114],v[8:15]):self.write(a,value)
        for a,value in zip([0x3b11188,0x3b11178,0x3b1117c,0x3b11180,0x3b11198,0x3b2a2e4],
                           [v[9],v[10],v[11],v[8],v[12],v[13]]):self.write(a,value)
        self.m.mem_write(0x3c5fed0,bytes([v[8]&255,v[9]&255]))
        self.write(0x3b28114,v[7])
        self.write(0x3b11184,v[10])
        self.write(0x3abdf70,v[7]);self.write(0x3c5ff00,0)
        self.m.mem_write(DATA+0xe100,bytes(60))
        self.m.mem_write(DATA+0xe000,struct.pack("<3I",*v[8:11]))
        self.write(0x3c5e8c8,v[8]);self.write(0x3c5e8cc,v[9])
        for a,value in zip([0x3c5e8d8,0x3c5e8dc,0x3c5e8e0],v[10:13]):self.write(a,value)
        self.write(0x3b0f0e8,0);self.write(0x10d7cb0,DATA+0x8000)
        for i in range(30):
            self.write(DATA+0x8000+i*8,DATA+0x9000+i*16)
            self.write(DATA+0x8004+i*8,DATA+0xa000+i*16)
        for i,value in enumerate([x%30 for x in v[13:16]][:v[1]%4]+[30]):self.write(DATA+0x7000+i*4,value)
        self.write(0x3d128b0,DATA+0x6000);self.write(DATA+0x6000,DATA+0x6100)
        self.write(DATA+0x6100+200,STOP+16)
        for i in range(3):self.write(0x10d7b40+4*i,SEQ+i*16);self.write(SEQ+i*16,v[15]+i)
        self.m.mem_write(FLAGS,bytes([v[1]&255,v[2]&255]))
        if v[0] in (11,12):
            self.write(0x3b2c588,TASKS+0x400 if v[4]&8 else TASKS+0x300)
            self.write(TASKS+0x300+0x2c,DIALOG)
            self.write(TASKS+0x400+0x2c,CANONICAL_DIALOG)
        dialog_values={0:v[8],4:v[9],8:v[10],12:v[11],16:v[12],20:v[13],36:v[14],60:v[15],64:v[7]}
        for base in (DIALOG,CANONICAL_DIALOG):
            self.m.mem_write(base,bytes(68))
            for offset,value in dialog_values.items():self.write(base+offset,value)
            self.m.mem_write(base+42,struct.pack('<4I',*v[8:12]))
        if v[0]==14:
            self.write(0x3c5e8d0,TASKS+0x600 if v[4]&8 else TASKS+0x500)
            self.write(TASKS+0x500+0x2c,DATA+0xd000)
            self.write(TASKS+0x600+0x2c,DATA+0xd100)
            raw=struct.pack('<7IH4B2x',SEQ,v[8],v[9],v[10],v[11],v[12],v[13],v[14]&65535,
                            (v[14]>>16)&255,(v[14]>>24)&255,v[15]&255,(v[15]>>8)&255)
            self.m.mem_write(DATA+0xd000,raw)
            self.m.mem_write(DATA+0xd100,raw)
            self.write(DATA+0xd100+4,v[9]);self.write(DATA+0xd100+8,v[10]);self.write(DATA+0xd100+20,v[11])
        if v[0]==15:
            self.write(TASKS+0x500+0x2c,DATA+0xd200)
            self.write(DATA+0xd200,SEQ);self.write(DATA+0xd204,v[15])
        if v[0]==17:
            records=[[v[8]%14,v[9],v[10],v[11]]]+[
                [v[12+i]%13+1,v[8+i],v[9+i],v[10+i]] for i in range(v[1]%4)]+[[0,0,0,0]]
            self.m.mem_write(DATA+0x8000,b''.join(struct.pack('<4I',*row) for row in records))
            for i,address in enumerate(FACTORIES):self.write(0x10d7b4c+i*4,address)
        if v[0] in (19,20,21):
            self.m.mem_write(DATA+0xe100,struct.pack('<15I',v[8],v[9],v[10],v[11],v[12],v[13],v[14],
                                v[15],v[8]&255,v[9],v[10],v[11],v[12],v[13],v[14]&255))
        if v[0] in (19,20):
            self.write(0x3c5ff00,TASKS+0x700)
            self.write(TASKS+0x700+0x2c,DATA+0xe200 if v[0]==19 and v[4]&8 else DATA+0xe100)
            self.m.mem_write(DATA+0xe200,bytes(self.m.mem_read(DATA+0xe100,60)))
            self.write(DATA+0xe200+8,v[5])
            self.write(0x3b0f0e8,v[1]);self.write(0x3b2a2fc,v[2]&255)
            self.m.mem_write(0x3b2a2f3,bytes([v[3]&255]))
        if v[0]==20:
            self.write(0x3c5e8d0,TASKS);self.write(TASKS+0x2c,DATA+0xd300)
            self.m.mem_write(DATA+0xd300,struct.pack('<7IH4B2x',SEQ,v[5],v[6],v[7],0,0,0,0,0,0,0,0))
            self.write(0x3b11180,v[9])
        if v[0] in (22,23,24,26):
            present=(v[0] in (22,24) or not(v[4]&1))
            if present:
                self.write(0x3c5e8d0,TASKS)
                if not(v[4]&2) or v[2]!=0 or v[0] in (23,26):
                    self.write(TASKS+0x2c,WORK)
                    self.m.mem_write(WORK,struct.pack('<7IH4B2x',SEQ,v[8],v[9],v[10],v[11],v[12],v[13],v[14]&65535,
                                    (v[14]>>16)&255,(v[14]>>24)&255,v[15]&255,(v[15]>>8)&255))
        self.write(STACK,STOP)
        entry=[0x5034a0,0x5033f0,0x5031e0,0x505e00,0x5032e0,0x505850,0x503480,0x505ed0,0x505fb0,0x425670,0x425620,0x432690,0x432650,0x432c60,0x505c20,0x505b60,0x510390,0x505a80,0x413920,0x510120,0x5101a0,0x510350,0x5033b0,0x503380,0x503270,0x503450,0x505b20,0x5060e0][v[0]]
        if v[0]==3:self.write(FLAGS,v[1]);self.write(STACK+4,FLAGS)
        if v[0]==4:self.write(STACK+4,FLAGS)
        if v[0]==7:self.write(STACK+4,DATA+0x7000)
        if v[0] in (14,15):self.write(STACK+4,TASKS+0x500)
        if v[0]==16:self.write(STACK+4,DATA+0xe000)
        if v[0] in (20,21):self.write(STACK+4,TASKS+0x700)
        if v[0]==11:self.m.reg_write(UC_X86_REG_EAX,TASKS+0x300)
        if v[0]==12:self.m.reg_write(UC_X86_REG_EAX,DIALOG)
        if v[0]==17:self.m.reg_write(UC_X86_REG_EAX,DATA+0x8000)
        if v[0]==19:self.m.reg_write(UC_X86_REG_ESI,DATA+0xe100)
        if v[0]==21:self.write(TASKS+0x700+0x2c,DATA+0xe100)
        self.m.reg_write(UC_X86_REG_ESP,STACK)
        tick_failure=0
        try:self.m.emu_start(entry,STOP,timeout=1000000)
        except unicorn.UcError:
            if v[0]!=18 or self.m.reg_read(UC_X86_REG_EIP)!=0x41392f:raise
            if v[9]==0:tick_failure=1
            elif v[8]==0x80000000 and v[9]==0xffffffff:tick_failure=2
            else:raise
            self.stopped=True
        if not self.stopped and self.m.reg_read(UC_X86_REG_EIP)!=STOP:
            raise RuntimeError('source did not complete')
        out=[0]*512
        out[:9]=[self.word(a) for a in ACTIONS]
        out[9:15]=[self.byte(GLOBALS[0]),self.byte(GLOBALS[1]),self.byte(GLOBALS[2]),
                   self.word(GLOBALS[3]),int(self.word(GLOBALS[4])!=0),
                   3 if self.word(GLOBALS[5]) else 0]
        if self.word(TASKS+0x2c):
            raw=bytes(self.m.mem_read(self.word(TASKS+0x2c),36));words=struct.unpack('<7Ih4B2x',raw)
            out[15:27]=[((words[0]-SEQ)//16+1) if words[0] else 0]+list(words[1:7])+[words[7]&65535]+list(words[8:12])
        if self.word(TASKS+0x100+0x2c):out[27:29]=[((self.word(CHILD_WORK)-SEQ)//16+1) if self.word(CHILD_WORK) else 0,self.word(CHILD_WORK+4)]
        out[29]=int(self.word(TASKS+0x10)==0x505c20)
        out[30]=int(self.word(TASKS+0x18)==0x505ae0)
        out[31]=int(self.word(TASKS+0x110)==0x505b60)
        out[32]=int(not self.stopped);out[33]=self.count
        out[34]=self.word(0x3c5e8c8);out[35]=self.word(0x3c5e8cc)
        out[36:39]=[self.word(a) for a in [0x3c5e8d8,0x3c5e8dc,0x3c5e8e0]]
        out[39:46]=[self.word(a) for a in [0x3b29d00,0x912698,0x91269c,0x9126a8,0x912694,0x3b29cf8,0x3b29cf4]]
        def dialog_values(base):
            return [self.word(base+i) for i in [0,4,8,12,16,20,36,42,46,50,54,60,64]]
        if v[0] in (11,12) or (v[0]==13 and self.word(TASKS+0x300+0x2c)):
            out[46:59]=dialog_values(DIALOG)
            out[59:72]=dialog_values(CANONICAL_DIALOG if v[0] in (11,12) and v[4]&8 else DIALOG)
        out[72]=int(self.word(TASKS+0x300+0x10)==0x432720)
        out[73]=int(self.word(TASKS+0x300+0x14)==0x432480)
        if v[0]==14:
            def sequence_values(base):
                words=struct.unpack('<7IH4B2x',bytes(self.m.mem_read(base,36)))
                return [int(words[0]!=0)]+list(words[1:])
            out[74:86]=sequence_values(DATA+0xd000)
            out[86:98]=sequence_values(DATA+0xd100 if v[4]&8 else DATA+0xd000)
        out[98]=self.word(0x3abdf70)
        out[99]=self.word(DATA+0xd204) if v[0]==15 else v[15]
        out[100]=4 if self.word(0x3c5ff00) else 0
        if self.word(0x3c5ff00) and self.word(TASKS+0x700+0x2c):
            ptr=self.word(TASKS+0x700+0x2c)
            out[101:116]=struct.unpack('<15I',bytes(self.m.mem_read(ptr,60)))
            out[108]=int(out[108]!=0);out[109]&=255;out[112]=int(out[112]!=0);out[115]&=255
        out[116]=int(self.word(TASKS+0x700+0x10)==0x5101a0)
        out[117]=int(self.word(TASKS+0x700+0x14)==0x510350)
        out[118]=self.word(0x3b1117c);out[119]=tick_failure
        if v[0] in (19,20,21):
            out[120:135]=struct.unpack('<15I',bytes(self.m.mem_read(DATA+0xe100,60)))
            out[127]=int(out[127]!=0);out[128]&=255;out[131]=int(out[131]!=0);out[134]&=255
        out[135]=self.m.reg_read(UC_X86_REG_FPSW)&63
        out[136]=self.word(0x3b11184)
        out[137]=self.word(0x3b28114)
        out[138]=self.m.reg_read(UC_X86_REG_EAX) if v[0] in (22,23,24) and not self.stopped else 0
        out[139]=self.byte(0x3c5fed0);out[140]=self.byte(0x3c5fed1)
        out[141:141+len(self.trace)]=self.trace
        return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe',type=Path);p.add_argument('native_library',type=Path)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--seed',type=int,default=20261020);p.add_argument('--cases',type=int,default=1000)
    a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('unexpected executable')
    source=Source(image);lib=ctypes.CDLL(str(a.native_library.resolve()))
    native=lib.sadx_native_title_menu;native.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;native.restype=None
    rng=random.Random(a.seed);rows=[]
    for op in range(28):
        for flags in [0,1,2,3,255]:
            if op==3 and flags>2:continue
            for fail in range(16):
                for stop in [0,1,2,5,10,15,20,25,30]:
                    v=[rng.getrandbits(32) for _ in range(17)]
                    v[:8]=[op,flags,rng.choice([0,1,255]),rng.choice([0,1,255]),fail,
                            rng.getrandbits(8),rng.getrandbits(8),rng.getrandbits(32)]
                    if op in (22,24):v[10]=rng.choice([0,5,14,15,16])
                    if op==19:v[2]=rng.choice([0,1,255]);v[3]=rng.choice([0,1,255])
                    if op==20:
                        v[8]=[0,1,2,3,255][[0,1,2,3,255].index(flags)]
                        v[5]=rng.choice([0,5]);v[6]=rng.choice([0,5]);v[9]=rng.choice([0,1,2,6,20,0xffffffff])
                        v[7]=rng.choice([0,1,2,6,7,0x7fffffff,0x80000000])
                        v[12]=rng.choice([0,0x3f000000,0x3f800000,0x3f666666,0x7f800000,0x7fc12345,0x7f812345,1])
                    if op==21:
                        v[13]=rng.choice([0,1,0xffffffff]);v[14]=rng.choice([0,1,0xffffffff,0x80000000])
                    if op==18:
                        cases={0:(0,0x80000000,0),1:(1,0x80000000,0xffffffff),2:(1,100,0),3:(1,0xfffffff9,2),255:(1,0x7fffffff,0xfffffffd)}
                        v[12],v[8],v[9]=cases[flags];v[13]=fail%2
                    if op in (11,12,13):v[7]=flags
                    if op==9 and flags%2==0:v[8]=0
                    if op==14:
                        v[8]=[0,5,14,15,16][[0,1,2,3,255].index(flags)]
                        v[9]=rng.choice([0,5,14,15,16]);v[11]=rng.choice([0xffffffff,1,100,101,102,103,104,105,106,107,108])
                        v[13]=rng.choice([0,5,14,15,16])
                        v[14]=(rng.choice([0,1,2])<<24)|(rng.choice([0,0,1])<<16)|(rng.choice([0,1,2,65535,32768,32767]))
                    if op==15:v[15]=rng.choice([0,1,2,0xffffffff,0x80000000,0x7fffffff])
                    v[16]=stop;rows.append(v)
    for _ in range(a.cases):
        v=[rng.getrandbits(32) for _ in range(17)]
        v[0]=rng.randrange(28);v[1]=rng.randrange(3) if v[0]==3 else rng.getrandbits(8)
        v[3]=rng.choice([0,1,255]);v[16]=0;rows.append(v)
    mismatches=[];flag_mismatches=0;first_flag_mismatches=[]
    for index,v in enumerate(rows):
        expected=source.run(v);arr=(ctypes.c_uint32*17)(*v);out=(ctypes.c_uint32*512)();native(arr,out)
        actual=list(out)
        if actual[135]!=expected[135]:
            flag_mismatches+=1
            if len(first_flag_mismatches)<3:first_flag_mismatches.append({'case':index,'source':expected[135],'native':actual[135]})
        value_differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,actual)) if i!=135 and x!=y]
        if value_differences:
            mismatches.append({'case':index,'input':v,'diff':value_differences})
            if len(mismatches)>=8:break
    report={'passed':not mismatches and flag_mismatches==0,'values_match':not mismatches,'fp_flag_mismatches':flag_mismatches,'first_fp_flag_mismatches':first_flag_mismatches,'cases':len(rows),
            'mismatches':len(mismatches),'first_mismatches':mismatches,'seed':a.seed,'limits':['Dependency bodies are traced seams, not recovered by this comparison.', 'CAlloc/MAlloc null faults are not compared; native allocations report explicit failure.'],
            'sonic_ranges':[['0x5031e0','0x503269'],['0x5032e0','0x503376'],
                            ['0x5033f0','0x503441'],['0x5034a0','0x503508'],
                            ['0x505b90','0x505c1d'],['0x505e00','0x505e5d'],['0x505850','0x505867'],['0x503480','0x50349e'],['0x505ed0','0x505f33'],['0x505fb0','0x505fd4'],['0x425670','0x42568d'],['0x425620','0x425662'],['0x432650','0x43268c'],['0x432690','0x4326ee'],['0x432c60','0x432ca4'],['0x505c20','0x505d65'],['0x505b60','0x505b87'],['0x510390','0x5103f2'],['0x505a80','0x505ad3'],['0x413920','0x41397b'],['0x510120','0x51019e'],['0x5101a0','0x51033e'],['0x510350','0x510385'],['0x5033b0','0x5033e3'],['0x503380','0x5033a1'],['0x503270','0x5032d6'],['0x503450','0x503476'],['0x505b20','0x505b3d'],['0x5060e0','0x5060ed']]}
    ranges = report.pop('sonic_ranges')
    report['source_sha256'] = SOURCE_SHA256
    report['source_ranges'] = {start+'-'+end: hashlib.sha256(image.read(int(start,16),int(end,16)-int(start,16))).hexdigest()
                               for start,end in ranges}
    root = Path(__file__).resolve().parents[2]
    paths = ['src/game/title_menu.hpp','src/game/title_menu.cpp',
             'tests/probes/native_title_menu_probe.cpp','src/engine/task.hpp',
             'src/engine/character_resources.hpp','src/game/game_cleanup.hpp','src/engine/game_frame_control.hpp','src/engine/frame_loop_native.hpp','src/engine/frame_loop_native.cpp','src/cpu/fidelity_float.hpp','src/cpu/x87_math.hpp','src/cpu/x87_math.cpp']
    report['native_sources'] = {path: hashlib.sha256(((root/path) if (root/path).exists() else Path.cwd()/path).read_bytes()).hexdigest()
                                for path in paths}
    report['native_library_sha256'] = hashlib.sha256(a.native_library.read_bytes()).hexdigest()
    if flag_mismatches:report['limits'].append('Compared low-six x87 exception flags differ; all compared stores, calls and returns match. Full stack/tag/condition flags are outside the arithmetic API.')
    report['limits'].append('Title floating-point cases use explicit post-device control word0x007f; other control words are not claimed.')
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report));return 0 if not mismatches and flag_mismatches==0 else 1
if __name__=='__main__':raise SystemExit(main())
