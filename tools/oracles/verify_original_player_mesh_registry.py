"""Actual fixed-pool/triangle/registry source instructions; no code overlays.

Real ECoast arrays remain intact. Bounded landtable count views and zero-filled
pool/camera inputs are explicit synthetic resource/state profiles.
"""
import argparse,ctypes,hashlib,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256,TABLE_ADDRESS
from verify_original_player_scene import SceneOriginal,MATRIX,CAMERA
from verify_original_math import DATA,STACK,STOP

GROUPS=0x3b43000;NODES=0x3b48550;BUCKETS=0x3b48150
WORDS=17+256*16+16384*16+256+1024*3+128*3+8*16+4+1024*3+1
def sha(data):return hashlib.sha256(data).hexdigest()
def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]
class RegistryOriginal(SceneOriginal):
    def run(self,mode,w,cw):
        m=self.machine
        for address,count in ((GROUPS,256*64),(NODES,16384*64),(BUCKETS,1024),(0x3b2f720,1024*12),(0x3b32728,128*12),(0x3b2e518,128*36)):
            m.mem_write(address,bytes(count))
        m.mem_write(MATRIX,self.unit*8)
        for address,value in ((0x3d0f818,MATRIX),(0x3d0fa28,8),(0x3d0fa2c,1),(0x3d0f7ac,0),(0x3c4855c,0),(0x3b4758c,w[2]),(0x3b47588,0),(0x3b42ff8,0),(0x3b36d38,0),(0x3b36d3c,0),(0x3b32724,0),(0x3c837a8,1),(0x2bcc6f0,1)):
            m.mem_write(address,struct.pack('<I',value))
        self.invoke(0x451030,cw);initializer_return=m.reg_read(UC_X86_REG_EAX);self.invoke(0x4518d0,cw,clear_flags=False)
        land=self.image.unpack('<I',0xe99cb8+12)[0];obj=self.image.unpack('<I',land+(w[0]%480)*36+24)[0]
        if mode in (0,1):self.invoke(0x451ad0 if mode==0 else 0x451a00,cw,(obj,0,0),False);ret=m.reg_read(UC_X86_REG_EAX)
        elif mode==2:
            for _ in range(w[0]):self.invoke(0x451790,cw,clear_flags=False)
            for i in range(w[1]):self.invoke(0x452160,cw,(w[2+i%4],),False)
            group=struct.unpack('<I',m.mem_read(0x3c48550,4))[0]
            if w[0] and group:
                self.invoke(0x451910,cw,(group,),False)
                if w[6]:self.invoke(0x451800,cw,(group,),False)
            ret=0
        elif mode==6:
            vertices=DATA+0xc00;model=DATA+0xc80;descriptor=DATA+0xd00;indices=DATA+0x6000
            m.mem_write(vertices,struct.pack('<12I',w[2],0,0,0,w[3],0,0,0,w[4],word(1),word(2),word(3)));m.mem_write(model,struct.pack('<I',vertices));m.mem_write(descriptor,bytes(16+1024*12))
            words=([w[1]&0xffff] if w[0] in (0x8000,0xc000) else [])+[i%4 for i in range(1024)];m.mem_write(indices,struct.pack('<'+'H'*len(words),*words));m.mem_write(descriptor,struct.pack('<4I',w[0],w[1],indices,model));self.invoke(0x452230,cw,(descriptor,),False);ret=m.reg_read(UC_X86_REG_EAX)
        else:
            header=bytearray(self.image.read(0xe99cb8,28));struct.pack_into('<h',header,0,w[0]);m.mem_write(DATA+0xb00,bytes(header));m.mem_write(0x3b2f718,struct.pack('<I',DATA+0xb00));m.mem_write(0x915460,struct.pack('<I',1));m.mem_write(0x3b2d510,b'\0\0')
            m.mem_write(CAMERA+0x20,struct.pack('<3I',*w[1:4]));m.mem_write(0x3b2cbb0,struct.pack('<I',CAMERA));m.mem_write(0x3b42e10,bytes(8));m.mem_write(0x915458,struct.pack('<I',w[4]));
            for _ in range(max(w[5],1) if mode==4 else 1):
                if mode==4:m.mem_write(0x3c4855c,struct.pack('<I',1))
                self.invoke({3:0x43b580,4:0x4511d0,5:0x43aef0}[mode],cw,clear_flags=False)
            ret=m.reg_read(UC_X86_REG_EAX) if mode==4 else 0
        global_addresses=(0x3c48554,0x3c48558,0x3c48550,0x3b4758c,0x3b47588,0x3c4855c,0x3b47000,0x3b42ff8,0x3b42ff4,0x3b42ff0,0x3b32724,0x3b36d38,0x3b36d3c,0x3d0fa2c)
        values=[struct.unpack('<I',m.mem_read(address,4))[0] for address in global_addresses]
        values.extend((m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63,ret))
        for address,count in ((GROUPS,256*16),(NODES,16384*16),(BUCKETS,256),(0x3b2f720,1024*3),(0x3b32728,128*3),(MATRIX,8*16)):
            values.extend(struct.unpack('<'+'I'*count,m.mem_read(address,count*4)))
        if mode==6:
            kind,count,cursor,_=struct.unpack('<4I',m.mem_read(descriptor,16));values.extend((kind,count,(cursor-indices)//2,ret));values.extend(struct.unpack('<3072I',m.mem_read(descriptor+16,1024*12)))
        else:values.extend([0]*(4+1024*3))
        values.append(initializer_return);return values
def fixtures():
    result=[]
    for ordinal in (0,1,2,3,7,15,31,47,63,79,127,159,191,239,319,399,479):
        for mode in (0,1):result.append((mode,[ordinal,0,word(0),0,0,0,0]))
    for groups,nodes,release in ((0,0,0),(1,0,0),(2,1,0),(4,12,0),(4,12,1),(255,129,1),(256,129,0),(257,1,0),(1,16383,0),(1,16384,1)):
        result.append((2,[groups,nodes,*map(word,(-2048,-1,0,2048)),release]))
    for count in (0,1,4,16,127,128,129,480):result.append((3,[count,0,word(0),0,word(40),0,0]))
    for count in (0,1,4):result.append((4,[count,0,word(0),0,word(40),0,0]))
    for count in (1,4):result.append((4,[count,0,word(0),0,word(40),2,0]))
    for count in (0,1,4,128,480):result.append((5,[count,word(-100),word(100),word(0),word(1000),0,0]))
    for kind in (0,0x4000,0x8000,0xc000,0x1234):
        counts=(0,3,4) if kind<0x8000 else (0,1,2,3,4,0x8003,0x8400,0x8401,0xffff)
        for count in counts:result.append((6,[kind,count,word(-4),word(5),word(-6),0,0]))
    return result
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.library.resolve()));probe=lib.sadx_native_player_mesh_registry;probe.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint8),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    lib.sadx_native_player_mesh_registry_compiler.restype=ctypes.c_char_p
    source=(ctypes.c_uint8*len(image.data)).from_buffer_copy(image.data);table=(ctypes.c_uint8*4100).from_buffer_copy(image.read(TABLE_ADDRESS,4100));unit=image.read(0x389d650,64);ub=(ctypes.c_uint8*64).from_buffer_copy(unit)
    cases=fixtures();blob=b''.join(struct.pack('<8I',mode,*w) for mode,w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    root=pathlib.Path(__file__).resolve().parent.parent;names=('player_mesh_registry','player_mesh_matrix','player_coordinates','player_scene_decode','player_scene_registration','player_scene_broadphase','crt_fabs','crt_integer','ninja_math','x87_math')
    paths=[f'src/{name}.{ext}' for name in names for ext in ('cpp','hpp')]+['src/player/player_scene_resources.hpp','tests/probes/native_player_mesh_registry_probe.cpp']
    report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_player_mesh_registry_compiler().decode(),native_sources={path:sha((root/path).read_bytes()) for path in paths},verifier_sha256=sha(pathlib.Path(__file__).read_bytes()),case_sha256=sha(blob),profiles=[],mismatches=[],limits=['Actual source instructions, no function overlays','Zero-filled pool bytes and masked CW/matherr1/fabs-policy1 are explicit test profiles, not observed startup state','Real Emerald Coast vertex/index/material/object arrays and exact source transforms; bounded landtable count views are synthetic metadata','Full registry4511d0 uses actual43b580 force profile plus real mesh production; regular43aef0 prefix is separately checked','No physical Intel or updated-mod runtime capture; x87 flags/oracle limitations retained','449500/445d10 positioning and43a260 stage resource startup remain unfinished'])
    oracle=RegistryOriginal(image,0x27f,unit);oracle.image=image
    for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)):
        item=dict(cw=hex(cw),cases=len(cases),value_mismatches=0,flag_mismatches=0)
        for index,(mode,w) in enumerate(cases):
            original=oracle.run(mode,w,cw);out=(ctypes.c_uint32*WORDS)();status=probe(source,len(image.data),table,ub,mode,(ctypes.c_uint32*7)(*w),cw,out)
            if status:raise RuntimeError(f'Native mode{mode}/case{index} failed {status}/{out[0]} at {out[1]:#x}')
            differences=[slot for slot in range(WORDS) if slot!=15 and original[slot]!=out[slot]]
            if original[15]!=out[15]:item['flag_mismatches']+=1
            if differences:
                item['value_mismatches']+=1;report['mismatches'].append(dict(cw=hex(cw),mode=mode,case=index,input=list(map(hex,w)),difference_count=len(differences),first_differences=[dict(word=slot,original=hex(original[slot]),native=hex(out[slot])) for slot in differences[:24]]))
        report['profiles'].append(item);print(json.dumps(item),flush=True)
    report['values_returns_cw_match']=all(not profile['value_mismatches'] for profile in report['profiles']);report['strict_match']=report['values_returns_cw_match'] and all(not profile['flag_mismatches'] for profile in report['profiles']);a.output.write_text(json.dumps(report,indent=2)+'\n');return 0 if report['values_returns_cw_match'] else 1
if __name__=='__main__':raise SystemExit(main())
