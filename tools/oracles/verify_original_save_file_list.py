"""Private FindSaveFiles+AddLineList+CheckSaveFile instruction comparison.

File enumeration and binary read APIs are resource inputs. All1392 live save bytes
and list nodes/timestamps are compared, including partial fread prefixes and resource
call snapshots. The first malloc sentinel's untouched name/time are tagged unknown.
"""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
SAVE=0x3b2b3a8
LIST=0x3c5e8b8
HEAP=0x12000000
NATIVE_PATHS=['src/game/save_file_list.cpp', 'src/game/save_file_list.hpp', 'src/game/file_select.cpp', 'tests/probes/native_save_file_list_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source:
    def __init__(self,image,input_mod=None):
        self.m=Original(image,0x7f).machine;self.m.mem_map(HEAP,0x100000)
        if input_mod is not None:
            for section in image.sections:
                if section['name'] in ('.text','.rdata'):continue
                size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
                if size:self.m.mem_map(image.base+section['rva'],size);self.m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
            for section in input_mod.sections:
                size=(max(section['virtual_size'],section['raw_size'])+4095)&~4095
                if size:self.m.mem_map(input_mod.base+section['rva'],size);self.m.mem_write(input_mod.base+section['rva'],input_mod.slice(section['file_offset'],section['raw_size']))
            site,target=input_mod.unpack('<II',input_mod.base+0xe0a8)
            assert (site,target)==(0x42d52d,0x10009240)
            self.m.mem_write(site,b'\xe9'+struct.pack('<i',target-site-5))
        for p in (0x3b2b000,0x3c5e000,0x2bcb000):
            if not any(a<=p<=b for a,b,_ in self.m.mem_regions()):self.m.mem_map(p,4096)
        self.imports={self.word(a):name for a,name in [(0x7db06c,'first'),(0x7db108,'next'),(0x7db1d4,'closefind'),(0x7db228,'format'),(0x7db0c8,'length'),(0x7db050,'copy')]}
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def string(self,a):
        out=bytearray()
        while True:
            b=self.m.mem_read(a+len(out),1)
            if b==b'\0':return out.decode('ascii')
            out+=b
    def nodes(self):
        result=[];p=self.word(LIST)
        while p:result.append(p);p=self.word(p+12)
        return result
    def record(self,n):return self.v[350+n*359:350+(n+1)*359]
    def name(self,n):return struct.pack('<8I',*self.record(n)[3:11]).split(b'\0')[0].decode()
    def fill(self,address):
        r=self.record(self.cursor);b=bytearray(320);struct.pack_into('<II',b,20,r[1],r[2]);name=self.name(self.cursor).encode();b[44:44+len(name)]=name;self.m.mem_write(address,bytes(b))
    def trace_call(self,kind,ordinal=0,path=''):
        raw=path.encode();row=[kind,ordinal,len(self.nodes()),len(raw)]+list(struct.unpack('<8I',raw.ljust(32,b'\0')))+list(struct.unpack('<348I',self.m.mem_read(SAVE,1392)))
        self.trace.append(row)
        if len(self.trace)==self.v[1]:self.stopped=True;self.m.emu_stop();return True
        return False
    def hook(self,m,a,size,user):
        if a not in self.imports and a not in (0x645002,0x644df9,0x6453a2,0x644de6,0x644cd3,0x644c7d,0x643ea7):return
        sp=m.reg_read(UC_X86_REG_ESP);arg=lambda n:self.word(sp+4*n);pop=0;result=0
        if a==0x645002:
            size=arg(1);result=self.allocated;self.allocated+=(size+15)&~15;m.mem_write(result,b'\xcd'*size)
        elif a==0x644df9 or self.imports.get(a)=='format':
            fmt=self.string(arg(2))
            value=self.string(arg(3)) if '%s' in fmt else ctypes.c_int32(arg(3)).value
            text=fmt%value;raw=text.encode();m.mem_write(arg(1),raw+b'\0');result=len(raw)
        elif a==0x6453a2:
            text=self.string(arg(1));digits=''
            for c in text:
                if not c.isdigit():break
                digits+=c
            result=int(digits or '0')
        elif a==0x644de6:
            path=self.string(arg(1));name=path.split('/')[-1];self.opened=next(n for n in range(self.v[0]) if self.name(n)==name)
            if self.trace_call(4,self.opened+1,path):return
            result=0x100+self.opened
        elif a==0x644cd3:
            assert arg(1)==SAVE and arg(2)==1392 and arg(3)==1
            n=arg(4)-0x100
            if self.trace_call(5,n+1):return
            r=self.record(n);length=min(r[0],1392);m.mem_write(SAVE,struct.pack('<348I',*r[11:])[:length]);result=int(length==1392)
        elif a==0x644c7d:
            if self.trace_call(6,arg(1)-0xff):return
        elif a==0x643ea7:result=m.reg_read(UC_X86_REG_EAX)
        else:
            name=self.imports[a]
            if name=='first':
                if self.trace_call(1,0,self.string(arg(1))):return
                pop=8;self.cursor=0
                if self.v[0]:self.fill(arg(2));result=1
                else:result=0xffffffff
            elif name=='next':
                if self.trace_call(2):return
                pop=8;self.cursor+=1
                if self.cursor<self.v[0]:self.fill(arg(2));result=1
            elif name=='closefind':
                if self.trace_call(3):return
                pop=4;result=1
            elif name=='length':pop=4;result=len(self.string(arg(1)))
            elif name=='copy':
                pop=8;m.mem_write(arg(1),self.string(arg(2)).encode()+b'\0');result=arg(1)
        m.reg_write(UC_X86_REG_EAX,result&0xffffffff);m.reg_write(UC_X86_REG_EIP,arg(0));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.cursor=0;self.allocated=HEAP;self.m.context_restore(self.initial);self.write(LIST,0);self.m.mem_write(SAVE,struct.pack('<348I',*v[2:350]));self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(0x505050,STOP,timeout=0)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        nodes=self.nodes();out=[0]*160000;out[:4]=[0 if self.stopped else self.m.reg_read(UC_X86_REG_EAX)&255,int(self.stopped),len(self.trace),len(nodes)];out[16:364]=list(struct.unpack('<348I',self.m.mem_read(SAVE,1392)))
        for n,p in enumerate(nodes):
            if not n:continue
            raw=self.string(self.word(p)).encode().ljust(20,b'\0');out[512+n*8:520+n*8]=[1]+list(struct.unpack('<5I',raw))+[self.word(p+4),self.word(p+8)]
        for n,row in enumerate(self.trace):out[2048+n*360:2408+n*360]=row
        return out

def checksum(raw):
    crc=65535
    for byte in raw[4:]:
        crc^=byte
        for _ in range(8):crc=(crc>>1)^(0x8408 if crc&1 else 0)
    return (~crc)&65535

def cases(seed,count):
    rng=random.Random(seed);initial=[rng.getrandbits(32) for _ in range(348)];result=[]
    def record(name,length=1392,valid=True,stamp=0x123456789abcdef0):
        raw=bytearray(rng.getrandbits(8) for _ in range(1392));struct.pack_into('<H',raw,0,checksum(raw)^(0 if valid else 1))
        return [length,stamp&0xffffffff,stamp>>32]+list(struct.unpack('<8I',name.encode().ljust(32,b'\0')))+list(struct.unpack('<348I',raw))
    for n in (0,1,2,99):
        rows=[record('SonicDX%02d.snc'%(i+1),stamp=100-i) for i in range(n)]
        for stop in (0,1,2,3,4,5,7):result.append([n,stop]+initial+sum(rows,[]))
    for length in (0,1,2,3,4,100,1391,1392):
     for valid in (False,True):
      for name in ('SonicDX00.snc','SonicDX01.snc','SonicDXAA.snc','sonicdx07.snc'):
        result.append([1,0]+initial+record(name,length,valid))
    for _ in range(count):
        n=rng.randrange(9);rows=[record('SonicDX%02d.snc'%(i+1),rng.choice([0,100,1391,1392]),bool(rng.randrange(2)),rng.getrandbits(64)) for i in range(n)]
        result.append([n,rng.choice([0,0,0,1,2,4,7,16])]+initial+sum(rows,[]))
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=256);p.add_argument('--seed',type=int,default=505050);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_save_file_list_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;vectors=cases(a.seed,a.cases);bad=[]
    for n,v in enumerate(vectors):
        expected=source.run(v);out=(ctypes.c_uint32*160000)();probe((ctypes.c_uint32*len(v))(*v),out)
        if list(out)!=expected:
            bad.append({'case':n,'input_header':v[:2],'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y][:30]})
            if len(bad)>=8:break
    report={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(5263440, 394), (5262928, 114), (4378800, 38)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['FindFirst/Next/File I/O and allocator are resource fixtures; real FindSaveFiles/AddLineList/CheckSaveFile/CRC instructions execute. First malloc sentinel name/time remain an unknown tag, not compared values. Normal matched ASCII save filenames; missing FILE and malformed provider names are explicit native boundaries.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
