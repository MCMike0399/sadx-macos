"""780f60/780fb0 against original instructions with recording Win32 heap fixtures.

HeapCreate/HeapAlloc/HeapFree return each case's DWORD, including zero and
noncanonical BOOL receipts. Compares EAX, heap global, call order and arguments.
Allocator internals and actual Windows handles/contents are platform facts, not
covered by these fixtures. Bridge tests separately use the live HLE allocator.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x27f).machine
    for s in image.sections:
        if s['name'] in ('.text','.rdata'):continue
        n=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff;m.mem_map(image.base+s['rva'],n);m.mem_write(image.base+s['rva'],image.slice(s['file_offset'],min(s['raw_size'],n)))
    stubs={0x7db078:(CODE+0x400,1),0x7db074:(CODE+0x410,2),0x7db0f8:(CODE+0x420,3)};state={}
    def word(at):return struct.unpack('<I',m.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        kind=next(k for stub,k in stubs.values() if stub==address);esp=mu.reg_read(UC_X86_REG_ESP);c=state['case'];args=[word(esp+4+4*i) for i in range(3)]
        if kind==1:
            if args!=[0,0x800000,0]:raise RuntimeError('HeapCreate flags/maximum')
            handle=value=0;receipt=c[3]
        else:
            handle,flags,value=args
            if flags!=0:raise RuntimeError('Heap flags')
            receipt=c[4] if kind==2 else c[5]
        state['trace'].extend([kind,handle,value,receipt]);mu.reg_write(UC_X86_REG_EAX,receipt)
    for slot,(stub,kind) in stubs.items():
        m.mem_write(slot,struct.pack('<I',stub));m.mem_write(stub,b'\xc2\x0c\x00');m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=stub,end=stub)
    clean=m.context_save();lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_memory;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x780f60);cases=[];bad=[]
    for op in (0,1):
        for n in range(1500):
            c=[op,rng.choice([0xffffffff,0xffffffff,0,0x510000,0x12345678]),rng.choice([0,1,16,4096,0xffffffff,rng.getrandbits(32)]),rng.choice([0,0x510000,0xffffffff,rng.getrandbits(32)]),rng.choice([0,0x50000000,rng.getrandbits(32)]),rng.choice([0,1,2,0x80004005,rng.getrandbits(32)])];cases.append(c)
            out=(ctypes.c_uint32*16)();fn((ctypes.c_uint32*6)(*c),out)
            m.context_restore(clean);state.update(case=c,trace=[]);m.mem_write(0x389d628,struct.pack('<I',c[1]));m.mem_write(STACK,struct.pack('<II',STOP,c[2]));m.reg_write(UC_X86_REG_ESP,STACK)
            m.emu_start(0x780f60 if op==0 else 0x780fb0,STOP,count=1000)
            if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
            want=[1,m.reg_read(UC_X86_REG_EAX),word(0x389d628),int(op==0 and c[1]==0xffffffff),0,0,len(state['trace'])//4]+state['trace'];want.extend([0]*(16-len(want)))
            if list(out)!=want:bad.append(dict(case=len(cases)-1,input=c,native=list(out),source=want))
    a.output.with_suffix('.cases.bin').write_bytes(b''.join(struct.pack('<6I',*c) for c in cases))
    sha=lambda b:hashlib.sha256(b).hexdigest()
    names=['src/engine/ninja_memory.hpp','src/engine/ninja_memory.cpp','tests/probes/native_ninja_memory_probe.cpp','tools/oracles/verify_original_ninja_memory.py']
    report=dict(source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_ranges_sha256={hex(x):sha(image.read(x,n)) for x,n in [(0x780f60,0x2d),(0x780fb0,0x18)]},cases=len(cases),mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(bad))));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
