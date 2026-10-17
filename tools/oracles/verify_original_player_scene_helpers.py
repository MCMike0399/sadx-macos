"""Actual43cb40/444f10/43ca40 instructions, explicit resource profiles."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_player_scene import SceneOriginal,MATRIX
from verify_original_math import native_source,DATA

ACTOR=DATA+0x100;WORLD=DATA+0x200;PLAYER=DATA+0x300;CONTACT=DATA+0x600;MOVING=DATA+0x900;FORCE=DATA+0xa00;OFFSET=DATA+0xb00
FLOATS=(0x7dcb10,0x7dcb2c,0x7dcc74,0x7dcc98,0x7dccd8,0x7dd540,0x7dd550,0x7dd570,0x7dd578,0x7dd584,0x7dd58c,0x7dfff0,0x7e0084,0x7e0088,0x7e008c,0x7e00e4,0x7e0118,0x7e0128,0x7e012c,0x7e0174,0x7e0178,0x7e017c,0x7e0180,0x7e0184,0x7e0188)
DOUBLES=(0x7dccc8,0x7dccd0,0x7dcce8)
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
class HelperOriginal(SceneOriginal):
    def run(self,mode,w,cw,parameters):
        m=self.machine;m.mem_write(ACTOR,bytes(64));m.mem_write(WORLD,bytes(64));m.mem_write(PLAYER,bytes(0x220));m.mem_write(CONTACT,bytes(132));m.mem_write(MOVING,bytes(0x34));m.mem_write(FORCE,bytes(56));m.mem_write(MATRIX,self.unit*4)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,4),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(ACTOR+0x20,struct.pack('<3I',*w[:3]));m.mem_write(ACTOR+4,struct.pack('<H',w[6]&0xffff));m.mem_write(ACTOR+0x14,struct.pack('<3I',*w[7:10]));m.mem_write(ACTOR+8,bytes([w[13]&255]))
        m.mem_write(WORLD+4,struct.pack('<3I',*w[3:6]));m.mem_write(PLAYER+0x10,struct.pack('<H',w[14]&0xffff));m.mem_write(PLAYER+0x38,struct.pack('<3I',*w[15:18]));m.mem_write(PLAYER+0x9c,parameters)
        for offset,start in ((0x1c,26),(0x3c,29),(0x48,32),(4,35)):m.mem_write(CONTACT+offset,struct.pack('<3I',*w[start:start+3]))
        m.mem_write(CONTACT+0x34,struct.pack('<2I',*w[10:12]));m.mem_write(0x3b0f0f8,struct.pack('<3I',*w[38:41]))
        if mode==0:
            m.reg_write(UC_X86_REG_EAX,ACTOR);m.reg_write(UC_X86_REG_ECX,CONTACT);m.reg_write(UC_X86_REG_EDX,WORLD);self.invoke(0x43cb40,cw)
        elif mode==1:
            m.mem_write(PLAYER+0x38,struct.pack('<I',w[12]));m.reg_write(UC_X86_REG_EBX,CONTACT);m.reg_write(UC_X86_REG_EDI,ACTOR);self.invoke(0x444f10,cw,(PLAYER,))
        else:
            m.mem_write(PLAYER+0x6c,struct.pack('<I',MOVING));m.mem_write(MOVING+0x28,struct.pack('<I',FORCE if mode==3 else 0));m.mem_write(OFFSET,struct.pack('<3I',*w[17:20]))
            row=struct.pack('<7I',0,*w[23:26],*w[20:23]);m.mem_write(FORCE,row*2);m.reg_write(UC_X86_REG_EAX,PLAYER);m.reg_write(UC_X86_REG_ECX,OFFSET);m.reg_write(UC_X86_REG_ESI,ACTOR);self.invoke(0x43ca40,cw,(CONTACT,))
        values=[0,0];values.extend(struct.unpack('<3I',m.mem_read(ACTOR+0x20,12)));values.extend(struct.unpack('<3I',m.mem_read(ACTOR+0x14,12)));values.extend(struct.unpack('<3I',m.mem_read(WORLD+4,12)));values.extend(struct.unpack('<3I',m.mem_read(CONTACT+4,12)));values.extend((m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0]));values.extend(struct.unpack('<64I',m.mem_read(MATRIX,256)));return values
def fixtures():
    rng=random.Random(0x444f10);cases=[]
    for mode in range(4):
        for i in range(160):
            w=list(map(word,(rng.uniform(-5,5) for _ in range(41))));w[6]=rng.choice((0,1,2,3,0x2000));w[7:12]=[rng.randrange(0x10000) for _ in range(5)];w[12]=word(rng.choice((0,1,2,5,10,15,20)));w[13]=rng.choice((0,1,5,255));w[14]=rng.choice((0,1,2,3,0x2000));w[23:26]=[rng.getrandbits(32) for _ in range(3)]
            if i%4==0:w[15:18]=[0,0,0]
            cases.append((mode,w))
    for mode in (0,1,3):
        for raw in (0,0x80000000,1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc12345,0x7f812345,0xff812345):
            w=[word(1)]*41;w[6]=1;w[7:12]=[0]*5;w[12]=word(2);w[13]=0;w[14]=1;w[23:26]=[0]*3;w[29]=raw;w[32]=raw;w[20]=raw;cases.append((mode,w))
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_scene_helpers;probe.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int;lib.sadx_native_player_scene_helpers_compiler.restype=ctypes.c_char_p
    constants=b''.join(image.read(address,4) for address in FLOATS)+b''.join(image.read(address,8) for address in DOUBLES);parameters=image.read(0x9154e8,132);unit=image.read(0x389d650,64)
    buffers=[(ctypes.c_uint8*len(data)).from_buffer_copy(data) for data in (constants,parameters,image.read(TABLE_ADDRESS,4100),unit)]
    cases=fixtures();blob=b''.join(struct.pack('<42I',mode,*w) for mode,w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];names=('player_scene_position_helpers','player_scene_position','player_coordinates','ninja_matrix','ninja_math','x87_math','x87_transcendental','crt_integer');paths=[f'src/{name}.cpp' for name in names if name!='player_scene_position']+[f'src/{name}.hpp' for name in names if name!='player_scene_position_helpers']+['tests/probes/native_player_scene_helpers_probe.cpp']
    # Follow the native/reference split without changing cases or comparisons.
    paths += [str(path.relative_to(root)) for pattern in
              ('src/player/player_scene_position*', 'src/guest/player_scene_position_reference*')
              for path in sorted(root.glob(pattern)) if path.is_file()
              and str(path.relative_to(root)) not in paths]
    paths += ['src/cpu/fidelity_float.hpp']
    report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_scene_helpers_compiler().decode(),sources={path:sha((native_source(root,path)).read_bytes()) for path in paths},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),case_sha256=sha(blob),profiles=[],mismatches=[],limits=['Actual43cb40/444f10/43ca40 instructions without overlays','Platform profiles have observed-null task+28 or observed-null row callback; nonnull callback execution remains separately unavailable','Caller-selected source parameter row, task/contact vectors, fixed matrix stack and masked CWs are synthetic state facts','FPATAN uses explicitly selected Bochs P5/P6 native model; Unicorn uses host-double and incomplete exception semantics','Unnormalized/raw nonfinite fixtures are retained without native value adjustments','Full449500/445d10 positioning is not established by these leaf checks'])
    oracle=HelperOriginal(image,0x27f,unit)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0)
        for index,(mode,w) in enumerate(cases):
            original=oracle.run(mode,w,cw,parameters);out=(ctypes.c_uint32*81)();status=probe(mode,(ctypes.c_uint32*41)(*w),*buffers,cw,out)
            if status:raise RuntimeError(f'Native mode{mode}/case{index} failed{status}/{out[0]} at{out[1]:#x}')
            differences=[i for i in range(81) if i!=15 and original[i]!=out[i]]
            if differences:item['value_mismatches']+=1;report['mismatches'].append(dict(cw=hex(cw),mode=mode,case=index,input=list(map(hex,w)),differences=[dict(word=i,original=hex(original[i]),native=hex(out[i])) for i in differences]))
            if original[15]!=out[15]:item['flag_mismatches']+=1
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['values_cw_frames_match']=all(not x['value_mismatches'] for x in report['profiles']);report['strict_match']=report['values_cw_frames_match'] and all(not x['flag_mismatches'] for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_cw_frames_match'] else 1
if __name__=='__main__':raise SystemExit(main())
