"""Original 795820/7887d0/788d40 instruction comparison, no replacements."""
import argparse
import ctypes
import hashlib
import json
import pathlib
import random
import struct
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG)
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,CODE,DATA,STACK,STOP

PLANES=DATA+0x100
RETURN=DATA+0x300
def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def sha(value):return hashlib.sha256(value).hexdigest()
class PlaneOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        self.machine.mem_write(CODE,b'\xdb\x3d'+struct.pack('<I',RETURN)+b'\xc3')
    def run(self,mode,words,alias,cw):
        m=self.machine;m.mem_write(PLANES,struct.pack('<18I',*words))
        m.reg_write(UC_X86_REG_ECX,PLANES);m.reg_write(UC_X86_REG_EDX,PLANES+24)
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        if mode==0:
            args=(STOP,);entry=0x795820
        else:
            output=0 if alias==3 else PLANES+24*(2 if alias==0 else alias-1)
            args=(CODE,output,STOP);entry=0x7887d0
        m.mem_write(STACK,struct.pack('<'+'I'*len(args),*args));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(entry,STOP,count=3000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4*len(args):raise RuntimeError('Original plane ABI did not return')
        result=list(struct.unpack('<18I',m.mem_read(PLANES,72)))
        if mode==0:result += [m.reg_read(UC_X86_REG_EAX),0,0,0]
        else:
            sig,se=struct.unpack('<QH',m.mem_read(RETURN,10));result += [0,sig&0xffffffff,sig>>32,se]
        return result+[m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)]

def fixtures(image):
    sentinel=list(map(word,(9,8,7,6,5,4)));result=[]
    def add(a,b,aliases=(0,1,2,3)):
        words=list(map(word,(*a,*b)))+sentinel
        result.append((0,0,words))
        for alias in aliases:result.append((1,alias,words))
    add((2,0,0,1,0,0),(0,3,0,0,1,0))
    add((2,0,0,1,0,0),(5,0,0,1,0,0))
    add((2,0,0,1,0,0),(5,0,0,-1,0,0))
    add((2,0,0,0,0,0),(5,0,0,1,0,0))
    rng=random.Random(0x7887d0)
    for _ in range(200):
        add(tuple(rng.uniform(-10,10) for _ in range(6)),tuple(rng.uniform(-10,10) for _ in range(6)))
    threshold=image.unpack('<I',0x7e6c30)[0]
    # Two unit normals near the exact source threshold; lengths are still
    # recomputed/spilled by the original classifier rather than assumed one.
    for delta in range(-3,4):
        x=struct.unpack('<f',struct.pack('<I',threshold+delta))[0]
        add((0,0,0,1,0,0),(1,2,3,x,(1-x*x)**0.5,0))
    for raw in (0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,
                0x7fc12345,0xffc54321,0x7f812345,0xff812345):
        for slot in range(12):
            words=list(map(word,(2,0,0,1,0,0,0,3,0,0,1,0)))+sentinel;words[slot]=raw
            result.append((0,0,words));result.append((1,0,words));result.append((1,3,words))
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path)
    p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_plane
    probe.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint32,ctypes.c_uint16,ctypes.c_float,ctypes.POINTER(ctypes.c_uint32)]
    probe.restype=ctypes.c_int;cases=fixtures(image);threshold=image.unpack('<f',0x7e6c30)[0]
    blob=b''.join(struct.pack('<20I',mode,alias,*words) for mode,alias,words in cases)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parent.parent
    report=dict(profile='actual795820/7887d0/788d40/no-overlays; output alias and null profiles',
        source_sha256=image.sha256,native_library_sha256=sha(a.library.read_bytes()),
        source_functions={hex(lo):dict(size=hi-lo,sha256=sha(image.read(lo,hi-lo))) for lo,hi in
            ((0x795820,0x7958c9),(0x7887d0,0x7889e0),(0x7871f0,0x78721b),(0x788d40,0x788df0))},
        native_sources={name:sha((root/name).read_bytes()) for name in
            ('src/player/player_plane.cpp','src/player/player_plane.hpp','src/ninja/ninja_math.cpp',
             'src/player/player_direction_reset.cpp','tests/probes/native_player_plane_probe.cpp','tools/oracles/verify_original_player_plane.py')},
        case_sha256=sha(blob),cases=len(cases),profiles=[],mismatches=[],limits=[
            'Masked control words and synthetic planes are selected test fixtures; no runtime or hardware observation',
            'No original math calls are overlaid; return/flags are captured after source instruction execution',
            'Unicorn nonfinite operand/status limitations remain; full contact response and game are not exercised'])
    oracle=PlaneOriginal(image)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,return_mismatches=0,flag_mismatches=0,control_mismatches=0)
        for index,(mode,alias,words) in enumerate(cases):
            expected=oracle.run(mode,words,alias,cw);out=(ctypes.c_uint32*24)()
            status=probe(mode,(ctypes.c_uint32*18)(*words),alias,cw,threshold,out)
            if status:raise RuntimeError(f'Native plane failed case{index}: {status}')
            actual=list(out);different=[]
            for field,x,y in (('state',expected[:18],actual[:18]),('return',expected[18:22],actual[18:22]),
                ('flag',expected[22],actual[22]),('control',expected[23],actual[23])):
                if x!=y:item[field+'_mismatches']+=1;different.append(field)
            if any(field!='flag' for field in different):
                report['mismatches'].append(dict(cw=hex(cw),case=index,mode=mode,alias=alias,
                    input=list(map(hex,words)),fields=different,source=list(map(hex,expected)),native=list(map(hex,actual))))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['state_returns_control_match']=all(not any(item[key] for key in
        ('state_mismatches','return_mismatches','control_mismatches')) for item in report['profiles'])
    report['full_emulator_match']=report['state_returns_control_match'] and all(
        not item['flag_mismatches'] for item in report['profiles'])
    a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['state_returns_control_match'])
if __name__=='__main__':raise SystemExit(main())
