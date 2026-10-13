"""Real ECoast registry -> full449500/445d10 instructions, no overlays."""
import argparse,collections,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import DATA
from verify_original_player_mesh_registry import RegistryOriginal
from verify_original_player_scene import MATRIX
from verify_original_player_scene_helpers import FLOATS,DOUBLES,word,sha
from verify_original_player_contact_attributes import CONSTANTS as ATTRIBUTES
from verify_original_floor_angles import CONSTANTS as FLOOR

ACTOR=DATA+0x100;WORLD=DATA+0x200;PLAYER=DATA+0x300;CONTACT=DATA+0xc00
ROTATION_FLOATS=(0x7dfff0,0x7dd568,0x7dd550,0x7e0070,0x7e0118,0x7dcc98,0x7dcb10,0x7e011c,0x7e0120,0x7dce68,0x7dcb2c,0x7e0164,0x7dccf4,0x7e0160,0x7e0170,0x7dcb60)
ROTATION_DOUBLES=(0x7e0168,0x7dccd0,0x7dcce8)
INITIALIZER=(0x7dcb2c,0x7dd584,0x7dd58c,0x7dd540,0x7e0078,0x7e0074,0x7e0070,0x7e006c,0x7e0068,0x7e0064,0x7dff54,0x7e0060,0x7dcc98)
RESPONSE=((0x7dcb2c,4),(0x7dcb10,4),(0x7dd570,4),(0x7e0080,4),(0x7dd58c,4),(0x7dcb0c,4),(0x7e6c30,4),(0x7dccd0,8),(0x7dccc8,8),(0x7e0b40,8))
def pack_constants(image):
    result=b''.join(image.read(a,4) for a in FLOATS)+b''.join(image.read(a,8) for a in DOUBLES)
    result+=b''.join(image.read(a,4) for a in ROTATION_FLOATS)+b''.join(image.read(a,8) for a in ROTATION_DOUBLES)+b''.join(image.read(a,4) for a in INITIALIZER)
    result+=b''.join(image.read(a,n) for a,n in RESPONSE)+b''.join(image.read(a,n) for a,n in ATTRIBUTES)+b''.join(image.read(a,n) for a,n in FLOOR)
    result+=image.read(0x7dcc98,4)+image.read(0x7dccd8,4)+image.read(0x7dcb0c,4)+struct.pack('<I',0x3f851eb8)+image.read(0x7dccd0,8)+image.read(0x7e05f8,8)
    result+=image.read(0x7dcb2c,4)+image.read(0x7dcb10,4)+image.read(0x7df1c8,4)+image.read(0x7e0600,4)+image.read(0x7e0028,4)
    if len(result)!=444:raise RuntimeError(f'Constant packing size{len(result)}')
    return result
