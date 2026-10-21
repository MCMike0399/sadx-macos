"""Private actual421de0 with original InitSaveData and pinned Input rumble patch."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import STACK,STOP
from verify_original_save_file_list import Source as FindSource,SAVE,LIST,HEAP,checksum
NATIVE_PATHS=['src/game/save_file_list.cpp', 'src/game/save_file_list.hpp', 'tests/probes/native_save_file_list_probe.cpp']
def native_sources():
    root=Path(__file__).resolve().parents[2]
    return {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in NATIVE_PATHS}

class Source(FindSource):
    def run(self,v):
        self.v=v;self.trace=[];self.stopped=False;self.cursor=0;self.allocated=HEAP+0x40000;self.m.context_restore(self.initial)
        extra=v[350+v[0]*359:];self.write(LIST,HEAP if extra[2] else 0)
        if extra[2]:
            self.m.mem_write(HEAP,b'\xcd'*16);self.write(HEAP+12,HEAP+16 if v[0] else 0)
            for n in range(v[0]):
                address=HEAP+(n+1)*16;name=HEAP+0x10000+n*32;r=self.record(n);self.m.mem_write(name,self.name(n).encode()+b'\0');self.m.mem_write(address,struct.pack('<4I',name,r[1],r[2],address+16 if n+1<v[0] else 0))
        self.write(0x3b29198,extra[0]);self.m.mem_write(0x3b290d8,bytes([extra[1]&255]));name=HEAP+0x20000;self.m.mem_write(name,self.name(extra[3]).encode()+b'\0');self.write(0x3b290dc,name);self.m.mem_write(0x3b0f0e4,struct.pack('<H',extra[4]&65535));self.m.mem_write(SAVE,struct.pack('<348I',*v[2:350]));self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.emu_start(0x421de0,STOP,timeout=0)
        assert self.stopped or self.m.reg_read(UC_X86_REG_EIP)==STOP
        nodes=self.nodes();out=[0]*160000;out[:5]=[0 if self.stopped else self.m.reg_read(UC_X86_REG_EAX),int(self.stopped),len(self.trace),len(nodes),int.from_bytes(self.m.mem_read(0x3b0f0e4,2),'little')];out[16:364]=list(struct.unpack('<348I',self.m.mem_read(SAVE,1392)))
        for n,p in enumerate(nodes):
            if n:out[512+n*8:520+n*8]=[1]+list(struct.unpack('<5I',self.string(self.word(p)).encode().ljust(20,b'\0')))+[self.word(p+4),self.word(p+8)]
        for n,row in enumerate(self.trace):out[2048+n*360:2408+n*360]=row
        return out

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('native_library');p.add_argument('--output',required=True);p.add_argument('--cases',type=int,default=128);p.add_argument('--seed',type=int,default=42180);p.add_argument('--input-dll',default='reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll');a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;dll=PEImage(a.input_dll);assert dll.sha256=='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5';source=Source(image,dll)
    rng=random.Random(a.seed);initial=[rng.getrandbits(32) for _ in range(348)]
    def row(index,length):
        raw=bytearray(rng.getrandbits(8) for _ in range(1392));struct.pack_into('<H',raw,0,checksum(raw));return [length,index+1,0]+list(struct.unpack('<8I',('SonicDX%02d.snc'%(index+1)).encode().ljust(32,b'\0')))+list(struct.unpack('<348I',raw))
    vectors=[]
    for has_list in (0,1):
     for disabled in (0,1,0xffffffff):
      for selected in (1,2):
       for length in (0,1,100,1391,1392):
        for stop in (0,1,2,3):vectors.append([2,stop]+initial+row(0,length)+row(1,length)+[disabled,selected,has_list,0,0xabcd])
    for _ in range(a.cases):
        n=rng.randrange(1,9);rows=sum([row(i,rng.choice([0,1,100,1391,1392])) for i in range(n)],[]);vectors.append([n,rng.randrange(4)]+initial+rows+[rng.choice([0,0,1]),rng.randrange(1,n+1),rng.randrange(2),rng.randrange(n),rng.getrandbits(16)])
    lib=ctypes.CDLL(a.native_library);probe=lib.sadx_native_load_selected_save_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;bad=[]
    for n,v in enumerate(vectors):
        expected=source.run(v);out=(ctypes.c_uint32*160000)();probe((ctypes.c_uint32*len(v))(*v),out)
        if list(out)!=expected:
            bad.append({'case':n,'extra':v[350+v[0]*359:],'diff':[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if x!=y][:30]})
            if len(bad)>=8:break
    result={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'input_sha256':dll.sha256,'native_sources':native_sources(),'source_functions':{f'0x{x:x}-0x{x+n:x}':hashlib.sha256(image.read(x,n)).hexdigest() for x,n in [(4333024, 167), (4379568, 539)]},'native_library_sha256':hashlib.sha256(Path(a.native_library).read_bytes()).hexdigest(),'limits':['Actual selected-save loader, save initializer and pinned Input default-rumble replacement execute; filesystem I/O is exact partial-write resource fixture. Selected unknown sentinel, invalid list traversal and missing FILE are explicit native boundaries, not successful source loads.']};Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
