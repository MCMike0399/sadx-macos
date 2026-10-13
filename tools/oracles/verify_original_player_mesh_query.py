"""Actual453820 with explicit selected real/local and synthetic triangle seeds."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import native_source,Original,DATA,STACK,STOP
from verify_original_floor_angles import CONSTANTS as FLOOR_CONSTANTS

CONTACT=DATA+0x100;GROUP=DATA+0x300;OBJECT=DATA+0x400;NODES=DATA+0x1000;MATRIX=DATA+0x8000
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
class QueryOriginal(Original):
    def __init__(self,image,unit):
        super().__init__(image,0x27f);self.unit=unit
        for page in (0x2bcc000,0x3c83000,0x3d0f000,0x3c48000,0x3b42000,0x3b47000,0x389d000):self.machine.mem_map(page,0x1000)
        self.machine.mem_write(0x2bcc000,image.read(0x2bcc000,0x1000));self.machine.mem_write(0x389d650,unit);self.machine.mem_write(0x3c837a8,struct.pack('<I',1));self.machine.mem_write(0x2bcc6f0,struct.pack('<I',1))
    def run(self,w,cw):
        m=self.machine;contact=bytearray(132);struct.pack_into('<7I',contact,0,*w[:7]);struct.pack_into('<3f',contact,0x1c,11,12,13);struct.pack_into('<2H',contact,0x28,23,24);struct.pack_into('<2I',contact,0x34,0x11111111,0x22222222)
        for offset,values in ((0x3c,(101,102,103)),(0x48,(104,105,106)),(0x54,(107,108,109)),(0x60,(110,111,112)),(0x6c,(113,114,115)),(0x78,(116,117,118))):struct.pack_into('<3f',contact,offset,*values)
        m.mem_write(CONTACT,bytes(contact));m.mem_write(GROUP,bytes(64));m.mem_write(GROUP+8,struct.pack('<I',OBJECT));m.mem_write(GROUP+0x1c,struct.pack('<I',0x3b47f50));m.mem_write(GROUP+0x24,struct.pack('<I',w[7]));m.mem_write(0x3c48550,struct.pack('<I',GROUP));m.mem_write(0x3b42ff4,struct.pack('<I',17));m.mem_write(0x3c837d4,struct.pack('<I',0x12345678))
        m.mem_write(0x3b47590,b'\xa5'*0x9c*16);m.mem_write(0x3b47008,b'\xb6'*88*16)
        for n in range(w[7]):
            node=bytearray(64);struct.pack_into('<2I',node,0,NODES+(n+1)*64 if n+1<w[7] else 0,NODES+(n-1)*64 if n else 0);struct.pack_into('<10I',node,0x18,*w[8+n*10:18+n*10]);m.mem_write(NODES+n*64,bytes(node));m.mem_write(0x3b47f50+n*4,struct.pack('<I',NODES+n*64))
        m.mem_write(MATRIX,self.unit*4)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,4),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        m.mem_write(STACK,struct.pack('<4I',STOP,CONTACT,OBJECT,1));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x453820,STOP,count=300000)
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Query source did not return/ABI mismatch')
        count=struct.unpack('<I',m.mem_read(0x3b42ff4,4))[0];header=[m.reg_read(UC_X86_REG_EAX),count,*struct.unpack('<2H',m.mem_read(CONTACT+0x28,4)),*struct.unpack('<2I',m.mem_read(CONTACT+0x34,8)),*struct.unpack('<18I',m.mem_read(CONTACT+0x3c,72)),m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_FPCW),struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0]]
        # Six contiguous source output vectors fill3c..83, exactly18words.
        records=[]
        if count<=16:
            for n in range(count):records.extend(struct.unpack('<22I',m.mem_read(0x3b47590+n*0x9c+0x44,88)))
        return header,records,list(struct.unpack('<64I',m.mem_read(MATRIX,256)))
def fixtures(image):
    cases=[];rng=random.Random(0x453820)
    flat=[*map(word,(0,0,0,20,0,0,0,0,20)),0]
    for center in ((1,1,1),(1,-1,1),(50,1,50),(1,20,1)):
        for displacement in ((0,-2,0),(0,0,0),(1,-30,1),(1,2,1)):
            for radius in (0.,-1.,1.,5.):cases.append([word(radius),*map(word,center),*map(word,displacement),1,*flat])
    for count in (0,1,2,15,16,17,20):cases.append([word(2),*map(word,(1,1,1,0,-2,0)),count,*flat*count])
    for n in range(180):
        vertices=[(rng.uniform(-20,20),rng.uniform(-5,5),rng.uniform(-20,20)) for _ in range(3)]
        center=tuple(sum(p[d] for p in vertices)/3 for d in range(3));center=(center[0]+rng.uniform(-3,3),center[1]+rng.uniform(-3,3),center[2]+rng.uniform(-3,3))
        velocity=tuple(rng.uniform(-10,10) for _ in range(3));flags=0x800000 if n%2 else 0
        cases.append([word(rng.uniform(.5,5)),*map(word,center),*map(word,velocity),1,*[word(v) for p in vertices for v in p],flags])
    land=image.unpack('<I',0xe99cb8+12)[0]
    for i in range(80):
        obj=image.unpack('<I',land+i*36+24)[0];model=image.unpack('<I',obj+4)[0];vertices,_,nv,sets,materials=image.unpack('<5I',model);typ,meshes,indices=image.unpack('<HHI',sets)
        if not meshes:continue
        head=image.unpack('<H',indices)[0];ids=list(image.unpack('<3H',indices+2))
        if head&0x8000:ids[1],ids[2]=ids[2],ids[1]
        triangle=[];coords=[]
        for vertex in ids:triangle.extend(image.unpack('<3I',vertices+vertex*12));coords.append(image.unpack('<3f',vertices+vertex*12))
        flag=image.unpack('<I',materials+(typ&0x3fff)*20+16)[0] if materials else 0
        center=tuple(sum(p[d] for p in coords)/3 for d in range(3));center=(center[0],center[1]+rng.choice((-2.,0.,2.)),center[2]);velocity=(0.,rng.choice((-5.,0.,5.)),0.)
        cases.append([word(3),*map(word,center),*map(word,velocity),1,*triangle,flag])
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cases=fixtures(image);unit=image.read(0x389d650,64);lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_mesh_query
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int;lib.sadx_native_player_mesh_query_compiler.restype=ctypes.c_char_p
    tb=(ctypes.c_uint8*4100).from_buffer_copy(image.read(TABLE_ADDRESS,4100));ub=(ctypes.c_uint8*64).from_buffer_copy(unit);axis=(ctypes.c_uint32*2)(image.unpack('<I',0x7dcb2c)[0],image.unpack('<I',0x7dcb10)[0]);floor=(ctypes.c_uint8*36).from_buffer_copy(b''.join(image.read(addr,n) for addr,n in FLOOR_CONSTANTS));query=(ctypes.c_uint8*32).from_buffer_copy(image.read(0x7dcc98,4)+image.read(0x7dccd8,4)+image.read(0x7dcb0c,4)+struct.pack('<I',0x3f851eb8)+image.read(0x7dccd0,8)+image.read(0x7e05f8,8))
    blob=b''.join(struct.pack('<I',len(w))+struct.pack('<'+'I'*len(w),*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parents[2];names=('player_mesh_query','player_mesh_math','player_mesh_matrix','player_coordinates','ninja_matrix','floor_angles','crt_atan2','crt_fabs','crt_asin','crt_sqrt','crt_argument','ninja_math','x87_math');report=dict(profile='actual453820,explicitselected real/local+synthetictriangles; nooverlays',source_sha256=image.sha256,source_function_sha256=sha(image.read(0x453820,0x4546d9-0x453820)),native_library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_mesh_query_compiler().decode(),native_sources={f'src/{n}.{e}':sha((native_source(root,f'src/{n}.{e}')).read_bytes()) for n in names for e in ('hpp','cpp')},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),fixture_sha256=sha(blob),fixtures=len(cases),profiles=[],mismatches=[],limits=['Selectedtriangle/resource pointers explicitly seeded; fullregistryproduceractivation notprovedbythisprofile','80realECoast localtriangles+syntheticflat/countcapcases','CRTpoliciesexplicit1,notobservedruntime','SourceFPATANextended/flagsoraclelimitations;hardwareunknown'])
    original=QueryOriginal(image,unit)
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0)
        for index,w in enumerate(cases):
            header,records,frames=original.run(w,cw);out=(ctypes.c_uint32*445)();status=probe(tb,ub,axis,floor,query,(ctypes.c_uint32*len(w))(*w),len(w),cw,out)
            if status or out[0]:raise RuntimeError(f'Native querycase{index} failed{status}/{out[0]} at{out[1]:#x}')
            native_header=list(out[2:29]);native_records=list(out[29:29+min(out[3],16)*22]);native_frames=list(out[381:445]);differences=[]
            if header[:24]!=native_header[:24] or header[25:]!=native_header[25:]:differences.append('header')
            if header[0] and records!=native_records:differences.append('records')
            if frames!=native_frames:differences.append('frames')
            if header[24]!=native_header[24]:item['flag_mismatches']+=1
            if differences:item['value_mismatches']+=1;report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),source=dict(header=list(map(hex,header)),records=list(map(hex,records)),frames=list(map(hex,frames))),native=dict(header=list(map(hex,native_header)),records=list(map(hex,native_records)),frames=list(map(hex,native_frames)))))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['values_match']=all(not x['value_mismatches'] for x in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_match'] else 1
if __name__=='__main__':raise SystemExit(main())
