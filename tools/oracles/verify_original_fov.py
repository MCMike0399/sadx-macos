"""Field-of-view setter 7815c0 (787e30, 788290, 7883c0 real code, source sine table) against original instructions.

Compares the stored projection scale 3d0f9e0 bitwise across angles, widths and
x87 control words; status flags are reported separately.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine;m.mem_map(0x3d0f000,0x1000);clean=m.context_save()
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_fov
    fn.argtypes=[ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_float),ctypes.c_int32,ctypes.c_uint32,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)]
    table=(ctypes.c_float*1025).from_buffer_copy(image.read(0x88af78,4100));unit=(ctypes.c_float*16).from_buffer_copy(image.read(0x389d650,64))
    rng=random.Random(0x7815c0);bad=[];flags=0;cases=[]
    for angle in[0x31c7,0x2000,0x3000,0x4000,0,1,-1,0x8000,-0x31c7,0x7fffffff,-0x80000000,0x10000,0x18e3]+[rng.randrange(-0x20000,0x20000) for _ in range(500)]:
        for width in(f32(640.0),f32(320.0),f32(rng.uniform(1,2000)),0x7fc00000,0):cases.append((angle,width))
    for cw in(0x7f,0x27f,0x37f,0xc7f):
        for angle,width in cases:
            m.context_restore(clean);m.mem_write(0x3d0f9e4,struct.pack('<I',width));m.mem_write(0x3d0f9e0,b'\xef\xbe\xad\xde')
            m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
            m.mem_write(STACK,struct.pack('<Ii',STOP,angle));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x7815c0,STOP,count=2000)
            src=struct.unpack('<I',m.mem_read(0x3d0f9e0,4))[0];out=(ctypes.c_uint32*4)();fn(table,unit,angle,width,cw,out)
            if not out[0] or out[2]!=src:bad.append(dict(cw=hex(cw),angle=angle,width=hex(width),source=hex(src),native=hex(out[2]),completed=out[0],boundary=hex(out[1])))
            if out[3]!=(m.reg_read(UC_X86_REG_FPSW)&0x3f):flags+=1
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),original_range_sha256=sha(image.read(0x7815c0,0x1f)+image.read(0x787e30,0x1b)),cases=4*len(cases),mismatch_count=len(bad),fp_flag_mismatches=flags,mismatches=bad[:20],scale_matches=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=4*len(cases),mismatches=len(bad),fp_flag_mismatches=flags)));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
