"""Original setup helper and common-link prefixes, separate from legacy judges."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from verify_source import (ROOT, Original, DATA, STACK, STOP, PEImage, put, word, ret,
    UC_HOOK_CODE, UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP)
from unicorn.x86_const import UC_X86_REG_EDI

def setup(image, row):
    m=Original(image,127).machine
    task,primary,motion,models=DATA+0x1000,DATA+0x2000,DATA+0x3000,DATA+0x4000
    put(m,task+0x20,primary)
    for i,value in enumerate(row[2:5]):put(m,models+4*i,value)
    put(m,STACK,STOP);put(m,STACK+4,row[1])
    m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_EDI,models)
    calls=[]
    def hook(machine,address,size,_):
        sp=m.reg_read(UC_X86_REG_ESP)
        if address==0x40b860:
            args=[word(m,sp+i) for i in [4,8,12]];assert args==[2,2,0x493c70]
            calls.append(['CreateElementalTask',*args]);ret(m,0 if row[0] in (1, 3) else task)
        elif address==0x40b220:
            assert word(m,sp+4)==136;calls.append(['MAlloc',136]);ret(m,0 if row[0]==2 else motion)
        elif address==0x40b6c0:
            assert word(m,sp+4)==task;calls.append(['FreeTask',task]);ret(m)
    m.hook_add(UC_HOOK_CODE,hook);m.emu_start(0x495860,STOP,count=1000)
    out=[0]*16;out[0]=m.reg_read(UC_X86_REG_EAX)
    if row[0] not in (1, 3):
        out[1]=1;out[2]=136 if word(m,task+0x24) else 0
        out[3]=m.mem_read(primary+8,1)[0];out[4]=int(row[0]==2)
        if row[0]!=2:out[5:8]=[word(m,motion+i) for i in [0x58,0x5c,0x60]]
    return out,calls

def common(image,row):
    m=Original(image,127).machine
    primary,player,table=DATA+0x1000,DATA+0x2000,DATA+0x3000
    m.mem_map(0x38f6000,4096);m.mem_map(0x3b18000,4096)
    m.mem_write(primary+4,struct.pack('<H',row[0]));m.mem_write(0x3b18db5,bytes([row[1]]))
    m.mem_write(player+4,struct.pack('<H',row[2]));put(m,0x38f6e80,table)
    slots=[3,14,15,19,20,58,60,62,63];objects=[DATA+0x4000+i*0x100 for i in range(9)]
    for i,slot in enumerate(slots):
        put(m,table+4*slot,objects[i]);put(m,objects[i]+48,objects[(i+1)%9])
    m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_EBX,primary);m.reg_write(UC_X86_REG_EBP,player)
    reads={0x49be38:19,0x49be3b:58,0x49be49:60,0x49be51:20,0x49be54:19,
           0x49be5f:15,0x49be62:14,0x49be71:62,0x49be79:63,0x49be81:3}
    trace=[]
    def hook(machine,address,size,_):
        if address in [0x43fa90,0x49be92]:
            if address==0x43fa90:trace.append(100)
            m.emu_stop();return
        if address in reads:
            trace.append(reads[address])
            if row[3]==len(trace):m.emu_stop()
    m.hook_add(UC_HOOK_CODE,hook);m.emu_start(0x49be10,STOP,count=1000)
    out=[0]*32;out[0]=len(trace);out[1:1+len(trace)]=trace
    out[16:25]=[objects.index(word(m,obj+48)) for obj in objects]
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('probe',type=Path);p.add_argument('--holdout',action='store_true');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    image=PEImage(ROOT/'reference/updated-pc-reference/game/sonic.exe')
    assert image.sha256=='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    lib=ctypes.CDLL(str(a.probe.resolve()));setup_fn=lib.sadx_native_sonic_collision_setup
    setup_fn.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    common_fn=lib.sadx_native_sonic_common_links
    common_fn.argtypes=[ctypes.c_char_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)]
    rng=random.Random(0x495860 if not a.holdout else 0x49be10)
    details=[];observations=hashlib.sha256()
    # Pool exhaustion is an actual native scheduler result; malloc exhaustion
    # leaves enough arena room for source primary calloc, but not MAlloc136.
    for mode in [0,2,1]:
        for case in range(4 if mode!=1 else 1):
            row=[mode,rng.randrange(256),*[rng.getrandbits(32) for _ in range(3)]]
            expected,calls=setup(image,row);out=(ctypes.c_uint32*16)()
            assert setup_fn((ctypes.c_uint32*5)(*row),out)==1
            assert list(out)==expected,(row,list(out),expected)
            details.append(dict(input=row,source_calls=calls,observation=expected))
    cases=512 if a.holdout else 256
    for case in range(cases):
        row=[rng.choice([0,1,256,257,65535]),rng.choice([0,0,1,128]),rng.randrange(65536),case%9]
        expected=common(image,row);out=(ctypes.c_uint32*32)()
        assert common_fn(str(ROOT/'game/imported/characters/sonic-selection.actions').encode(),(ctypes.c_uint32*4)(*row),out)==1
        assert list(out)==expected,(row,list(out),expected)
        observations.update(struct.pack('<4I32I',*row,*out))
    # Append without consuming the established generator or changing its reports.
    exhausted_primary = [3, 7, 101, 202, 303]
    expected, calls = setup(image, exhausted_primary)
    output = (ctypes.c_uint32 * 16)()
    assert setup_fn((ctypes.c_uint32 * 5)(*exhausted_primary), output) == 1
    assert list(output) == expected
    additional = dict(input=exhausted_primary, source_calls=calls, observation=expected)
    report=dict(primary_exhaustion=additional,setup_cases=details,common_cases=cases,common_observation_sha256=observations.hexdigest(),
                source_sha256=image.sha256,helper_body_sha256=hashlib.sha256(image.read(0x495860,91)).hexdigest(),
                limits=['Setup lower calls controlled in original; native uses actual scheduler and arena.','Common-link tables use distinct controlled export identities and compare every sibling and failed-read prefix.'])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print('9 setup cases and',cases,'common-link original prefixes exact')
if __name__=='__main__':main()