class PositionOriginal(RegistryOriginal):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw,unit);self.scene_calls=collections.Counter();self.observe_scene=False
        entries={0x449500,0x445d10,0x453820,0x450c10,0x43cce0,0x43cf90,0x43d180,0x43d8b0,0x43d360,0x444640,0x444aa0,0x444d50,0x43dca0,0x43df60,0x43ca40,0x43cb40,0x444f10,0x43bdc0}
        def record(machine,address,size,user):
            if self.observe_scene and address in entries:self.scene_calls[hex(address)]+=1
        self.machine.hook_add(unicorn.UC_HOOK_CODE,record)
    def run_position(self,w,cw):
        m=self.machine
        # Producer uses the actual bounded source table view and all original
        # mesh/vertex/index/material bytes; its flags continue into positioning.
        self.run(4,[w[1],0,0,0,word(40),1,0],cw)
        m.mem_write(ACTOR,bytes(64));m.mem_write(WORLD,bytes(64));m.mem_write(PLAYER,bytes(0x220));m.mem_write(CONTACT,bytes(132))
        m.mem_write(ACTOR,bytes([w[17]&255]));m.mem_write(ACTOR+4,struct.pack('<H',w[8]&0xffff));m.mem_write(ACTOR+8,bytes([0,w[16]&255]));m.mem_write(ACTOR+0x14,struct.pack('<3I',*w[9:12]));m.mem_write(ACTOR+0x20,struct.pack('<3I',*w[2:5]))
        m.mem_write(WORLD+4,struct.pack('<3I',*w[5:8]));m.mem_write(WORLD+0x3c,struct.pack('<I',word(1)))
        m.mem_write(PLAYER+0x38,struct.pack('<3I',*w[5:8]));m.mem_write(PLAYER+0x10,struct.pack('<H',w[12]&0xffff));m.mem_write(PLAYER+0x60,struct.pack('<I',w[13]));m.mem_write(PLAYER+0x1e4,struct.pack('<I',w[14]));m.mem_write(PLAYER+0x1c,struct.pack('<I',w[15]));m.mem_write(PLAYER+0x50,struct.pack('<3I',0,word(1),0));m.mem_write(PLAYER+0x64,struct.pack('<I',CONTACT));m.mem_write(PLAYER+0x9c,self.image.read(0x9154e8+w[16]*132,132))
        for address,value in ((0x3b36dd0,WORLD),(0x3b3cdf0,PLAYER),(0x3b42e10,ACTOR),(0x3b42e74,0),(0x3b42e70,0),(0x3b2c578,0)):
            m.mem_write(address,struct.pack('<I',value))
        m.mem_write(0x3b0f0f8,struct.pack('<3I',0,word(-1),0));m.mem_write(0x3b0f0ec,b'\xff');m.mem_write(0x3b22dec,b'\0\0');m.mem_write(0x3b22dcc,b'\x01\0');m.mem_write(0x3b22dc0,struct.pack('<H',w[16]));m.mem_write(0x3b0e7a0,bytes(8))
        self.scene_calls.clear();self.observe_scene=True;self.invoke(0x445d10 if w[0] else 0x449500,cw,(ACTOR,WORLD,PLAYER),False);self.observe_scene=False
        values=[0,0,0,m.reg_read(UC_X86_REG_EAX),struct.unpack('<H',m.mem_read(ACTOR+4,2))[0]]
        values.extend(struct.unpack('<3I',m.mem_read(ACTOR+0x14,12)));values.extend(struct.unpack('<3I',m.mem_read(ACTOR+0x20,12)));values.extend(struct.unpack('<3I',m.mem_read(WORLD+4,12)))
        values.extend(struct.unpack('<I',m.mem_read(WORLD+a,4))[0] for a in (0x1c,0x24,0x20,0x3c));values.extend(struct.unpack('<3I',m.mem_read(PLAYER+0x38,12)));values.extend((struct.unpack('<I',m.mem_read(PLAYER+0x5c,4))[0],struct.unpack('<H',m.mem_read(PLAYER+0x10,2))[0]))
        values.extend(struct.unpack('<3I',m.mem_read(PLAYER+0x50,12)));values.extend(struct.unpack('<3I',m.mem_read(PLAYER+0x44,12)));values.extend(struct.unpack('<I',m.mem_read(PLAYER+a,4))[0] for a in (0x114,0xf4,0xd8,0x74,));values.extend(struct.unpack('<I',m.mem_read(a,4))[0] for a in (0x3b42e74,0x3b32724,0x3b42ff0,0x3b42ff4,0x3d0fa2c));values.extend((m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63));values.extend(struct.unpack('<33I',m.mem_read(CONTACT,132)));values.extend(struct.unpack('<128I',m.mem_read(MATRIX,512)));return values
