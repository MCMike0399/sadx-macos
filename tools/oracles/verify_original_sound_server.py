"""Original sound server including overlap/boss/name/sample selection instruction bodies.

Final PCM/3D/distance/voice APIs are traced resource receipts with stopped prefixes.
This proves queue consumption and ordering, not physical backend playback.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
QUEUE=0x3b292f8
BANK=0x12000000
WAVES=BANK+0x20000
CALLS={0x4106f0:1,0x410970:2,0x4103b0:3,0x423c20:4,0x423c80:5,0x410260:6,0x423cc0:7,0x410050:8,0x410100:9,0x425710:10,0x424b10:11,0x4244a0:12}
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine;self.imports={};self.m.mem_map(BANK,0x40000)
        for page in (0x3b29000,0x3b2c000,0x3b42000,0x910000,0x90a000,0x2bcb000):
            if not any(a<=page<=b for a,b,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
        for i,cue in enumerate(range(-1,1401)):
            name=BANK+0x10000+i*24;self.m.mem_write(name,('B00_00_%02d.wav'%cue).encode()+b'\0');self.m.mem_write(BANK+28+i*12,struct.pack('<3I',name,WAVES+i*16,1000+cue))
        self.write(BANK+24,1402)
        for address,symbol in [(0x7db228,'format'),(0x7db0e4,'compare')]:self.imports[self.word(address)]=symbol
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def string(self,a):
        data=bytearray()
        while True:
            b=self.m.mem_read(a+len(data),1)
            if b==b'\0':return bytes(data).decode('ascii')
            data+=b
    def sample_id(self):
        pointer=self.word(0x3b291c0)
        return 0 if not pointer else (pointer-WAVES)//16+1
    def overlap(self):return list(struct.unpack('<19I',self.m.mem_read(0x3b292a8,74)+b'\0\0'))
    def row(self,ch):return list(struct.unpack('<14I',self.m.mem_read(QUEUE+ch*56,56)))
    def receipt(self,kind,ch,args):
        out=[kind,*args,ch&0xffffffff]+[0]*42
        if ch>=0:out[6:20]=self.row(ch);out[20]=self.word(0x3b29c28+ch*4);out[47]=self.word(0x3b29b90+ch*4)
        out[21]=self.m.mem_read(0x3b29ce8,1)[0];out[22]=self.sample_id();out[23]=self.word(0x3b29ae0);out[24:43]=self.overlap();out[43]=self.word(0x3b29ce0);out[44]=self.word(0x3b29ce4);out[45]=self.word(0x3b29b7c);out[46]=self.word(0x910290)
        return out
    def hook(self,m,a,size,user):
        sp=m.reg_read(UC_X86_REG_ESP)
        if a in self.imports:
            if self.imports[a]=='format':
                target=self.word(sp+4);bank=self.word(sp+12);cue=self.word(sp+16);bank=ctypes.c_int32(bank).value;cue=ctypes.c_int32(cue).value
                raw=('B%02d_00_%02d.wav'%(bank,cue)).encode();m.mem_write(target,raw+b'\0');result=len(raw);pop=0
            else:
                left=self.string(self.word(sp+4)).lower();right=self.string(self.word(sp+8)).lower();result=(left>right)-(left<right);pop=8
            m.reg_write(UC_X86_REG_EAX,result&0xffffffff);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop);return
        if a==0x643ea7:
            m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4);return
        if a not in CALLS:return
        kind=CALLS[a]
        if kind==11 and not self.v[5]:return
        args=[0]*4
        if kind in (1,2):
            ch=self.word(sp+4);pointer=self.word(sp+8);size=self.word(sp+12);loop=self.word(sp+16)
            args=[ch,0 if not pointer else (pointer-WAVES)//16+1,loop,size]
        elif kind==3:ch=self.word(sp+4);args[:3]=[ch,self.word(sp+8),self.word(sp+12)]
        elif kind in (4,5,7):args[:2]=[self.word(sp+4),self.word(sp+8)];ch=args[1]
        elif kind==6:args[:2]=[self.word(sp+4),self.word(sp+8)];ch=args[0]
        elif kind in (8,9):ch=self.word(sp+4);args[0]=ch
        elif kind==10:ch=m.reg_read(UC_X86_REG_EBX);args[0]=self.word(sp+4)
        elif kind==11:ch=-1
        else:ch=m.reg_read(UC_X86_REG_EAX);args[0]=ch
        self.trace.append(self.receipt(kind,ch,args))
        if self.v[7]==len(self.trace):self.stopped=True;m.emu_stop();return
        m.reg_write(UC_X86_REG_EAX,self.v[6] if kind==12 else 0);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def invoke(self,entry):
        self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(entry,STOP,count=3000000)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.m.context_restore(self.initial)
        self.write(0x3b29ce0,v[0]);self.write(0x3b29ce4,v[1]);self.m.mem_write(0x3b29ce8,bytes([v[2]&255]));self.write(0x90a0ac,v[3]);self.write(0x3b42e14,v[4]);self.write(0x3b2cbb0,v[5]);self.write(0x3b29b7c,v[13]);self.write(0x910290,8)
        self.m.mem_write(QUEUE,struct.pack('<504I',*v[14:518]));self.m.mem_write(0x3b29c28,struct.pack('<36I',*v[518:554]));self.m.mem_write(0x3b29b90,struct.pack('<36I',*v[554:590]));self.m.mem_write(0x3b292a8,struct.pack('<19I',*v[590:609])[:74]);self.m.mem_write(0x910090,struct.pack('<128I',*v[609:737]));self.write(0x3b291c0,0);self.write(0x3b29ae0,0)
        for bank in range(16):self.write(0x3b291c8+bank*4,BANK if v[10]&(1<<bank) else 0)
        for n in range(1402):self.write(BANK+32+n*12,WAVES+n*16 if v[8] else 0)
        if v[12]:self.invoke(0x505810)
        self.invoke(0x4250d0)
        out=[0]*32768;out[:9]=[len(self.trace),int(self.stopped),self.sample_id(),self.word(0x3b29ae0),self.m.mem_read(0x3b29ce8,1)[0],self.word(0x3b29ce0),self.word(0x3b29ce4),self.word(0x3b29b7c),self.word(0x910290)]
        out[9:513]=list(struct.unpack('<504I',self.m.mem_read(QUEUE,2016)));out[513:549]=list(struct.unpack('<36I',self.m.mem_read(0x3b29c28,144)));out[549:585]=list(struct.unpack('<36I',self.m.mem_read(0x3b29b90,144)));out[585:604]=self.overlap()
        for index,row in enumerate(self.trace):out[640+48*index:688+48*index]=row
        return out
def cases(image,seed,count):
    table=list(image.unpack('<128I',0x910090));base=[0,0,0,0xffffffff,0,0,0xffffffff,0,1,0,65535,0,0,0]+[0]*595+table
    out=[]
    for pending in (False,True):
     for flags in (0,1,17,257,513,2049,8193,16385,51199,34,258,512,2048,8192,16384):
      for timer in (0,1,2,120,0xffffffff):
       for tone in (-1,0,2,7,39,40,762,780,822,1384,1385):
        v=base.copy();v[14+1]=timer;v[14+3]=flags;v[14+4]=tone&0xffffffff;v[14+5]=0x7fffffff;v[14+6]=0xffffffce;v[14+8]=0x12345678;v[3]=3 if pending else 0xffffffff;v[4]=int(pending);out.append(v)
    for paused in (0,1):
     for frame in (0,3,4,255):
      for stop in (0,1,2,3):
       v=base.copy();v[0]=paused;v[2]=frame;v[7]=stop;v[12]=1;out.append(v)
    for tone in (2,256,762):
     for tail in (0,2,0x102,0x202,0x302,0xff02):
      v=base.copy();v[14+3]=17;v[14+1]=120;v[14+4]=tone
      v[590:608]=[0x01010101]*18;v[608]=tail;out.append(v)
    rng=random.Random(seed)
    for _ in range(count):
        v=base.copy();v[:14]=[rng.choice([0,0,1]),rng.randrange(2),rng.randrange(256),rng.choice([0,3,6,0xffffffff]),rng.randrange(2),rng.randrange(2),rng.getrandbits(32),rng.choice([0,0,1,2,7,32]),rng.randrange(2),0,rng.getrandbits(16),0,0,rng.randrange(36)]
        for n in range(36):
            row=[rng.getrandbits(32) for _ in range(14)];row[1]=rng.choice([0,1,120,0xffffffff]);row[3]=rng.randrange(65536);row[4]=rng.choice([0,2,7,40,762,780,822,1384,1385]);v[14+n*14:28+n*14]=row;v[518+n]=rng.getrandbits(32);v[554+n]=rng.randrange(8)
        out.append(v)
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=256);p.add_argument('--seed',type=int,default=42500);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_sound_server_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[];vectors=cases(image,a.seed,a.cases)
    for index,v in enumerate(vectors):
        expected=source.run(v);output=(ctypes.c_uint32*32768)();probe((ctypes.c_uint32*737)(*v),output)
        if list(output)!=expected:
            bad.append({'case':index,'input':v,'diff':[[n,x,y] for n,(x,y) in enumerate(zip(expected,output)) if x!=y][:40]})
            if len(bad)>=8:break
    paths=['src/game/sound_effects.hpp','src/game/sound_effects.cpp','tests/probes/native_sound_server_probe.cpp']
    report={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x4250d0,0x4253bd),(0x423ad0,0x423b19),(0x423b20,0x423bdb),(0x424590,0x4245d9),(0x4246c0,0x42475e)]},'native_sources':{n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in paths},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['Original server, overlap, boss-bank and bank-sample selection bodies execute. PCM/3D/positional/distance/voice callees are exact traced resource seams; no physical playback claim. Valid native sample names include cues−1..1400; absent bank/data is covered; missing-name out-of-array source access remains explicit native boundary.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
