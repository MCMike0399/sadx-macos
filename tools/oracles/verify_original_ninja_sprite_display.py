"""Full 77e390 sprite display against original instructions.

Executes actual 77f3d0/77f440/78cf20/78cdc0 texture selection, 781670 and the
sprite geometry, 78b840/791940 state helpers, vertex stores, the D3D8 calls and
78c5a0 RET. The D3D8 device is an explicit recording COM fixture returning the
case HRESULT (ignored by the source). D3DX 78dbe0 is unlifted: it is either a
required stop or an explicit seeded `fld [seed]; ret 4` fixture, never a model
of projection. The world (flags&0x10==0) branch stops at unlifted 791a50.
Palette banks are off (id 0xffffffff); texture indices stay inside the list.
This does not establish Windows GPU raster/filter equality or full queue use.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn import UC_PROT_ALL
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
sha=lambda b:hashlib.sha256(b).hexdigest()
CODE=0x10000000;STOP=CODE+0x100;STUBS=CODE+0x200;DEPTH_STUB=CODE+0x400;SEED=CODE+0x500
HEAP=0x11000000;PACKET=HEAP;FRAMES=HEAP+0x100;LIST=HEAP+0x200;ARRAY=HEAP+0x300;MEM=(HEAP+0x400,HEAP+0x500);VB=HEAP+0x800;DEVICE=HEAP+0xa00;VTABLE=HEAP+0xb00
STACK=0x12000000+0xf000
METHODS={0xc8:0xc,0xf4:0xc,0xfc:0x10,0x120:0x14,0x130:0x8}
FAIL={'none':0,'world':9,'depth':10}
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class Source:
    def __init__(self,image):
        m=self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
        for s in image.sections:
            if s['name'] not in('.text','.rdata','.data','.data1'):continue
            base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
        for base in(CODE,HEAP,0x12000000):m.mem_map(base,0x10000,UC_PROT_ALL)
        stubs=bytearray()
        for i,(offset,pop) in enumerate(METHODS.items()):
            # mov eax,[HRESULT]; ret pop
            stubs+=b'\xa1'+struct.pack('<I',SEED+4)+b'\xc2'+struct.pack('<H',pop)+b'\x90'*8
        m.mem_write(STUBS,bytes(stubs));self.stub={offset:STUBS+16*i for i,offset in enumerate(METHODS)}
        m.mem_write(DEPTH_STUB,b'\xd9\x05'+struct.pack('<I',SEED)+b'\xc2\x04\x00')
        vtable=bytearray(0x200)
        for offset,address in self.stub.items():struct.pack_into('<I',vtable,offset,address)
        m.mem_write(VTABLE,bytes(vtable));m.mem_write(DEVICE,struct.pack('<I',VTABLE))
        self.clean=m.context_save();m.hook_add(unicorn.UC_HOOK_CODE,self.code)
        self.case=None
    def code(self,m,address,size,user):
        c=self.case
        if c is None:return
        esp=m.reg_read(UC_X86_REG_ESP);word=lambda at:struct.unpack('<I',m.mem_read(at,4))[0]
        for offset,stub in self.stub.items():
            if address==stub:
                call=word(esp)-6;args=[word(esp+8+4*i) for i in range(METHODS[offset]//4-1)]
                if offset==0xc8:entry=[offset,call,args[0],args[1],0]
                elif offset==0xfc:entry=[offset,call,args[0],args[1],args[2]]
                elif offset==0xf4:entry=[offset,call,args[0],args[1],0]
                elif offset==0x130:entry=[offset,call,args[0],0,0]
                else:
                    entry=[offset,call,args[0],args[1],args[3]]
                    if args[2]!=VB:raise RuntimeError('DrawPrimitiveUP data is not the source vertex buffer')
                    self.drawn=bytes(m.mem_read(VB,112))
                self.trace.append(entry);return
        if address==0x78dbe0:
            self.reciprocal=word(esp+4)
            if not c[49]:self.failure=('depth',0x77e637);m.emu_stop();return
            m.reg_write(UC_X86_REG_EIP,DEPTH_STUB)
        elif address==0x791a50:self.failure=('world',0x77e716);m.emu_stop()
    def run(self,c,assets_unused=None):
        m=self.m;m.context_restore(self.clean);self.case=c;self.trace=[];self.drawn=bytes(112);self.reciprocal=None;self.failure=None
        put=lambda a,v:m.mem_write(a,struct.pack('<I',v&0xffffffff))
        m.mem_write(SEED,struct.pack('<II',c[50],c[51]));m.mem_write(VB,bytes(112))
        m.mem_write(PACKET,struct.pack('<6I',*c[3:9])+struct.pack('<II',0 if c[56] else LIST,FRAMES))
        m.mem_write(FRAMES,struct.pack('<10I',*c[9:19]))
        m.mem_write(LIST,struct.pack('<II',ARRAY,c[42]))
        rows=bytearray(12*c[42])
        for i in range(c[42]):struct.pack_into('<III',rows,12*i,HEAP+0xf00,0,MEM[i%2])
        m.mem_write(ARRAY,bytes(rows))
        for i,p in enumerate(MEM):
            raw=bytearray(0x48);struct.pack_into('<II',raw,0,c[47+i],0xffffffff);struct.pack_into('<I',raw,0x20,c[53]);struct.pack_into('<I',raw,0x34,c[45+i]);m.mem_write(p,bytes(raw))
        matrix=0x12000000+0x100;m.mem_write(matrix,struct.pack('<16I',*c[19:35]));put(0x3d0f818,matrix)
        put(0x3d0f9e0,c[35]);put(0x3d0fa0c,c[36]);put(0x3d0fa10,c[37]);m.mem_write(0x3d0f7f0,struct.pack('<4I',*c[38:42]))
        put(0x3d0fa24,LIST if c[43] else 0);put(0x3ce7128,0);put(0x3d129a0,0 if not c[52] else MEM[c[52]-1]);put(0x3d0fdf4,c[44])
        put(0x3d128b0,DEVICE);put(0x3d128ac,VB)
        m.mem_write(STACK,struct.pack('<4I',STOP,0 if c[55] else PACKET,c[1],c[0]))
        m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,c[2]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(0x77e390,STOP,count=50000)
        if not self.failure and(m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError('Source return ABI')
        word=lambda a:struct.unpack('<I',m.mem_read(a,4))[0]
        def ref(v):return 0 if v==0 else MEM.index(v)+1 if v in MEM else 0xfe
        out=[0]*128
        kind,boundary=self.failure or('none',0)
        out[0]=FAIL[kind];out[1]=boundary;out[3]=int(self.failure is None)
        out[4]=int(self.reciprocal is not None and c[49]!=0);out[5]=self.reciprocal if(self.reciprocal is not None and c[49])else 0
        out[6]=word(0x3d0fdf4);out[7]=ref(word(0x3d129a0));out[8]=ref(word(0x3ce7128));list_value=word(0x3d0fa24);out[9]=1 if list_value==LIST else 0 if list_value==0 else 0xfe
        out[10]=m.reg_read(UC_X86_REG_FPSW)&0x3f;out[11]=m.reg_read(UC_X86_REG_FPCW);out[12]=min(len(self.trace),12)
        for i,entry in enumerate(self.trace[:12]):out[13+5*i:18+5*i]=entry
        out[73:101]=struct.unpack('<28I',self.drawn)
        return out
def frame(sx,sy,cx,cy,u1,v1,u2,v2,texid,attr):return list(struct.unpack('<5I',struct.pack('<10h',sx,sy,cx,cy,u1,v1,u2,v2,texid,attr)))
def fixtures(unit):
    rng=random.Random(0x77e390);cases=[]
    def base():
        c=[0]*64;c[0]=0x30;c[2]=0x7f;c[3:9]=[f32(10),f32(-5),f32(50),f32(1),f32(1),0]
        c[9:14]=frame(32,32,16,16,0,0,255,255,1,0);c[14:19]=frame(16,8,0,8,10,20,200,180,0,0)
        c[19:35]=list(unit);c[35]=f32(320);c[36]=f32(320);c[37]=f32(240);c[38:42]=[f32(1),f32(.5),f32(.25),f32(1)]
        c[42]=2;c[43]=0;c[44]=99;c[45]=0x11007000;c[46]=0x11008000;c[47]=7;c[48]=8;c[49]=1;c[50]=f32(.25);c[51]=0;c[52]=0;c[53]=0
        return c
    for flags in range(64):
        for z in(50,-50,f32(0)):
            c=base();c[0]=flags;c[5]=f32(z) if isinstance(z,int) else z;cases.append(c)
    for provider in(0,1):
        for z in(-1e-9,0.0,1e-9,1.0,1e6):
            c=base();c[49]=provider;c[5]=f32(z);cases.append(c)
    for list_state in(0,1):
        for cache in(7,8,99):
            for active in(0,1,2):
                for surface in((0x11007000,0x11008000),(0,0x11008000),(0x11007000,0)):
                    for texid in(0,1,2):
                        c=base();c[43]=list_state;c[44]=cache;c[52]=active;c[45],c[46]=surface;c[42]=3;c[9:14]=frame(32,32,16,16,0,0,255,255,texid,0);cases.append(c)
    for fmt in(0,0x08000000,0x10000000,0x18000000,0xffffffff):c=base();c[53]=fmt;cases.append(c)
    for index in(0,1):c=base();c[1]=index;cases.append(c)
    c=base();c[55]=1;cases.append(c);c=base();c[56]=1;cases.append(c)
    for hresult in(0,0x8876086c,0xffffffff):c=base();c[51]=hresult;cases.append(c)
    for _ in range(160):
        c=base();c[0]=rng.choice([0x10,0x30,0x11,0x31,0x12,0x32,0x3f])
        c[3:6]=[f32(rng.uniform(-200,200)),f32(rng.uniform(-200,200)),f32(rng.uniform(-20,400))];c[6]=f32(rng.uniform(-4,4));c[7]=f32(rng.uniform(-4,4))
        c[9:14]=frame(rng.randrange(1,128),rng.randrange(1,128),rng.randrange(-64,64),rng.randrange(-64,64),rng.randrange(256),rng.randrange(256),rng.randrange(256),rng.randrange(256),rng.randrange(2),0)
        m=list(unit);m[12]=f32(rng.uniform(-50,50));m[13]=f32(rng.uniform(-50,50));m[14]=f32(rng.uniform(-50,50));c[19:35]=m
        c[35]=f32(rng.uniform(50,800));c[36]=f32(rng.uniform(0,640));c[37]=f32(rng.uniform(0,480));c[38:42]=[f32(rng.uniform(-.5,1.5)) for _ in range(4)];c[50]=f32(rng.uniform(0,1));cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    assets=image.read(0x9279d8,20)+b''.join(image.read(x,4) for x in(0x7db2c4,0x7dce6c,0x7e9b40,0x7dcb2c,0x7dcb10))+image.read(0x88ad64,56)+image.read(0x88af78,4100)+image.read(0x389d650,64)+image.read(0x389d458,16)
    unit=struct.unpack('<16I',image.read(0x389d650,64));cases=fixtures(unit)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_sprite_display
    fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);native_assets=(ctypes.c_ubyte*len(assets)).from_buffer_copy(assets)
    profiles=[];bad=[];outcomes={}
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300) for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,flag_mismatches=0)
        for number,case in enumerate(cases):
            case=list(case);case[2]=cw;expected=source.run(case);out=(ctypes.c_uint32*128)()
            if fn(native_assets,len(assets),(ctypes.c_uint32*64)(*case),64,out,128)!=1:raise RuntimeError(f'Native fixture rejected case {number}')
            got=list(out);key=f"{expected[0]}:{hex(expected[1])}:{expected[12]}";outcomes[key]=outcomes.get(key,0)+1
            state=[i for i in range(128) if i!=10 and got[i]!=expected[i]]
            if state:
                profile['state_mismatches']+=1
                if len(bad)<40:bad.append(dict(cw=hex(cw),case=number,fields=state,source=expected,native=got))
            if got[10]!=expected[10]:profile['flag_mismatches']+=1
        profiles.append(profile);print(json.dumps(profile),flush=True)
    blob=b''.join(struct.pack('<64I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/ninja/ninja_sprite_display.hpp','src/ninja/ninja_sprite_display.cpp','src/ninja/ninja_sprite.cpp','src/ninja/ninja_texture_selection.cpp','src/ninja/ninja_render.cpp','tests/probes/native_ninja_sprite_display_probe.cpp','tools/oracles/verify_original_ninja_sprite_display.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},
        assets_sha256=sha(assets),original_range_sha256=sha(image.read(0x77e390,0x487)),cases=len(cases),case_sha256=sha(blob),profiles=profiles,source_outcomes_all_profiles=outcomes,
        mismatch_examples=bad,state_calls_vertices_boundaries_match=all(not p['state_mismatches'] for p in profiles),strict_flags_match=all(not p['flag_mismatches'] for p in profiles),limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases)*len(profiles),state_mismatches=sum(p['state_mismatches'] for p in profiles),flag_mismatches=sum(p['flag_mismatches'] for p in profiles))));return int(any(p['state_mismatches'] for p in profiles))
if __name__=='__main__':raise SystemExit(main())
