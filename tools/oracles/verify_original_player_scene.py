"""Real ECoast LANDTABLE binding-prefix/registration/query; no overlays.

Known-empty dynamic-task and registered mesh-group selectors are explicit test
inputs, not claims about a running stage. Real static records remain intact.
"""
import argparse,collections,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_math import native_source,Original,DATA,STACK,STOP

CAMERA=DATA+0x100;FIRST=DATA+0x200;SECOND=DATA+0x300;CONTACT=DATA+0x400;MATRIX=DATA+0x4000
def sha(v):return hashlib.sha256(v).hexdigest()
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class SceneOriginal(Original):
    def __init__(self,image,cw,unit):
        super().__init__(image,cw);self.unit=unit
        for section in image.sections:
            if section['name'] not in ('.text','.rdata'):
                self.machine.mem_map(image.base+section['rva'],(max(section['raw_size'],section['virtual_size'])+4095)&~4095)
                if section['raw_size']:self.machine.mem_write(image.base+section['rva'],image.slice(section['file_offset'],section['raw_size']))
        self.binding=None
        self.recent=collections.deque(maxlen=24)
        self.machine.hook_add(unicorn.UC_HOOK_CODE,lambda m,address,size,user:self.recent.append(hex(address)))
    def invoke(self,entry,cw,args=(),clear_flags=True):
        m=self.machine;m.mem_write(STACK,struct.pack('<'+'I'*(1+len(args)),STOP,*args));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,cw)
        if clear_flags:m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        try:m.emu_start(entry,STOP,count=500000)
        except unicorn.UcError as error:raise RuntimeError(f'Source{entry:#x} stopped at EIP{m.reg_read(UC_X86_REG_EIP):#x}: {error}; recent={list(self.recent)}') from error
        if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError(f'Source return mismatch{entry:#x}')
    def run(self,w,cw):
        m=self.machine;m.mem_write(MATRIX,self.unit*3)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,3),(0x3d0fa2c,1),(0x3d0f7ac,0)):m.mem_write(address,struct.pack('<I',value))
        if self.binding is None:
            m.mem_write(0x3b36d30,b'\0'*4);m.mem_write(0x3b36d40,b'\x01\x00');m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
            # Actual lookup/enabled/table stores, stopping after43b751 before
            # stage initializer43a260 can require mesh conversion/Win32 heap.
            m.emu_start(0x43b700,0x43b757,count=100)
            self.binding=dict(landtable=hex(struct.unpack('<I',m.mem_read(0x3b2f718,4))[0]),static_enabled=struct.unpack('<I',m.mem_read(0x915460,4))[0],stop_boundary='0x43b757',unexecuted_initializer='0x43a260')
        for address,position in ((CAMERA,w[3:6]),(FIRST,w[6:9]),(SECOND,w[9:12])):m.mem_write(address+0x20,struct.pack('<3I',*position))
        for global_address,pointer,present in ((0x3b2cbb0,CAMERA,w[0]),(0x3b42e10,FIRST,w[1]),(0x3b42e14,SECOND,w[2])):m.mem_write(global_address,struct.pack('<I',pointer if present else 0))
        m.mem_write(0x3b2d510,b'\0\0');m.mem_write(0x3c48550,b'\0'*4);m.mem_write(0x915458,struct.pack('<I',w[19]));self.invoke(0x43aef0,cw)
        static_count=struct.unpack('<h',m.mem_read(0x3b36d3c,2))[0];dynamic_count=struct.unpack('<I',m.mem_read(0x3b36d38,4))[0];registration_count=struct.unpack('<h',m.mem_read(0x3b32724,2))[0]
        registered=bytes(m.mem_read(0x3b2e518,static_count*36))
        m.mem_write(CONTACT,struct.pack('<I',w[15]));m.mem_write(CONTACT+4,struct.pack('<3I',*w[12:15]));m.mem_write(CONTACT+0x10,struct.pack('<3I',*w[16:19]));self.invoke(0x450c10,cw,(CONTACT,),False)
        count=struct.unpack('<h',m.mem_read(0x3b32724,2))[0];selected=struct.unpack('<I',m.mem_read(0x3b42ff0,4))[0]
        return dict(header=[static_count,dynamic_count,registration_count,count,selected,m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0]],frames=list(struct.unpack('<48I',m.mem_read(MATRIX,192))),registered=list(struct.unpack('<'+'I'*(static_count*9),registered)),candidates=list(struct.unpack('<'+'I'*(count*3),m.mem_read(0x3b2f720,count*12))))
