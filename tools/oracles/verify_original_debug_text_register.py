"""793e70 (781070's 78115d) against its original instructions, with the real
77ffc0. Cases seed the callback count 3ce712c (0..31) and the 32 slots at
3d0fa40. Compared: the count, the slots and EAX.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    clean=m.context_save()
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_debug_text_register;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x793e70);bad=[];cases=0
    for n in range(500):
        c=[rng.randrange(32)]+[rng.getrandbits(32) for _ in range(32)]
        out=(ctypes.c_uint32*40)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);m.mem_write(0x3ce712c,struct.pack('<I',c[0]));m.mem_write(0x3d0fa40,struct.pack('<32I',*c[1:33]))
        m.reg_write(UC_X86_REG_EAX,0x12345678);m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x793e70,STOP,count=1000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        e=[0]*40;e[0]=1;e[1]=struct.unpack('<I',m.mem_read(0x3ce712c,4))[0]
        for i in range(32):e[2+i]=struct.unpack('<I',m.mem_read(0x3d0fa40+4*i,4))[0]
        e[34]=m.reg_read(UC_X86_REG_EAX)
        cases+=1;d=[i for i in range(40) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/ninja/ninja_debug_text.hpp','src/ninja/ninja_debug_text.cpp','src/ninja/ninja_keyboard.cpp','tests/probes/native_debug_text_register_probe.cpp','tools/oracles/verify_original_debug_text_register.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x793e70-0x793e80':sha(image.read(0x793e70,0x10)),'0x77ffc0':sha(image.read(0x77ffc0,0x17)),'0x78c5a0':sha(image.read(0x78c5a0,1))},
        cases=cases,source_outcomes={'completed':cases},mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=1,mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