def fixtures(image):
    # Actual first mesh polygon center after its source object transform,
    # recovered once with the real source registry as a fixture producer.
    oracle=PositionOriginal(image,0x27f,image.read(0x389d650,64));oracle.image=image;oracle.run(4,[1,0,0,0,word(40),1,0],0x27f)
    center=struct.unpack('<3f',oracle.machine.mem_read(0x3b48550+8,12));result=[]
    for variant in (0,1):
        for count in (0,1,4):
            for flags in (0,1,2,3):
                for dy in (0,5,15):
                    position=[center[0],center[1]+dy,center[2]]
                    result.append([variant,count,*map(word,position),word(0),word(-10),word(0),flags,0,0,0,flags,0,0,word(1),0,0])
    # Select additional actual generated horizontal polygons. This host-side
    # cross only chooses fixtures; both compared game paths still execute
    # their own original/native normal, query and contact instructions.
    horizontal=[];floor_count=1
    for floor_count in (1,4,16,64):
        if floor_count!=1:oracle.run(4,[floor_count,0,0,0,word(40),1,0],0x27f)
        free=struct.unpack('<I',oracle.machine.mem_read(0x3c48558,4))[0];node_count=(free-0x3b48550)//64
        for index in range(node_count):
            raw=struct.unpack('<9f',oracle.machine.mem_read(0x3b48550+index*64+24,36));a=raw[:3];b=raw[3:6];c=raw[6:]
            u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
            normal=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);squared=sum(value*value for value in normal)
            if squared and normal[1]*normal[1]>.81*squared:
                point=struct.unpack('<3f',oracle.machine.mem_read(0x3b48550+index*64+8,12))
                if point not in horizontal:horizontal.append(point)
                if len(horizontal)==4:break
        if horizontal:break
    if not horizontal:raise RuntimeError('Real source resource views have no horizontal triangle fixture')
    for point in horizontal:
        for variant in (0,1):
            for flags in (0,1):
                for dy in (0,3,5,8):
                    result.append([variant,floor_count,word(point[0]),word(point[1]+dy),word(point[2]),0,word(-10),0,flags,0,0,0,flags,0,0,word(1),6,0])
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--limit',type=int);p.add_argument('--floor-only',action='store_true');p.add_argument('--cw',type=lambda value:int(value,0));a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_scene_position;probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int;lib.sadx_native_player_scene_position_compiler.restype=ctypes.c_char_p
    data=(image.data,image.read(TABLE_ADDRESS,4100),image.read(0x389d650,64),pack_constants(image),image.read(0x9154e8,8*132));buffers=[(ctypes.c_uint8*len(v)).from_buffer_copy(v) for v in data]
    cases=fixtures(image);cases=cases[72:] if a.floor_only else cases;cases=cases[:a.limit] if a.limit else cases;blob=b''.join(struct.pack('<18I',*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];paths=['src/player/player_scene_position.cpp','src/player/player_scene_position.hpp','src/player/player_scene_position_helpers.cpp','tests/probes/native_player_scene_position_probe.cpp']
    report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_scene_position_compiler().decode(),sources={path:sha((root/path).read_bytes()) for path in paths},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),case_sha256=sha(blob),profiles=[],mismatches=[],limits=['Actual source registry plus449500/445d10; no code overlays','Zero-filled caller work, real ECoast arrays and bounded landtable count views are synthetic profiles','Platform callback/render/input selectors are explicit controlled facts, not live Windows observations','Unicorn flags/transcendental limits remain; full hardware/mod behavior is unverified'])
    oracle=PositionOriginal(image,0x27f,data[2]);oracle.image=image
    control_words=[a.cw] if a.cw is not None else [pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)]
    for cw in control_words:
        item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0,boundary_failures=0,source_calls={},source_contact_cases=0,source_outcomes={})
        calls=collections.Counter();outcomes=collections.Counter()
        for index,w in enumerate(cases):
            original=oracle.run_position(w,cw);calls.update(oracle.scene_calls);outcomes[original[3]]+=1
            if original[50]&0xffff:item['source_contact_cases']+=1
            out=(ctypes.c_uint32*201)();status=probe(buffers[0],len(data[0]),*buffers[1:],(ctypes.c_uint32*18)(*w),cw,out)
            if status:item['boundary_failures']+=1;report['mismatches'].append(dict(cw=hex(cw),case=index,input=w,status=status,boundary=list(out[:3])));continue
            differences=[i for i in range(201) if i!=39 and original[i]!=out[i]]
            if differences:item['value_mismatches']+=1;report['mismatches'].append(dict(cw=hex(cw),case=index,input=w,differences=[dict(word=i,original=hex(original[i]),native=hex(out[i])) for i in differences[:32]]))
            if original[39]!=out[39]:item['flag_mismatches']+=1
        item['source_calls']=dict(calls);item['source_outcomes']=dict(outcomes);report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['values_cw_frames_match']=all(not x['value_mismatches'] and not x['boundary_failures'] for x in report['profiles']);report['strict_match']=report['values_cw_frames_match'] and all(not x['flag_mismatches'] for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_cw_frames_match'] else 1
if __name__=='__main__':raise SystemExit(main())
