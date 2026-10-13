"""Vanilla InitControllers 40f3a0 against original instructions.

The source runs on a pseudo-random 3b0e000 page; the comparison covers the
four ControllersRaw records (incl. the overlapping forward replication),
ControllerPointers[0..3] targets and every other byte the function writes or
must leave untouched on that page. The Input mod's RET hook at 40f451 is not
applied here; its repoint is input_mod_init_raw_controllers.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
CLEARED=[(0x3b0e768,16),(0x3b0e344,16),(0x3b0e9b8,16),(0x3b0e9a4,4),(0x3b0e990,20)]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_controller_init;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    m=Original(image,0x7f).machine;m.mem_map(0x3b0e000,0x1000);clean=m.context_save();rng=random.Random(0x40f3a0);bad=[];untouched_bad=0;cases=0
    for case in range(300):
        page=bytes(rng.getrandbits(8) for _ in range(0x1000)) if case else bytes(0x1000)
        template=list(struct.unpack('<13I',page[0xaa8:0xaa8+52]))
        m.context_restore(clean);m.mem_write(0x3b0e000,page);m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x40f3a0,STOP,count=2000)
        after=bytes(m.mem_read(0x3b0e000,0x1000));cases+=1
        raw=list(struct.unpack('<52I',after[0x9c8:0x9c8+208]));pointers=[(struct.unpack('<I',after[0x77c+4*i:0x780+4*i])[0]-0x3b0e9c8)//0x34 for i in range(4)]
        cleared=all(after[x-0x3b0e000:x-0x3b0e000+n]==bytes(n) for x,n in CLEARED)
        written=set(range(0x9c8,0x9c8+208))|set(range(0x77c,0x78c))
        for x,n in CLEARED:written|=set(range(x-0x3b0e000,x-0x3b0e000+n))
        if any(after[i]!=page[i] for i in range(0x1000) if i not in written):untouched_bad+=1
        out=(ctypes.c_uint32*60)()
        if fn((ctypes.c_uint32*13)(*template),13,out,60)!=1:raise RuntimeError('Native rejected')
        expected=raw+pointers+[int(cleared)]
        if list(out)[:57]!=expected:bad.append(dict(case=case,fields=[i for i,(x,y) in enumerate(zip(list(out)[:57],expected)) if x!=y][:10]))
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_range_sha256=sha(image.read(0x40f3a0,0xb2)),cases=cases,mismatch_count=len(bad),mismatches=bad[:20],
        source_untouched_bytes_changed=untouched_bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,mismatches=len(bad),untouched_violations=untouched_bad)));return int(bool(bad or untouched_bad))
if __name__=='__main__':raise SystemExit(main())
