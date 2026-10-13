"""Actual43c7d0 and its vector/normalization callees, without overlays."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
from verify_original_alternate_acceleration import ROTATION_FLOATS,ROTATION_DOUBLES

PLAYER=DATA+0x100;MOTION=DATA+0x500;CONTACT=DATA+0x600;OFFSET=DATA+0x700
CONSTANTS=(0x7dcb2c,0x7dd584,0x7dd58c,0x7dd540,0x7e0078,0x7e0074,0x7e0070,0x7e006c,0x7e0068,0x7e0064,0x7dff54,0x7e0060,0x7dcc98)
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
class ContactOriginal(Original):
    def __init__(self,image,cw):
        super().__init__(image,cw);self.machine.mem_map(0x3b0f000,0x1000)
    def run(self,w,cw):
        m=self.machine
        m.mem_write(PLAYER+0x60,struct.pack('<I',w[0]));m.mem_write(PLAYER+0x10,struct.pack('<H',w[1]));m.mem_write(PLAYER+0x38,struct.pack('<3I',*w[2:5]));m.mem_write(PLAYER+0x9c,struct.pack('<33I',*w[17:50]))
        m.mem_write(MOTION+4,struct.pack('<3I',*w[5:8]));m.mem_write(OFFSET,struct.pack('<3I',*w[8:11]));m.mem_write(CONTACT+0x1c,struct.pack('<3I',*w[11:14]));m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[14:17]))
        m.mem_write(STACK,struct.pack('<2I',STOP,CONTACT));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_EAX,MOTION);m.reg_write(UC_X86_REG_ESI,PLAYER);m.reg_write(UC_X86_REG_EDI,OFFSET)
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.emu_start(0x43c7d0,STOP,count=5000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Return ABI mismatch')
        return list(struct.unpack('<3I',m.mem_read(OFFSET,12))),m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)
def fixtures(parameters):
    rng=random.Random(0x43c7d0);result=[]
    for i in range(3600):
        attributes=rng.choice((0,8,0x4000,0x20000,0x24008));flags=rng.choice((0,1,2,3,0x2000,0x2003))
        values=[rng.uniform(-12,12) for _ in range(15)]
        # Contact direction and gravity exercise dot thresholds without host
        # normalization: caller-selected source facts are kept as supplied.
        result.append([attributes,flags,*map(word,values),*parameters])
    raw=(0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345)
    for attrs in (0,8,0x4000,0x20000):
        for flags in (0,3,0x2000):
            for v in raw:
                for index in (2,3,6,7,9,11,13,14,16,18):
                    w=[attrs,flags,*map(word,(3,-1,0,1,0,2,99,1,98,0,1,0,0,-1,0)),*parameters];w[index]=v;result.append(w)
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    parameters=list(struct.unpack('<33I',image.read(0x9154e8,132)));cases=fixtures(parameters)
    lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_contact_initializer
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    lib.sadx_native_player_contact_initializer_compiler.restype=ctypes.c_char_p
    cb=(ctypes.c_uint32*13)(*[image.unpack('<I',addr)[0] for addr in CONSTANTS]);rotation=b''.join(image.read(addr,4) for addr in ROTATION_FLOATS)+b''.join(image.read(addr,8) for addr in ROTATION_DOUBLES);rb=(ctypes.c_uint8*88).from_buffer_copy(rotation)
    blob=b''.join(struct.pack('<50I',*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parent.parent
    paths=['src/'+n+ext for n in ('player_contact_initializer','player_rotation','ninja_math','x87_math') for ext in ('.hpp','.cpp')]+['src/player/player_contact.hpp','src/player/player_physics.hpp','src/player/player_acceleration.hpp','tests/probes/native_player_contact_initializer_probe.cpp']
    report=dict(source_sha256=image.sha256,function=dict(address='0x43c7d0',end_exclusive='0x43ca3c',sha256=sha(image.read(0x43c7d0,0x26c))),library_sha256=sha(a.library.read_bytes()),native_sources={path:sha((root/path).read_bytes()) for path in paths},compiler=lib.sadx_native_player_contact_initializer_compiler().decode(),verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),fixture_sha256=sha(blob),fixture_count=len(cases),control_words=[],mismatches=[],limits=['Explicit caller data and masked experimental CW profiles, not observed startup','All original instructions/callees execute without overlays','Raw/nonfinite values are preserved','Unicorn status/transcendental limitations; no original Intel hardware observation'])
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        oracle=ContactOriginal(image,cw);item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0,cw_mismatches=0)
        for index,w in enumerate(cases):
            original,flags,restored=oracle.run(w,cw);out=(ctypes.c_uint32*7)();status=probe(cb,rb,(ctypes.c_uint32*50)(*w),cw,out)
            if status or out[3]:raise RuntimeError(f'Native failure{status}/{out[3]} at{out[4]:#x}')
            differences=[]
            for label,x,y in (('value',original,list(out[:3])),('flag',flags,out[5]),('cw',restored,out[6])):
                if x!=y:item[label+'_mismatches']+=1;differences.append(label)
            if differences:report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),original=list(map(hex,original)),native=list(map(hex,out[:3])),flags=[hex(flags),hex(out[5])]))
        report['control_words'].append(item);print(json.dumps(item),flush=True)
    report['values_cw_match']=all(not x['value_mismatches'] and not x['cw_mismatches'] for x in report['control_words']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_cw_match'] else 1
if __name__=='__main__':raise SystemExit(main())
