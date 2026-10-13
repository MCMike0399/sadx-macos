"""CRT srand 6443b5 / rand 6443bf against original instructions over mixed call sequences.

Compares every rand() return and the seed word 2bcbdf8 after each call, including
the 40bf40 reset seed 0xdead0cab.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine;m.mem_map(0x2bcb000,0x1000)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_crt_rand
    rng=random.Random(0x6443bf);bad=0;total=0;sequences=[]
    for n in range(400):
        initial=rng.choice([0,1,0xdead0cab,0xffffffff,rng.getrandbits(32)]);count=rng.randrange(1,200)
        ops=[1 if rng.random()<0.05 else 0 for _ in range(count)];values=[rng.choice([0xdead0cab,0,rng.getrandbits(32)]) for _ in range(count)]
        m.mem_write(0x2bcbdf8,struct.pack('<I',initial));expected=[]
        for op,v in zip(ops,values):
            m.mem_write(STACK,struct.pack('<II',STOP,v));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x6443b5 if op else 0x6443bf,STOP,count=100)
            expected+=[0 if op else m.reg_read(UC_X86_REG_EAX),struct.unpack('<I',m.mem_read(0x2bcbdf8,4))[0]]
        out=(ctypes.c_uint32*(2*count))();fn(ctypes.c_uint32(initial),(ctypes.c_uint32*count)(*ops),(ctypes.c_uint32*count)(*values),ctypes.c_size_t(count),out)
        total+=count;bad+=sum(1 for x,y in zip(out,expected) if x!=y);sequences.append(struct.pack('<II',initial,count)+struct.pack(f'<{count}I',*ops)+struct.pack(f'<{count}I',*values))
    blob=b''.join(sequences)
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_range_sha256=sha(image.read(0x6443b5,0x2d)),sequences=400,calls=total,case_sha256=sha(blob),mismatched_words=bad,returns_and_seed_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(calls=total,mismatched_words=bad)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
