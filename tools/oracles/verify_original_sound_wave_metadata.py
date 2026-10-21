"""Judge imported WAV sampler-loop policy against original constructor/ReadFormat.

WinMM only supplies raw chunk navigation and reads.
API search/read contracts: https://learn.microsoft.com/en-us/windows/win32/api/mmiscapi/nf-mmiscapi-mmiodescend
and https://learn.microsoft.com/en-us/windows/win32/api/mmiscapi/nf-mmiscapi-mmioread . Original instructions perform
all format and loop decisions. Loop endpoints left uninitialized remain None in
metadata, with known poison words supplied only by this reference fixture.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
OBJ,ALLOC=DATA+0x1000,DATA+0x2000
POISON=0xabadfeed
class Source:
    def __init__(self,image):
        self.m=Original(image,0x7f).machine;self.imports={}
        rva,size=image.import_directory
        for delta in range(0,size-19,20):
            original,_,_,name,first=image.unpack('<5I',image.base+rva+delta)
            if not any((original,name,first)):break
            index=0
            while True:
                value,=image.unpack('<I',image.base+(original or first)+4*index)
                if not value:break
                if not value&0x80000000:
                    symbol=image.string(image.base+value+2)
                    if symbol in ('mmioDescend','mmioRead','mmioAscend'):
                        self.imports[self.word(image.base+first+4*index)]=symbol
                index+=1
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook);self.initial=self.m.context_save()
    def word(self,a):return struct.unpack('<I',self.m.mem_read(a,4))[0]
    def write(self,a,v):self.m.mem_write(a,struct.pack('<I',v&0xffffffff))
    def hook(self,m,a,size,user):
        if a not in self.imports and a not in (0x6443e7,0x6443e2):return
        sp=m.reg_read(UC_X86_REG_ESP);pop=0;result=0
        if a==0x6443e7:result=ALLOC
        elif a!=0x6443e2:
            symbol=self.imports[a]
            if symbol=='mmioDescend':
                chunk,parent,flags=[self.word(sp+4*i) for i in (2,3,4)];pop=16
                if not parent:
                    if self.raw[:4]!=b'RIFF' or len(self.raw)<12:result=1
                    else:
                        m.mem_write(chunk,struct.pack('<5I',int.from_bytes(b'RIFF','little'),struct.unpack_from('<I',self.raw,4)[0],int.from_bytes(self.raw[8:12],'little'),8,0));self.cursor=12
                else:
                    wanted=self.word(chunk).to_bytes(4,'little');offset=self.cursor;end=min(len(self.raw),8+struct.unpack_from('<I',self.raw,4)[0]);found=None
                    while offset+8<=end:
                        name,length=struct.unpack_from('<4sI',self.raw,offset)
                        if name==wanted:found=(offset+8,length);break
                        offset+=8+length+(length&1)
                    if found:
                        start,length=found;m.mem_write(chunk,struct.pack('<5I',int.from_bytes(wanted,'little'),length,0,start,0));self.cursor=start
                    else:result=1
            elif symbol=='mmioRead':
                target,count=self.word(sp+8),self.word(sp+12);pop=12
                payload=self.raw[self.cursor:self.cursor+count];m.mem_write(target,payload);self.cursor+=len(payload);result=len(payload)
            elif symbol=='mmioAscend':
                chunk=self.word(sp+8);pop=12
                self.cursor=self.word(chunk+12)+self.word(chunk+4)+(self.word(chunk+4)&1)
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4+pop)
    def invoke(self,entry):
        self.m.mem_write(STACK-0x100,struct.pack('<I',STOP));self.m.reg_write(UC_X86_REG_ESP,STACK-0x100);self.m.reg_write(UC_X86_REG_ECX,OBJ)
        self.m.emu_start(entry,STOP,count=100000);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        return self.m.reg_read(UC_X86_REG_EAX)
    def run(self,raw):
        self.raw=raw;self.cursor=0;self.m.context_restore(self.initial);self.m.mem_write(OBJ,bytes(0x200));self.write(OBJ+0x98,POISON);self.write(OBJ+0x9c,POISON)
        self.invoke(0x44d920);self.write(OBJ+4,1);status=self.invoke(0x44d940)
        return status,[self.word(OBJ+offset) for offset in (0x94,0x98,0x9c)]
def chunk(name,payload):return name+struct.pack('<I',len(payload))+payload+bytes(len(payload)&1)
def wave(loops=None,start=0,end=1,align=2,before=False,short=False):
    fmt=chunk(b'fmt ',struct.pack('<HHIIHH',1,1,22050,44100,align,16));smpl=b''
    if loops is not None:
        payload=bytearray(36);struct.pack_into('<I',payload,28,loops&0xffffffff)
        if not short:payload+=struct.pack('<6I',0,0,start,end,0,0)
        smpl=chunk(b'smpl',payload)
    body=b'WAVE'+(smpl+fmt if before else fmt+smpl)+chunk(b'data',bytes(64))
    return b'RIFF'+struct.pack('<I',len(body))+body
def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('--output',required=True);p.add_argument('--importer',default='tools/import/sound_effects.py');p.add_argument('--assets',type=Path);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
    spec=importlib.util.spec_from_file_location('sound_effects_import',a.importer);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);source=Source(image)
    vectors=[wave(),wave(1,short=True),wave(-1,short=True)]
    for loops in (0,1,2,0xffffffff,0x80000000):
     for first,last in ((0,1),(1,1),(2,1),(0xffffffff,1),(1,0xffffffff),(0x80000000,0x80000001)):
      for align in (0,1,2,4,65535):
       for before in (False,True):vectors.append(wave(loops,first,last,align,before))
    sample_digests={}
    if a.assets:
        manifest=json.loads((a.assets/'sound-banks.json').read_text())
        for bank in manifest['banks']:
            for sample in bank['samples']:
                raw=(a.assets/sample['path']).read_bytes();digest=hashlib.sha256(raw).hexdigest()
                assert digest==sample['sha256'];sample_digests[sample['path']]=digest;vectors.append(raw)
    bad=[]
    for index,raw in enumerate(vectors):
        status,words=source.run(raw)
        try:
            metadata=module.extract_wave_metadata(raw)
            native=[metadata['loop_count']&0xffffffff]+[POISON if metadata[k] is None else metadata[k] for k in ('loop_start','loop_end')];accepted=True
        except ValueError:native=[];accepted=False
        if accepted!=(status<0x80000000) or (accepted and native!=words):bad.append({'case':index,'source_status':status,'source_words':words,'imported_words':native})
    report={'passed':not bad,'cases':len(vectors),'mismatches':len(bad),'first_mismatches':bad[:8],'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x44d920,0x44d93f),(0x44d940,0x44db7c)]},'source_sample_sha256':sample_digests,'importer_sha256':hashlib.sha256(Path(a.importer).read_bytes()).hexdigest(),'limits':['WinMM chunk navigation/read and allocation are resource seams. Valid RIFF PCM fmt/sampler records, signed loop count and wrapping byte offsets are compared; physical audio is outside this metadata judge.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':raise SystemExit(main())
