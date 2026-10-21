"""LoadLevelSounds/LoadCharVoices selection, live owner reads and failure stores.

Executes original bodies and GetPlayerNumber. SetBankDir is a resource receipt for
its separately verified pinned LoadSoundList_r implementation; dsStop_all is a
receipt for separately verified native cleanup::stop_all_sounds. Their internal
bank decoding/PCM lifetimes are not retested here. The compatibility792f60 entry
executes its actual RET0. Return values of LoadCharVoices are unused by its caller
and excluded; LoadLevelSounds returns the actual dsStop_all receipt result.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, STACK, STOP

FIELDS = [(0x3b22dcc,2),(0x3b22dec,2),(0x3b22dc0,2),(0x3b0ef38,4),
          (0x3b18db5,1),(0x3b29ce0,4),(0x3b29ce4,4),(0x3b29b88,4),(0x3b29208,4)]

class Source:
    def __init__(self,image):
        assert image.sha256 == SOURCE_SHA256 and unicorn.__version__ == '2.1.4'
        self.m = m = Original(image,127).machine
        for section in image.sections:
            if section['name'] in ('.text','.rdata'):continue
            size=(max(section['raw_size'],section['virtual_size'])+4095)&~4095
            m.mem_map(image.base+section['rva'],size)
            m.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
        assert image.read(0x792f60,3) == b'\x33\xc0\xc3'
        m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x4238e0,end=0x4238e0)
        m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=0x424460,end=0x424460)
        self.context=m.context_save()
    def get(self,at,size=4):return int.from_bytes(self.m.mem_read(at,size),'little')
    def put(self,at,value,size=4):self.m.mem_write(at,(value&((1<<(size*8))-1)).to_bytes(size,'little'))
    def values(self):return [self.get(at,size) for at,size in FIELDS]
    def hook(self,m,at,size,user):
        sp=m.reg_read(UC_X86_REG_ESP)
        kind=self.get(sp+4) if at==0x4238e0 else 100
        stage,act,char,voice,metal,paused,pospaused,queue,bank=self.values()
        self.trace.extend([kind,paused,pospaused,queue,bank,char,voice,metal,stage,act])
        calls=len(self.trace)//10
        if self.input[10] and calls==self.input[10]:self.failed=True;m.emu_stop();return
        if self.input[11] and calls==self.input[11]:
            for field,value in zip(FIELDS[2:5],self.input[12:15]):self.put(field[0],value,field[1])
            for field,value in zip(FIELDS[:2],self.input[15:17]):self.put(field[0],value,field[1])
        m.reg_write(UC_X86_REG_EAX,self.input[17] if at==0x424460 else 0)
        m.reg_write(UC_X86_REG_ESP,sp+4);m.reg_write(UC_X86_REG_EIP,self.get(sp))
    def run(self,values):
        self.input=values;self.failed=False;self.trace=[];m=self.m;m.context_restore(self.context)
        for (address,size),value in zip(FIELDS,values[1:10]):self.put(address,value,size)
        self.put(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x4245f0 if values[0] else 0x424c80,STOP)
        if not self.failed and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('source did not return')
        stage,act,char,voice,metal,paused,pospaused,queue,bank=self.values()
        out=[0]*128
        out[:11]=[int(self.failed),paused,pospaused,queue,bank,len(self.trace)//10,char,voice,metal,stage,act]
        if not values[0] and not self.failed:out[11]=m.reg_read(UC_X86_REG_EAX)
        out[32:32+len(self.trace)]=self.trace
        return out

def fixtures(seed,count):
    rng=random.Random(seed);cases=[]
    for stage in range(44):
        for char in range(10):
            for voice in [0,1]:
                for metal in [0,255]:cases.append([0,stage,0,char,voice,metal,7,8,9,10,0,0,0,0,0,0,0,0])
    for char in [0,1,2,3,4,5,6,7,8,65535]:
        for voice in [0,1,0xffffffff]:cases.append([1,0,0,char,voice,1,7,8,9,10,0,0,0,0,0,0,0,0])
    for fail in range(1,6):cases.append([0,9,0,3,0,1,7,8,9,10,fail,0,0,0,0,0,0,0])
    cases.append([0,9,0,3,0,0,7,8,9,10,0,3,3,1,0,9,0,0])
    for _ in range(count):
        cases.append([rng.randrange(2),rng.choice([rng.randrange(44),rng.getrandbits(16)]),
                      rng.choice([rng.randrange(256),rng.getrandbits(16)]),rng.choice([rng.randrange(10),65535]),
                      rng.choice([0,1,0xffffffff]),rng.choice([0,1,255]),rng.getrandbits(32),rng.getrandbits(32),
                      rng.getrandbits(32),rng.getrandbits(32),rng.choice([0,0,0,rng.randrange(1,6)]),
                      rng.randrange(5),rng.choice([rng.randrange(10),65535]),rng.choice([0,1,0xffffffff]),
                      rng.randrange(256),rng.getrandbits(16),rng.getrandbits(16),rng.getrandbits(32)])
    return cases

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path)
    p.add_argument('--output',required=True,type=Path);p.add_argument('--seed',type=int,default=20261105);p.add_argument('--cases',type=int,default=500)
    a=p.parse_args();image=PEImage(a.exe);source=Source(image);lib=ctypes.CDLL(str(a.probe.resolve()))
    U=ctypes.c_uint32;probe=lib.sadx_native_level_sounds;probe.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U)]
    initial_reset_words={'queue':source.get(0x3b29b88),'bank':source.get(0x3b29208)}
    assert initial_reset_words=={'queue':0,'bank':0}
    bad=[];cases=fixtures(a.seed,a.cases)
    for number,values in enumerate(cases):
        expected=source.run(values);out=(U*128)();assert probe((U*18)(*values),out)==1
        differences=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y]
        if differences:bad.append({'case':number,'input':values,'differences':differences})
    paths=['src/game/level_sounds.hpp','src/game/level_sounds.cpp','tests/probes/native_level_sounds_probe.cpp','src/game/game_cleanup.cpp','src/game/game_cleanup.hpp']
    ranges=[(0x424c80,0x424fb4),(0x4245f0,0x4246b8),(0x4144e0,0x4144e8),(0x792f60,0x792f63)]
    report={'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),
            'cases':len(cases),'seed':a.seed,'mismatch_count':len(bad),'mismatches':bad[:12],
            'stores_calls_live_reads_match':not bad,'limits':__doc__,'initial_reset_words':initial_reset_words,
            'native_sources':{path:hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in paths},
            'source_ranges':{f'{hex(start)}-{hex(end)}':hashlib.sha256(image.read(start,end-start)).hexdigest() for start,end in ranges}}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(cases),'mismatch_count':len(bad)}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