def fixtures(image):
    address=image.unpack('<I',0xe99cb8+12)[0];count=image.unpack('<h',0xe99cb8)[0];rng=random.Random(0x450c10);cases=[]
    for i in range(480):
        center=image.unpack('<3f',address+(i%count)*36)
        camera=[c+rng.uniform(-300,300) for c in center];first=[c+rng.uniform(-100,100) for c in center];second=[c+rng.uniform(-500,500) for c in center]
        query=[c+rng.uniform(-50,50) for c in center]
        cases.append([1,i%2,(i//2)%2,*map(word,camera),*map(word,first),*map(word,second),*map(word,query),word(rng.choice((0.,1.,5.,50.))),*map(word,(rng.uniform(-10,10) for _ in range(3))),word(rng.choice((40.,100.,1000.)))])
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    cases=fixtures(image);lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_scene
    probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    lib.sadx_native_player_scene_compiler.restype=ctypes.c_char_p
    identity=lib.sadx_native_scene_landtable_identity;identity.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.c_int8,ctypes.c_int8,ctypes.POINTER(ctypes.c_uint32)];identity.restype=ctypes.c_int
    source=(ctypes.c_uint8*len(image.data)).from_buffer_copy(image.data);table=(ctypes.c_uint8*4100).from_buffer_copy(image.read(TABLE_ADDRESS,4100));unit=image.read(0x389d650,64);ub=(ctypes.c_uint8*64).from_buffer_copy(unit);constants=(ctypes.c_uint32*3)(image.unpack('<I',0x7df1c8)[0],image.unpack('<I',0x7e0600)[0],image.unpack('<I',0x7e0028)[0])
    decoded_identity=(ctypes.c_uint32*4)()
    if identity(source,len(image.data),1,0,decoded_identity) or decoded_identity[0]:raise RuntimeError('Native table lookup/decode failed')
    root=pathlib.Path(__file__).resolve().parents[2];blob=b''.join(struct.pack('<20I',*w) for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    paths=[f'src/{n}.{e}' for n in ('player_scene_decode','player_scene_registration','player_scene_broadphase','player_coordinates','ninja_matrix','ninja_math','x87_math') for e in ('hpp','cpp')]+['src/player/player_scene_resources.hpp','tests/probes/native_player_scene_probe.cpp']
    report=dict(source_sha256=image.sha256,landtable=dict(address='0xe99cb8',header_sha256=sha(image.read(0xe99cb8,28)),record_count=480,records_sha256=sha(image.read(image.unpack('<I',0xe99cb8+12)[0],480*36))),library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_scene_compiler().decode(),native_sources={x:sha((native_source(root,x)).read_bytes()) for x in paths},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),fixture_sha256=sha(blob),fixture_count=len(cases),control_words=[],mismatches=[],limits=['Real source ECoast LANDTABLE, actual binding43b700 prefix through43b751 store; stopsbefore43a260','Separate actual source static registration43aef0/query450c10 experiments using that observed table and real records; no overlays','Registered dynamic-task count0 and mesh-group headnull are explicit test selectors, not observed live-world absence','Camera/player positions and FPU profiles are caller-selected, not physical gameplay observations','43a260 stage initialization/NJS mesh conversion/Win32 heap initialization remain unfinished; no heap overlays added','Object-query453820/contactresponse/completepositionpath remain unfinished','Unicorn exception flags remain limited'])
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        oracle=SceneOriginal(image,cw,unit);item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0)
        for index,w in enumerate(cases):
            original=oracle.run(w,cw);out=(ctypes.c_uint32*4282)();status=probe(source,len(image.data),table,ub,constants,(ctypes.c_uint32*20)(*w),cw,out)
            if status or out[0]:raise RuntimeError(f'Native scene failure{status}/{out[0]} at{out[1]:#x}')
            actual=dict(header=list(out[2:10]),frames=list(out[10:58]),registered=list(out[58:58+out[2]*9]),candidates=list(out[1210:1210+out[5]*3]))
            value_keys=('frames','registered','candidates');differences=[k for k in value_keys if original[k]!=actual[k]]
            if original['header'][:6]!=actual['header'][:6] or original['header'][7]!=actual['header'][7]:differences.append('header')
            if differences:item['value_mismatches']+=1
            if original['header'][6]!=actual['header'][6]:item['flag_mismatches']+=1
            if differences:report['mismatches'].append(dict(cw=hex(cw),case=index,fields=differences,input=list(map(hex,w)),original=original,native=actual))
        report['control_words'].append(item);report['original_binding']=oracle.binding;report['native_decoded_table_identity']=list(map(hex,decoded_identity));print(json.dumps(item),flush=True)
        if int(oracle.binding['landtable'],16)!=decoded_identity[1] or oracle.binding['static_enabled']!=1:raise RuntimeError('Actual source/native table binding identity mismatch')
    report['values_match']=all(not x['value_mismatches'] for x in report['control_words']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_match'] else 1
if __name__=='__main__':raise SystemExit(main())
