"""Actual450c10 +43acd0 with explicitly synthetic native-owned linked bounds.

These resource seeds exercise traversal; they do not establish loaded geometry.
The real ECoast decoder/registration profile is retained separately.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_player_scene import SceneOriginal,DATA,MATRIX

CONTACT=DATA+0x400;OBJECT=DATA+0x700;GROUP0=DATA+0x1000;GROUP1=DATA+0x1100;NODES=DATA+0x5000;QUERY=0x3b47f50
NULL=0xffffffff
def sha(v):return hashlib.sha256(v).hexdigest()
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def fixtures():
    rng=random.Random(0x450c63);result=[]
    for i in range(240):
        count=rng.randrange(0,161);zs=sorted(rng.uniform(-20,20) for _ in range(count));head=0 if count else NULL;tail=count-1 if count else NULL
        if i%9==0:head=NULL
        if i%11==0:tail=NULL
        if count and i%7==0:head=count//2
        center=[rng.uniform(-2,2),rng.uniform(-2,2),rng.uniform(-30,30)]
        w=[count,head,tail,0 if i%13==0 else 1,word(rng.choice((0.,1.,5.))),*map(word,center),word(rng.choice((0.,1.,5.,20.))),*map(word,(rng.uniform(-3,3) for _ in range(3))),word(1000.)]
        for n,z in enumerate(zs):w.extend([n+1 if n+1<count else NULL,n-1 if n else NULL,word(rng.uniform(-2,2)),word(rng.uniform(-2,2)),word(z),word(rng.choice((0.,1.,5.,100.)))])
        result.append(w)
    for count in (1,2,127,128,129,160):
        w=[count,0,count-1,1,word(0.),word(0.),word(0.),word(0.),word(1.),word(0.),word(0.),word(0.),word(1000.)]
        for n in range(count):w.extend([n+1 if n+1<count else NULL,n-1 if n else NULL,word(0.),word(0.),word(0.),word(100.)])
        result.append(w)
    for z in (-1.,0.,1.):
        w=[1,0,0,1,word(0.),word(0.),word(0.),word(0.),word(1.),word(0.),word(0.),word(0.),word(1000.),NULL,NULL,word(0.),word(0.),word(z),word(100.)];result.append(w)
    for i,w in enumerate(result):w[13:13]=[i%3,0x10000001 if i%2 else 1,*[rng.choice((0,0x4000,0x8000,0xc000,rng.randrange(2**32))) for _ in range(3)]]
    return result
class BoundsOriginal(SceneOriginal):
    def run_bounds(self,w,cw):
        m=self.machine;m.mem_write(MATRIX,self.unit*3)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,3),(0x3d0fa2c,1),(0x3d0f7ac,0),(0x3b36d38,1 if w[13] else 0),(0x915460,1),(0x3b2f718,DATA+0x900),(0x3c48550,GROUP0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(0x3b32728,struct.pack('<3I',w[14],OBJECT,0));m.mem_write(OBJECT+4,struct.pack('<I',DATA+0x800));m.mem_write(OBJECT+8,struct.pack('<3f',4,5,6));m.mem_write(OBJECT+0x14,struct.pack('<3I',*w[15:18]));m.mem_write(DATA+0x800+0x18,struct.pack('<4f',1,2,3,1000))
        m.mem_write(0x3b36d3c,struct.pack('<h',1));m.mem_write(0x3b2e518,struct.pack('<9I',0,0,0,w[12],0,0,OBJECT,0,1))
        nodeptr=lambda n:0 if n==NULL else NODES+n*32
        for address,next_group,object_pointer,head,tail,padding in ((GROUP0,GROUP1,OBJECT if w[3] else OBJECT+4,nodeptr(w[1]),nodeptr(w[2]),w[4]),(GROUP1,0,OBJECT+8,0,0,0)):
            m.mem_write(address,struct.pack('<11I',next_group,0,object_pointer,0,head,tail,0,QUERY+17*4,0,44,padding))
        for n in range(w[0]):
            values=w[18+n*6:24+n*6];m.mem_write(NODES+n*32,struct.pack('<6I',nodeptr(values[0]),nodeptr(values[1]),*values[2:]))
        m.mem_write(CONTACT,struct.pack('<I',w[8]));m.mem_write(CONTACT+4,struct.pack('<3I',*w[5:8]));m.mem_write(CONTACT+0x10,struct.pack('<3I',*w[9:12]));self.invoke(0x450c10,cw,(CONTACT,))
        count=struct.unpack('<I',m.mem_read(0x3b42ff0,4))[0];header=[count]
        for g in (GROUP0,GROUP1):
            start=struct.unpack('<I',m.mem_read(g+0x1c,4))[0];header.extend([(start-QUERY)//4 if start else NULL,struct.unpack('<I',m.mem_read(g+0x24,4))[0]])
        from unicorn.x86_const import UC_X86_REG_FPSW,UC_X86_REG_FPCW
        header.extend([m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW),struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0]])
        selected=[(v-NODES)//32 for v in struct.unpack('<'+'I'*count,m.mem_read(QUERY,count*4))]
        return header,selected,list(struct.unpack('<48I',m.mem_read(MATRIX,192)))
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cases=fixtures();lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_scene_mesh_bounds;probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    lib.sadx_native_player_scene_compiler.restype=ctypes.c_char_p
    table=(ctypes.c_uint8*4100).from_buffer_copy(image.read(TABLE_ADDRESS,4100));unit=image.read(0x389d650,64);ub=(ctypes.c_uint8*64).from_buffer_copy(unit);cb=(ctypes.c_uint32*2)(image.unpack('<I',0x7df1c8)[0],image.unpack('<I',0x7e0600)[0])
    blob=b''.join(struct.pack('<I',len(w))+struct.pack('<'+'I'*len(w),*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];paths=['src/player/player_scene_resources.hpp','src/player/player_scene_broadphase.hpp','src/player/player_scene_broadphase.cpp','src/player/player_coordinates.hpp','src/player/player_coordinates.cpp','src/math/ninja_math.hpp','src/math/ninja_math.cpp','src/cpu/x87_math.hpp','src/cpu/x87_math.cpp','tests/probes/native_player_scene_probe.cpp']
    report=dict(profile='synthetic linked bounds; actual450c10/43acd0; no overlays',source_sha256=image.sha256,source_functions={hex(addr):sha(image.read(addr,end-addr)) for addr,end in ((0x450c10,0x451026),(0x43acd0,0x43aee8))},library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_scene_compiler().decode(),native_sources={x:sha((root/x).read_bytes()) for x in paths},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),fixture_sha256=sha(blob),fixture_count=len(cases),control_words=[],mismatches=[],limits=['Synthetic native/source resource graphs, not loaded level geometry','Actual forward/backward walks, selected/unselected identity, range edges and fixed128cap','Native/source group partial stores, selected order, matrix frames and CW compared','No original hardware or physicalgameplay observation; Unicornflagslimited'])
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        original=BoundsOriginal(image,cw,unit);item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0)
        for index,w in enumerate(cases):
            header,selected,frames=original.run_bounds(w,cw);out=(ctypes.c_uint32*186)();status=probe(table,ub,cb,(ctypes.c_uint32*len(w))(*w),len(w),cw,out)
            if status or out[0]:raise RuntimeError(f'Native bounds failure{status}/{out[0]} at{out[1]:#x}')
            actual_header=list(out[2:10]);actual_selected=list(out[10:10+out[2]]);actual_frames=list(out[138:186]);differences=[]
            if header[:5]!=actual_header[:5] or header[6:]!=actual_header[6:]:differences.append('header')
            if selected!=actual_selected:differences.append('selected')
            if frames!=actual_frames:differences.append('frames')
            if differences:item['value_mismatches']+=1;report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),original=dict(header=header,selected=selected,frames=frames),native=dict(header=actual_header,selected=actual_selected,frames=actual_frames)))
            if header[5]!=actual_header[5]:item['flag_mismatches']+=1
        report['control_words'].append(item);print(json.dumps(item),flush=True)
    report['values_match']=all(not x['value_mismatches'] for x in report['control_words']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_match'] else 1
if __name__=='__main__':raise SystemExit(main())
