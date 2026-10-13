"""Actual mesh normal/normalize/determinant/inverse instructions, no overlays."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import native_source,Original,CODE,DATA,STACK,STOP

VECTOR=DATA+0x100;OUTPUT=DATA+0x300
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
class MeshOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x27f);self.machine.mem_map(0x2bcc000,0x1000);self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000));self.machine.mem_map(0x3c83000,0x1000);self.machine.mem_write(0x3c837a8,struct.pack('<I',1))
        self.machine.mem_map(0x3d0f000,0x1000);self.machine.mem_write(0x3d0f818,struct.pack('<I',VECTOR))
        self.machine.mem_write(CODE,b'\xdb\x3d'+struct.pack('<I',OUTPUT)+b'\xc3')
    def run(self,mode,w,cw):
        m=self.machine;m.mem_write(VECTOR,struct.pack('<19I',*w));m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        if mode==0:entry=0x450810;args=(STOP,VECTOR,VECTOR+12,VECTOR+24,OUTPUT)
        elif mode in (1,2):entry=0x450920 if mode==1 else 0x4509d0;args=(STOP,VECTOR)
        elif mode==3:entry=0x787630;args=(STOP,);m.reg_write(UC_X86_REG_ECX,VECTOR)
        elif mode==4:entry=0x795000;args=(CODE,STOP);m.reg_write(UC_X86_REG_ECX,VECTOR)
        else:entry=0x788a50;args=(STOP,);m.reg_write(UC_X86_REG_ECX,VECTOR);m.reg_write(UC_X86_REG_EDX,VECTOR+64)
        m.mem_write(STACK,struct.pack('<'+'I'*len(args),*args));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(entry,STOP,count=5000)
        values=list(struct.unpack('<3I',m.mem_read(OUTPUT if mode==0 else VECTOR,12))) if mode<3 else list(struct.unpack('<16I',m.mem_read(VECTOR,64)))
        if mode==4:
            sig,se=struct.unpack('<QH',m.mem_read(OUTPUT,10));values=[se,sig&0xffffffff,sig>>32]
        return values,m.reg_read(UC_X86_REG_EAX) if mode==3 else 0,m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW)
def fixtures(image):
    rng=random.Random(0x787630);result={m:[] for m in range(6)}
    for i in range(400):
        result[0].append([*map(word,(rng.uniform(-100,100) for _ in range(9))),*([0]*10)])
        for m in (1,2):result[m].append([*map(word,(rng.uniform(-100,100) for _ in range(3))),*([0]*16)])
        matrix=[word(rng.uniform(-4,4)) for _ in range(16)];scales=[word(rng.uniform(-4,4)) for _ in range(3)]
        for m in (3,4,5):result[m].append(matrix+scales)
    raw=(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff812345)
    for m in range(6):
        slots=9 if m==0 else 3 if m in (1,2) else 19 if m==5 else 16
        for v in raw:
            for slot in range(slots):
                w=[word(1 if k%5==0 else 0) for k in range(16)]+[word(1)]*3;w[slot]=v;result[m].append(w)
    # Source-local triangle inputs from actual ECoast vertex/index resources.
    land=image.unpack('<I',0xe99cb8+12)[0]
    for i in range(80):
        obj=image.unpack('<I',land+i*36+24)[0];model=image.unpack('<I',obj+4)[0]
        vertices,_,count,sets=image.unpack('<4I',model);type_,meshes,indices=image.unpack('<HHI',sets)
        if type_&0xc000!=0xc000 or not meshes:continue
        header=image.unpack('<H',indices)[0]
        if header&0x7fff<3:continue
        ids=list(image.unpack('<3H',indices+2))
        if header&0x8000:ids[1],ids[2]=ids[2],ids[1]
        if any(v>=count for v in ids):raise RuntimeError('Real vertex index outside model')
        w=[]
        for vertex in ids:w.extend(image.unpack('<3I',vertices+vertex*12))
        result[0].append(w+[0]*10)
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cases=fixtures(image);lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_mesh_math
    probe.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    lib.sadx_native_player_mesh_math_compiler.restype=ctypes.c_char_p
    axis=(ctypes.c_uint32*2)(image.unpack('<I',0x7dcb2c)[0],image.unpack('<I',0x7dcb10)[0]);blob=b''.join(struct.pack('<20I',m,*w) for m in range(6) for w in cases[m]);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];names=('player_mesh_math','player_mesh_matrix','crt_sqrt','crt_asin','crt_argument','ninja_math','x87_math')
    report=dict(source_sha256=image.sha256,profile='actual450810/450920/4509d0/795000/787630/788a50; no overlays; explicitmatherr1',native_library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_mesh_math_compiler().decode(),native_sources={f'src/{n}.{e}':sha((native_source(root,f'src/{n}.{e}')).read_bytes()) for n in names for e in ('hpp','cpp')},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),case_sha256=sha(blob),profiles=[],mismatches=[],limits=['Masked experiment CWs/matherr1 not observed runtime','80realECoast localtriangle inputs plus syntheticraw/matrix fixtures; not finalcontactquery','Generated matrix arithmetic compiles to directnativeC++calls; no runtimeopcodeinterpreter','Unicorn signaling-load/transcendental/status limitations remain; no hardwareequivalenceclaim'])
    oracle=MeshOriginal(image)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        for mode in range(6):
            item=dict(cw=hex(cw),mode=mode,cases=len(cases[mode]),value_mismatches=0,return_mismatches=0,flag_mismatches=0,cw_mismatches=0)
            for index,w in enumerate(cases[mode]):
                source,ret,flags,control=oracle.run(mode,w,cw);out=(ctypes.c_uint32*20)();status=probe(mode,(ctypes.c_uint32*19)(*w),axis,cw,out)
                if status:raise RuntimeError(f'Native mode{mode}/case{index} failed{status}/{out[19]}')
                actual=list(out[:len(source)]);differences=[]
                for kind,x,y in (('value',source,actual),('return',ret,out[16]),('flag',flags,out[17]),('cw',control,out[18])):
                    if x!=y:item[kind+'_mismatches']+=1;differences.append(kind)
                if any(k!='flag' for k in differences):report['mismatches'].append(dict(cw=hex(cw),mode=mode,case=index,fields=differences,input=list(map(hex,w)),source=list(map(hex,source)),native=list(map(hex,actual)),returns=[ret,out[16]],flags=[flags,out[17]],control=[control,out[18]]))
            report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['values_returns_cw_match']=all(not x['value_mismatches'] and not x['return_mismatches'] and not x['cw_mismatches'] for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_returns_cw_match'] else 1
if __name__=='__main__':raise SystemExit(main())
