"""Pinned mode-zero continuation, with controlled lower calls and exact prefixes."""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/lib'), str(ROOT / 'tools/oracles')]
from pe_image import PEImage
from verify_original_math import Original, DATA, STACK, STOP
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP

PLAYER, PRIMARY, WORLD, TABLE = DATA + 0x1000, DATA + 0x2000, DATA + 0x3000, DATA + 0x4000
JOINS, METAL_JOINS, METAL, OBJECTS = 0x3c55e28, 0x3c55d68, 0x3b18db5, 0x38f6e80
OBJA, OBJB, EFFECT = DATA + 0x5000, DATA + 0x6000, DATA + 0x7000

def put(m, a, v): m.mem_write(a, struct.pack('<I', v & 0xffffffff))
def word(m, a): return struct.unpack('<I', m.mem_read(a, 4))[0]
def ret(m, result=0):
    sp = m.reg_read(UC_X86_REG_ESP)
    target = word(m, sp)
    m.reg_write(UC_X86_REG_EAX, result)
    m.reg_write(UC_X86_REG_ESP, sp + 4)
    m.reg_write(UC_X86_REG_EIP, target)

def original(image, row):
    m = Original(image, 127).machine
    for base, extent in [(0x3c55000,8192),(0x3b18000,4096),(0x38f6000,4096),(0x3b22000,4096)]:
        m.mem_map(base, extent)
    m.mem_write(METAL, bytes([row[0]]))
    m.mem_write(0x3b22dcc, struct.pack('<H', row[5]))
    m.mem_write(0x3b22dec, struct.pack('<H', row[6]))
    put(m, OBJECTS, TABLE)
    put(m, TABLE + 22*4, [OBJA, OBJB][row[17]-1])
    m.mem_write(PRIMARY + 8, bytes([row[7]]))
    put(m, PLAYER + 0x94, row[10]); put(m, PLAYER + 0x98, row[11])
    put(m, PLAYER + 0x144, 7)
    for table, addr in enumerate([JOINS, METAL_JOINS]):
        for index, obj in enumerate([OBJA, OBJB, 0]):
            record = struct.pack('<IIIBbBBII', obj, 0, 0, 23+index,
                ctypes.c_int8(row[13+table*2+index%2]).value, 45+index, 67+index, 0, 0)
            m.mem_write(addr+24*index, record)
    put(m, STACK, 0); put(m, STACK + 0x40, EFFECT)
    m.reg_write(UC_X86_REG_ESP, STACK)
    m.reg_write(UC_X86_REG_EBP, PLAYER); m.reg_write(UC_X86_REG_EBX, PRIMARY)
    m.reg_write(UC_X86_REG_ESI, WORLD)
    trace, stopped = [], [0]
    motion_count = [0]
    def hook(machine, address, size, _):
        if address == 0x49be10:
            m.emu_stop(); return
        event = None
        sp = m.reg_read(UC_X86_REG_ESP)
        if address == 0x421280: event = 10 + word(m, sp+4)
        elif address == 0x495860: event = 20 + (word(m, sp+4) << 8)
        elif address == 0x44a800: event = 30
        elif address == 0x43fa90: event = 40
        elif address == 0x40b860: event = 50 + word(m, sp+4)
        if event is None: return
        trace.append(event)
        if row[8] == len(trace):
            stopped[0] = len(trace); m.emu_stop(); return
        if address == 0x421280: m.mem_write(METAL, bytes([row[2] if event==18 else row[1]]))
        elif address == 0x44a800:
            assert word(m,sp+4) == PLAYER+0x120
            m.mem_write(METAL, bytes([row[3]])); motion_count[0]+=1
        elif address == 0x43fa90:
            assert [word(m,sp+i) for i in (4,8,12)] == [PRIMARY,WORLD,PLAYER]
            m.mem_write(METAL, bytes([row[4]]))
        elif address == 0x40b860:
            flags = event-50
            assert word(m,sp+8)==3
            assert word(m,sp+12)==(0x493730 if flags==2 else 0x4937b0)
        ret(m,0)
    m.hook_add(UC_HOOK_CODE, hook)
    # Entry at CALL: its first argument is already on the source stack.
    m.emu_start(0x49abeb, STOP, count=10000)
    out=[0]*64
    out[:7]=[stopped[0],m.mem_read(METAL,1)[0],word(m,PLAYER+0x94),word(m,PLAYER+0x98),
             {7:7,JOINS:0,METAL_JOINS:8}[word(m,PLAYER+0x144)],
             (int(word(m,EFFECT+0x18)==0x494860) | (int(word(m,EFFECT+0x14)==0x4948c0)<<1)), len(trace)]
    out[8:8+len(trace)]=trace
    for table,addr in enumerate([JOINS,METAL_JOINS]):
        for index in range(3):
            count,mode,src,dst=struct.unpack('<4B',m.mem_read(addr+24*index+12,4))
            offset=24+table*12+index*4
            out[offset:offset+4]=[mode,count,src,dst]
    return out

def main():
    p=argparse.ArgumentParser();p.add_argument('probe',type=Path);p.add_argument('--holdout',action='store_true');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    image=PEImage(ROOT/'reference/updated-pc-reference/game/sonic.exe')
    assert image.sha256=='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
    lib=ctypes.CDLL(str(a.probe.resolve()));f=lib.sadx_native_sonic_initialization
    f.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2
    rng=random.Random(0x40b220 if a.holdout else 0x49abeb)
    mismatches=[];digest=hashlib.sha256();prefixes=0
    for case in range(640 if a.holdout else 384):
        row=[rng.choice([0,0,1,128,255]) for _ in range(18)]
        row[5]=rng.choice([0,0xffff,0x13,0x12,0x113,0xff13]);row[6]=rng.choice([0,1,0x1300,0xff00,0x1200,0xffff]);row[7]=rng.randrange(8)
        row[8]=case%9;row[10]=rng.getrandbits(32);row[11]=rng.getrandbits(32)
        row[13:17]=[rng.randrange(256) for _ in range(4)];row[17]=rng.choice([1,2])
        expected=original(image,row);out=(ctypes.c_uint32*64)();assert f((ctypes.c_uint32*18)(*row),out)==1
        got=list(out);delta=[[i,x,got[i]] for i,x in enumerate(expected) if x!=got[i]]
        if delta:mismatches.append(dict(case=case,row=row,differences=delta))
        prefixes+=bool(expected[0]);digest.update(struct.pack('<18I64I',*row,*got))
    report=dict(cases=640 if a.holdout else 384,controlled_failure_prefixes=prefixes,mismatches=mismatches,
        observation_sha256=digest.hexdigest(),source_sha256=image.sha256,
        limits=['Lower calls controlled at original entries; no claim of their complete behavior.','Original source stores, three live Metal rereads, table selection and stage/act rule are compared; real resources are tested separately.'])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='mismatches'}));assert not mismatches, mismatches[:2]
if __name__=='__main__':main()
