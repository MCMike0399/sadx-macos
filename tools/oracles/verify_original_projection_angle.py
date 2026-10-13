"""Actual438410 with original dot/acos/ftol/cross and explicit matherr bypass.

Complete local signed-vector-angle body; not startup, mods or hardware proof.
Original goldens check return angle/control word and independently execute
canonical cross for its binary32 stores/ext80 length; flags are separate.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,CODE,DATA,STACK,STOP
FIRST=DATA+0x100;SECOND=DATA+0x200;SIDE=DATA+0x300;CRT=0x3c837a8;SEED=0x4384107889f0
def sha(v):return hashlib.sha256(v).hexdigest()
def fword(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class OriginalAngle(Original):
    def __init__(self,image,cw):
        super().__init__(image,cw);self.machine.mem_map(0x2bcc000,0x1000);self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000));self.machine.mem_map(CRT&~0xfff,0x1000)
        address=CODE+0x500;self.machine.mem_write(address,b'\x68'+struct.pack('<I',DATA+0x500)+b'\xe8'+struct.pack('<i',0x7889f0-(address+10))+b'\xdb\x3d'+struct.pack('<I',DATA+0x600)+b'\xc3')
    def run(self,w,cw):
        m=self.machine;m.mem_write(FIRST,struct.pack('<3I',*w[:3]));m.mem_write(SECOND,struct.pack('<3I',*w[3:6]));m.mem_write(SIDE,struct.pack('<3I',*w[6:9]));m.mem_write(CRT,struct.pack('<I',w[11]));m.mem_write(STACK,struct.pack('<4I',STOP,FIRST,SECOND,SIDE if w[9] else 0));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(0x438410,STOP,count=10000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original vector-angle ABI mismatch')
        return m.reg_read(UC_X86_REG_EAX),m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&0x3f
    def run_cross(self,w,cw):
        m=self.machine;m.mem_write(FIRST,struct.pack('<3I',*w[:3]));m.mem_write(SECOND,struct.pack('<3I',*w[3:6]));m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,FIRST);m.reg_write(UC_X86_REG_EDX,SECOND);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(CODE+0x500,STOP,count=1000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original cross ABI mismatch')
        low,high,exponent=struct.unpack('<IIH',m.mem_read(DATA+0x600,10));return [*struct.unpack('<3I',m.mem_read(DATA+0x500,12)),exponent,low,high,m.reg_read(UC_X86_REG_FPSW)&0x3f]
def fixtures():
    axes=((1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),(0,0,0),(0.5,0.25,0.75))
    result=[[*map(fword,a),*map(fword,b),*map(fword,s),present,1,1] for a in axes for b in axes for s in ((0,1,0),(0,-1,0),(0,0,0)) for present in (0,1)]
    rng=random.Random(SEED)
    for _ in range(600):result.append([*map(fword,(rng.uniform(-1,1) for _ in range(9))),rng.randrange(2),1,1])
    for raw in (0x3f7fffff,0x3f800000,0x3f800001,0xbf7fffff,0xbf800000,0xbf800001,0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321):
        result.append([raw,0,0,fword(1),0,0,0,fword(1),0,1,1,1])
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);args=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Requires Unicorn2.1.4')
    image=PEImage(args.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source image changed')
    constants=image.read(0x7dcb2c,4)+image.read(0x7dcb10,4)+image.read(0x7dcb0c,4)+image.read(0x7dccd0,8)+image.read(0x7dccc8,8);cases=fixtures();args.output.parent.mkdir(parents=True,exist_ok=True);blob=b''.join(struct.pack('<12I',*w) for w in cases);path=args.output.with_suffix('.cases.bin');path.write_bytes(blob)
    lib=ctypes.CDLL(str(args.native_library.resolve()));fn=lib.sadx_native_projection_angle;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int;buffer=(ctypes.c_ubyte*28).from_buffer_copy(constants)
    crossfn=lib.sadx_native_projection_cross;crossfn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];crossfn.restype=ctypes.c_int
    report=dict(profile='complete original438410 with real acos/ftol/cross; no overlays',source_sha256=image.sha256,native_library_sha256=sha(args.native_library.read_bytes()),constants_sha256=sha(constants),case_blob=dict(path=str(path),sha256=sha(blob),record='12 little-endian uint32 words'),seed=hex(SEED),source_functions={hex(a):dict(size=b-a,sha256=sha(image.read(a,b-a))) for a,b in ((0x438410,0x4384b8),(0x7889f0,0x788a48))},control_words=[],limits=['CRT matherr bypass policy1 is explicitly selected','Bochs FPATAN and Unicorn oracle are not original hardware','Original angle/CW and independently executed cross stores/ext80 length are checked; native pending-error facts use owner tests','Flags and actual startup/mod FP state remain unverified'])
    for cw in (p|r|0x7f for p in (0,0x200,0x300) for r in (0,0x400,0x800,0xc00)):
        original=OriginalAngle(image,cw);item=dict(control_word=hex(cw),cases=len(cases),angle_control_mismatches=0,flag_mismatches=0,cross_cases=len(cases),cross_store_length_mismatches=0,cross_flag_mismatches=0,examples=[],flag_examples=[])
        for index,w in enumerate(cases):
            expected=original.run(w,cw);out=(ctypes.c_uint32*15)();status=fn(buffer,28,(ctypes.c_uint32*12)(*w),12,cw,out,15)
            if status or out[1]:raise RuntimeError(f'Native rejectedcase{index}:status{status},failure{out[1]}')
            different=(out[0],out[11])!=expected[:2];item['angle_control_mismatches']+=different;item['flag_mismatches']+=out[10]!=expected[2]
            if different:item['examples'].append(dict(case=index,original=list(map(hex,expected)),native=[hex(out[0]),hex(out[11]),hex(out[10])]))
            if out[10]!=expected[2] and len(item['flag_examples'])<5:item['flag_examples'].append(dict(case=index,original=hex(expected[2]),native=hex(out[10])))
            expected_cross=original.run_cross(w,cw);cross=(ctypes.c_uint32*7)()
            if crossfn((ctypes.c_uint32*6)(*w[:6]),cw,cross):raise RuntimeError('Native cross rejected fixture')
            cross_different=list(cross[:6])!=expected_cross[:6];item['cross_store_length_mismatches']+=cross_different;item['cross_flag_mismatches']+=cross[6]!=expected_cross[6]
            if cross_different:item['examples'].append(dict(case=index,function='cross7889f0',original=list(map(hex,expected_cross)),native=list(map(hex,cross))))
        report['control_words'].append(item);print(json.dumps({k:v for k,v in item.items() if k not in ('examples','flag_examples')}),flush=True)
    report['return_angle_control_and_cross_stores_length_match']=all(i['angle_control_mismatches']==0 and i['cross_store_length_mismatches']==0 for i in report['control_words']);args.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['return_angle_control_and_cross_stores_length_match'])
if __name__=='__main__':raise SystemExit(main())
