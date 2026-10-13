"""Exact77e390 numeric/vertex instruction slices, not full sprite rendering.

Starts after texture binding at77e3c9. Executes real781670 and CRT_ftol.
Stops before GPU state calls; vertex stores are resumed with explicit seeded
unrounded depth, excluding the unlifted D3DX/texture/queue/renderer owners.
"""
import argparse,ctypes,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP,DATA,CODE
from verify_original_task_work import sha
SPRITE=DATA+0x100;FRAME=DATA+0x200;MATRIX=DATA+0x1000;VERTICES=DATA+0x300;DEPTH=DATA+0x500;LOCAL=STACK-0x400
CONST=(0x7db2c4,0x7dce6c,0x7e9b40,0x7dcb2c,0x7dcb10)
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures(unit):
    rng=random.Random(0x77e390);cases=[]
    def add(name,flags=0,**changes):
        w=[flags,*map(word,(3,4,5,2,3)),0x1234,31,*map(word,(100,10,20,1,.5,.25,.125,.25))]
        for key,value in changes.items():w[int(key[1:])]=value
        cases.append(dict(name=name,words=w,matrix=list(unit)))
    for flags in range(64):add('source flags / vertex topology',flags)
    for z in(0,-1):add('projected clip guard',0x31,w3=word(z))
    for bit in(1,2,4,8):add('required projection boundary',0x31,w7=31^bit)
    add('required source tint boundary',2,w7=15)
    for _ in range(96):
        add('finite source matrix/scale/color fixture',rng.randrange(64),w1=word(rng.uniform(-100,100)),w2=word(rng.uniform(-100,100)),w3=word(rng.uniform(.01,100)),w4=word(rng.uniform(-8,8)),w5=word(rng.uniform(-8,8)))
        c=cases[-1];c['matrix'][12]=word(rng.uniform(-2,2));c['matrix'][13]=word(rng.uniform(-2,2));c['matrix'][14]=word(rng.uniform(-.001,.001))
    for raw in(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345):
        for field in(1,3,4,8,11,15):add('source nonfinite/subnormal perturbed input',0x33,**{'w'+str(field):raw})
    return cases
class SpriteOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x7f);self.machine.mem_map(0x3d0f000,0x1000);self.machine.mem_map(0x3d12000,0x1000);self.machine.mem_write(FRAME,image.read(0x9279d8,20));self.machine.mem_write(0x3d0f818,struct.pack('<I',MATRIX));self.target=0;self.stopped=0
        # Test-only FLD seed restores an explicitly provided external depth,
        # then enters actual vertex-store instructions. It is not D3DX output.
        self.machine.mem_write(CODE,b'\xd9\x05'+struct.pack('<I',DEPTH)+b'\xe9'+struct.pack('<i',0x77e63c-(CODE+11)))
        def code(machine,address,size,user):
            if address in(self.target,0x77e5c2,0x77e80f,0x77e6db,0x77e7d7):self.stopped=address;machine.emu_stop()
        self.machine.hook_add(unicorn.UC_HOOK_CODE,code)
    def run(self,c,cw):
        w=c['words'];m=self.machine;m.mem_write(LOCAL,bytes(0x100));m.mem_write(VERTICES,bytes(112));m.mem_write(SPRITE,struct.pack('<6I',*w[1:7]));m.mem_write(MATRIX,struct.pack('<16I',*c['matrix']));m.mem_write(0x3d0f9e0,struct.pack('<I',w[8]));m.mem_write(0x3d0fa0c,struct.pack('<2I',*w[9:11]));m.mem_write(0x3d0f7f0,struct.pack('<4I',*w[11:15]));m.mem_write(LOCAL+0x48,struct.pack('<I',w[0]));m.mem_write(0x3d128ac,struct.pack('<I',VERTICES));m.mem_write(DEPTH,struct.pack('<I',w[15]))
        self.target=0;self.stopped=0
        if w[0]&16:
            for bit,site in((1,0x77e42a),(2,0x77e444),(4,0x77e454),(8,0x77e464)):
                if not(w[7]&bit):self.target=site;break
        if not self.target and(w[0]&2)and not(w[7]&16):self.target=0x77e53b
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_EDI,SPRITE);m.reg_write(UC_X86_REG_ESI,FRAME);m.reg_write(UC_X86_REG_ESP,LOCAL)
        m.emu_start(0x77e3c9,STOP,count=20000);stop=self.stopped
        mask=15|(0 if stop==0x77e42a else 7<<8)
        if stop in(0x77e5c2,0x77e53b):mask|=15<<4
        offsets=(0x10,0x40,0x14,0x18,0x20,0x1c,0x24,0x28,0x30,0x34,0x38)
        values=[struct.unpack('<I',m.mem_read(LOCAL+x,4))[0]for x in offsets]
        color=m.reg_read(UC_X86_REG_EDI)if stop==0x77e5c2 else None
        if stop==0x77e5c2:
            self.target=0;self.stopped=0;m.reg_write(UC_X86_REG_EBX,w[0]);m.reg_write(UC_X86_REG_EDI,color);m.reg_write(UC_X86_REG_ESP,LOCAL)
            if w[0]&16:
                m.reg_write(UC_X86_REG_ESI,VERTICES);m.emu_start(CODE,STOP,count=1000)
            else:m.emu_start(0x77e733,STOP,count=1000)
        return dict(boundary=0 if stop in(0x77e5c2,0x77e80f)else stop,mask=mask,values=values,color=color,clipped=stop==0x77e80f,vertices=bytes(m.mem_read(VERTICES,112)),flags=m.reg_read(UC_X86_REG_FPSW)&63,cw=m.reg_read(UC_X86_REG_FPCW))
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Wrong source/oracle version')
    constants=image.read(0x9279d8,20)+b''.join(image.read(x,4)for x in CONST);table=image.read(0x88af78,4100);unit=image.read(0x389d650,64);cases=fixtures(struct.unpack('<16I',unit));blob=b''.join(struct.pack('<32I',*c['words'],*c['matrix'])for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_sprite;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_float),ctypes.c_size_t,ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=SpriteOriginal(image);root=pathlib.Path(__file__).resolve().parent.parent
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),constants_sha256=sha(constants),case_blob=dict(path=str(a.output.with_suffix('.cases.bin')),sha256=sha(blob),cases=len(cases),record='16 input words +16 exact float-matrix words LE'),native_sources={n:sha((root/n).read_bytes())for n in('src/ninja/ninja_sprite.cpp','src/ninja/ninja_sprite.hpp','tests/probes/native_ninja_sprite_probe.cpp','tools/oracles/verify_original_ninja_sprite.py')},source_ranges={hex(x):dict(bytes=y-x,sha256=sha(image.read(x,y-x)))for x,y in((0x77e390,0x77e817),(0x781670,0x7816d9),(0x644340,0x6443bf))},profiles=[],limits=['Actual instruction slices77e3c9..77e5c2/77e80f and vertex stores; no full source texture/queue/renderer body claim','Matrix ownership/data and projection/color are explicit source-bound fixtures, not a captured game camera','Vertex depth is explicitly seeded FLD32 external input, not an implemented D3DX project approximation','Actual781670/CRT_ftol instructions execute unchanged; no callee overlays','Required unknown native facts stop before their source instruction and retain local stores','All status/nonfinite discrepancies retained, including known Unicorn FLD SNaN limits','No GPU rendering, resource loading or full game/mod/hardware parity claim'])
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300)for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,vertex_mismatches=0,boundary_mismatches=0,control_mismatches=0,flag_mismatches=0,examples=[])
        for index,c in enumerate(cases):
            expected=source.run(c,cw);out=(ctypes.c_uint32*32)();vertices=(ctypes.c_ubyte*112)();matrix=(ctypes.c_float*16).from_buffer_copy(struct.pack('<16I',*c['matrix']));status=fn((ctypes.c_ubyte*40).from_buffer_copy(constants),40,(ctypes.c_float*1025).from_buffer_copy(table),1025,(ctypes.c_float*16).from_buffer_copy(unit),matrix,(ctypes.c_uint32*16)(*c['words']),16,cw,out,32,vertices,112)
            if status:raise RuntimeError(f'Native rejectedcase{index}: {status}')
            source_values=[expected['values'][i]for i in range(11)if expected['mask']&(1<<i)];native_values=[out[16+i]for i in range(11)if expected['mask']&(1<<i)];different=[]
            for field,x,y in(('value',[expected['mask'],expected['color']is not None,expected['color']or 0,expected['clipped'],source_values],[out[4],bool(out[5]),out[6],bool(out[7]),native_values]),('vertex',expected['vertices'],bytes(vertices)),('boundary',expected['boundary'],out[1]),('control',expected['cw'],out[3]),('flag',expected['flags'],out[2])):
                if x!=y:profile[field+'_mismatches']+=1;different.append(field)
            if any(d!='flag'for d in different)or(different and len(profile['examples'])<5):profile['examples'].append(dict(case=index,name=c['name'],fields=different,native=list(out),source_values=list(map(hex,source_values)),native_values=list(map(hex,native_values)),source_boundary=hex(expected['boundary']),source_flags=hex(expected['flags']),vertex_word_examples=[dict(word=i,source=hex(struct.unpack_from('<I',expected['vertices'],i*4)[0]),native=hex(struct.unpack_from('<I',bytes(vertices),i*4)[0]))for i in range(28)if expected['vertices'][i*4:i*4+4]!=bytes(vertices)[i*4:i*4+4]]))
        report['profiles'].append(profile);print(json.dumps({k:v for k,v in profile.items()if k!='examples'}),flush=True)
    report['supported_slice_values_vertices_boundaries_control_match']=all(not any(x[k]for k in('value_mismatches','vertex_mismatches','boundary_mismatches','control_mismatches'))for x in report['profiles']);report['strict_emulator_match']=report['supported_slice_values_vertices_boundaries_control_match']and all(not x['flag_mismatches']for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return int(not report['supported_slice_values_vertices_boundaries_control_match'])
if __name__=='__main__':raise SystemExit(main())
