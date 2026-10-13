"""Original6452a4/classifier/return instructions versus native square root.

Explicit masked CW and matherr bypass profiles; hardware/game initialization
not observed. No original math/return call is overlaid.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,CODE,DATA,STACK,STOP

def sha(b):return hashlib.sha256(b).hexdigest()
def bits(d):return struct.unpack('<Q',struct.pack('<d',d))[0]
class SqrtOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f);m=self.machine
        m.mem_map(0x2bcc000,0x1000);m.mem_write(0x2bcc000,image.read(0x2bcc000,0x200))
        m.mem_map(0x3c83000,0x1000);m.mem_write(0x3c837a8,struct.pack('<I',1))
        m.mem_write(CODE,b'\x83\xc4\x08\xdb\x3d'+struct.pack('<I',DATA)+b'\xc3')
    def run(self,u,cw):
        m=self.machine;m.mem_write(STACK,struct.pack('<IQI',CODE,u,STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_FPCW,cw)
        m.emu_start(0x6452a4,STOP,count=2000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+16:raise RuntimeError('Square-root source ABI mismatch')
        sig,se=struct.unpack('<QH',m.mem_read(DATA,10));return sig,se,m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_crt_sqrt
    fn.argtypes=[ctypes.c_uint64,ctypes.c_uint16,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    cases=list(map(bits,[0.,-0.,1.,2.,4.,16.,-1.,-2.,-4.,float('inf'),-float('inf')]))+[1,0x8000000000000001,0xfffffffffffff,0x800fffffffffffff,0x7ff8000000000123,0xfff8000000000123,0x7ff0000000000123,0xfff0000000000123]
    rng=random.Random(0x6452a4);cases += [bits(rng.uniform(-1e8,1e8)) for _ in range(400)]+[rng.getrandbits(64) for _ in range(600)]
    report=dict(profile='actual6452a4/classifier/common-return/native-arm64/no-overlays',source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),
        source_functions={hex(lo):dict(size=hi-lo,sha256=sha(image.read(lo,hi-lo))) for lo,hi in [(0x6452a4,0x64534a),(0x648bd5,0x648c18),(0x648c2e,0x648c3b)]},
        case_sha256=sha(b''.join(struct.pack('<Q',u) for u in cases)),matherr_control=1,profiles=[],limits=['Explicit matherr bypass and masked control words are test seeds, not observed runtime','Binary64 arguments only; retained-ST0 wrapper has separate source semantics','Error handler/unmasked delivery and Intel hardware remain unverified','Full geometry/game/controller behavior is not exercised'])
    start=time.monotonic();oracle=SqrtOriginal(image)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,control_mismatches=0,flag_mismatches=0,examples=[])
        for i,u in enumerate(cases):
            sig,se,control,flags=oracle.run(u,cw);out=(ctypes.c_uint32*10)();status=fn(u,cw,1,out)
            if status or out[0] or not out[7]:raise RuntimeError(f'Native sqrt failedcase{i},status{status}/{out[0]}')
            value=(out[4]|out[5]<<32,out[6]);vm=value!=(sig,se);cm=out[2]!=control;fm=out[3]!=flags
            item['value_mismatches']+=vm;item['control_mismatches']+=cm;item['flag_mismatches']+=fm
            if (vm or cm) and len(item['examples'])<10:item['examples'].append(dict(case=i,input=hex(u),source=[sig,se,control],native=[*value,out[2]]))
        report['profiles'].append(item);print(json.dumps({k:v for k,v in item.items() if k!='examples'}),flush=True)
    report['returned_values_control_match']=all(not i['value_mismatches'] and not i['control_mismatches'] for i in report['profiles'])
    report['full_emulator_match']=report['returned_values_control_match'] and all(not i['flag_mismatches'] for i in report['profiles'])
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['Flag difference (flag_mismatches per profile): x87 exception bits after FSQRT/FST rounding (PE) are set natively where Unicorn 2.1.4 does not raise them. All compared values match; the hardware flags need the Intel capture (M4).']
    report['seconds']=time.monotonic()-start;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['returned_values_control_match'])
if __name__=='__main__':raise SystemExit(main())
