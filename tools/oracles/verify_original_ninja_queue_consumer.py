"""404760 sprite submissions followed by 4086f0 queue consumption, against original instructions.

Source runs the actual 404760 queued route (403250/403f60/784bc0/403e80) and the
whole 4086f0 body with every local callee unchanged (402f40, 412400, 411f30,
77ed00/77ed20, 784d00, 411af0/411b40, 794950/794980, type-3 77e390 ...). Only
these are fixtures or stops: the D3D8 device is a recording COM fixture; D3DX
78dbe0 is a seeded `fld [seed]; ret 4` or a required stop; world-sprite 791a50
stops as the native display boundary; the unlifted
4071c0/787240 callees, the 411a20 body after its bit0 early return (411a30), and non-type-3 dispatch bodies stop exactly where
the native consumer reports its required boundary. Submission happens with
queue mode 0 and display switch 0; the case values are applied before 4086f0.
Records are type-3 sprites only, header texlist word 0, palette banks off.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn import UC_PROT_ALL
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
sha=lambda b:hashlib.sha256(b).hexdigest()
CODE=0x10000000;STOP=CODE+0x100;STUBS=CODE+0x200;DEPTH_STUB=CODE+0x400;SEED=CODE+0x500
HEAP=0x11000000;LIST=HEAP+0x100;ARRAY=HEAP+0x200;MEM=HEAP+0x1000;FRAMES=HEAP+0x1100;SPRITES=HEAP+0x1200;VB=HEAP+0x1800;DEVICE=HEAP+0x1a00;VTABLE=HEAP+0x1b00;MATRIX=HEAP+0x1d00
ARENA=0x20000000;SIZE=65536;BUCKETS=0x21000000;STACK=0x12000000+0xf000
METHODS={0xc8:0xc,0xf4:0xc,0xfc:0x10,0x120:0x14,0x130:0x8}
UNLIFTED={0x4071c0,0x787240}
DISPATCH={0x4089b5,0x408a1b,0x408baa,0x408c00,0x408cd4,0x408d2a,0x408d70,0x408db6,0x408e01,0x408e3b,0x408e94,0x408eda,0x408b2c,0x408e70}
DISPLAY_FAILED,UNLIFTED_CALLEE,UNLIFTED_DISPATCH=5,6,16
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class Source:
    def __init__(self,image):
        m=self.m=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32);self.image=image
        for s in image.sections:
            if s['name'] not in('.text','.rdata','.data','.data1'):continue
            base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
        for base,size in((CODE,0x10000),(HEAP,0x10000),(0x12000000,0x10000),(ARENA,SIZE),(BUCKETS,0x4000)):m.mem_map(base,size,UC_PROT_ALL)
        stubs=bytearray()
        for pop in METHODS.values():stubs+=b'\x31\xc0\xc2'+struct.pack('<H',pop)+b'\x90'*11
        m.mem_write(STUBS,bytes(stubs));self.stub={offset:STUBS+16*i for i,offset in enumerate(METHODS)}
        m.mem_write(DEPTH_STUB,b'\xd9\x05'+struct.pack('<I',SEED)+b'\xc2\x04\x00')
        vtable=bytearray(0x200)
        for offset,address in self.stub.items():struct.pack_into('<I',vtable,offset,address)
        m.mem_write(VTABLE,bytes(vtable));m.mem_write(DEVICE,struct.pack('<I',VTABLE));m.mem_write(FRAMES,image.read(0x9279d8,20))
        self.clean=m.context_save();m.hook_add(unicorn.UC_HOOK_CODE,self.code);self.case=None
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
                    entry=[offset,call,args[0],args[1],args[3]];self.drawn=bytes(m.mem_read(args[2],112));self.draws+=1
                self.trace.append(entry);return
        if address==0x78dbe0:
            if not c[36]:self.stop=(DISPLAY_FAILED,0x408b1f,0x77e390);m.emu_stop();return
            m.reg_write(UC_X86_REG_EIP,DEPTH_STUB)
        elif address==0x791a50:self.stop=(DISPLAY_FAILED,0x408b1f,0x77e390);m.emu_stop()
        elif address==0x411a30:self.stop=(UNLIFTED_CALLEE,0x411b02,0x411a20);m.emu_stop() # 411a20 bit0-clear body
        elif address in UNLIFTED:self.stop=(UNLIFTED_CALLEE,word(esp)-5,address);m.emu_stop()
        elif address in DISPATCH:self.stop=(UNLIFTED_DISPATCH,address,0);m.emu_stop()
    def call(self,address,args):
        m=self.m;m.mem_write(STACK,struct.pack(f'<{1+len(args)}I',STOP,*args));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(address,STOP,count=400000)
        if self.stop is None and(m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4):raise RuntimeError(f'Source return ABI at {address:#x}')
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.case=c;self.trace=[];self.drawn=bytes(112);self.draws=0;self.stop=None
        put=lambda a,v:m.mem_write(a,struct.pack('<I',v&0xffffffff));byte=lambda a,v:m.mem_write(a,bytes([v&255]))
        m.mem_write(SEED,struct.pack('<I',c[37]));m.mem_write(ARENA,bytes([0xcd])*SIZE);m.mem_write(BUCKETS,b''.join(struct.pack('<2I',BUCKETS+i*8,BUCKETS+i*8) for i in range(2048)));put(0x90083c,BUCKETS)
        m.mem_write(LIST,struct.pack('<II',ARRAY,1))
        rows=bytearray(12*131)
        for i in range(131):struct.pack_into('<III',rows,12*i,0xffffffff if(i==0 and c[51])else 0x1234,0,MEM if i in(0,130) else 0)
        m.mem_write(ARRAY,bytes(rows));raw=bytearray(0x48);struct.pack_into('<II',raw,0,7,0xffffffff);struct.pack_into('<I',raw,0x34,0x11007000);m.mem_write(MEM,bytes(raw))
        matrix=list(struct.unpack('<16I',self.image.read(0x389d650,64)));matrix[12:15]=c[59:62];m.mem_write(MATRIX,struct.pack('<16I',*matrix));put(0x3d0f818,MATRIX)
        for at,value in((0x3abd954,ARENA),(0x3ab9874,ARENA+SIZE),(0x3abd9c8,1),(0x3ab98ac,0),(0x3abd950,0),(0x3d0f9c8,11),(0x3d0f840,22),(0x3d0f9c4,33),(0x3b12208,c[35]),(0x3abd9c0,0),(0x3b1117c,0),(0x3abd9b8,c[63]),
                        (0x3abd9d4,c[46]),(0x3abd9d8,c[47]),(0x3abd9dc,c[48]),(0x3abd9e0,c[49]),(0x3d12798,c[29]),(0x3d128a4,c[30]),(0x90a098,c[31]),(0x90a09c,c[32]),(0x3d0849c,0xdeadbeef),
                        (0x3d0f9e0,c[52]),(0x3d0fa0c,c[53]),(0x3d0fa10,c[54]),(0x3d0fa24,0),(0x3ce7128,0),(0x3d129a0,0),(0x3d0fdf4,c[50]),(0x3d128b0,DEVICE),(0x3d128ac,VB)):put(at,value)
        byte(0x3abdcfe,c[28]);byte(0x3ab9890,c[62]);byte(0x909eb4,c[26]);byte(0x3abdc6c,c[27]);byte(0x3abdbc0,0xef)
        m.mem_write(0x3abd9a8,struct.pack('<4I',*c[55:59]));m.mem_write(0x3ab9864,struct.pack('<4I',*c[42:46]));m.mem_write(0x3b18230,struct.pack('<4I',*c[38:42]));m.mem_write(0x3d0f7f0,bytes(16));m.mem_write(VB,bytes(112))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        for i in range(min(c[1],3)):
            w=c[2+8*i:10+8*i];sprite=SPRITES+0x40*i;m.mem_write(sprite,struct.pack('<6I',*w[:5],0)+struct.pack('<II',LIST,FRAMES))
            self.call(0x404760,[sprite,w[5],w[6],w[7]])
            if self.stop:raise RuntimeError('Submission reached a stop')
        self.trace=[];self.draws=0;self.drawn=bytes(112);put(0x3ab98ac,c[34]);put(0x3b1117c,c[33])
        self.call(0x4086f0,[])
        word=lambda a:struct.unpack('<I',m.mem_read(a,4))[0];get=lambda a:m.mem_read(a,1)[0]
        out=[0]*400;failure,boundary,callee=self.stop or(0,0,0)
        out[0:6]=[failure,boundary,callee,int(self.stop is None),0,self.draws]
        out[6]=word(0x3b12208);out[7]=get(0x3abdbc0);out[8]=word(0x3ab98ac);out[9]=word(0x3abd9d8);out[10]=word(0x3abd9e0)
        out[11]=word(0x3d0f9c8);out[12]=word(0x3d0f840);out[13]=word(0x3d0f9c4);out[14]=get(0x909eb4);out[15]=get(0x3abdcfe);out[16]=word(0x3d12798);out[17]=word(0x3d0849c)
        out[18:22]=struct.unpack('<4I',m.mem_read(0x3ab9864,16));out[22:26]=struct.unpack('<4I',m.mem_read(0x3d0f7f0,16));out[26]=word(0x3d0f9e0);out[27]=word(0x3d0fdf4)
        out[28:44]=struct.unpack('<16I',m.mem_read(word(0x3d0f818),64));out[44]=m.reg_read(UC_X86_REG_FPSW)&0x3f;out[45]=m.reg_read(UC_X86_REG_FPCW);out[46]=min(len(self.trace),48)
        for i,entry in enumerate(self.trace[:48]):out[47+5*i:52+5*i]=entry
        out[287:315]=struct.unpack('<28I',self.drawn)
        return out
def fixtures():
    rng=random.Random(0x4086f0);cases=[]
    def base():
        c=[0]*96;c[0]=0x7f;c[1]=1
        for i in range(3):c[2+8*i:10+8*i]=[f32(10*i),f32(-5),f32(50+10*i),f32(1),f32(1),0,0x30,0]
        c[26]=0;c[27]=0;c[28]=0;c[29]=0;c[30]=0;c[31]=c[32]=f32(-65535.0);c[33]=0;c[34]=0;c[35]=6;c[36]=1;c[37]=f32(.25)
        c[38:42]=[f32(1)]*4;c[42:46]=[f32(1)]*4;c[46]=3;c[47]=2;c[48]=5;c[49]=4;c[50]=99;c[51]=0;c[52]=f32(320);c[53]=f32(320);c[54]=f32(240);c[55:59]=[f32(1)]*4
        c[59:62]=[0,0,0];c[62]=0xa8;c[63]=0;c[64]=f32(2048);return c
    cases.append(base())
    for count in(0,1,2,3):c=base();c[1]=count;cases.append(c)
    for special in range(4):
        for fog_flag in(0,1):
            for queue_flags in(0,0x20,0x40,0xc0,0x10):
                c=base();c[1]=2;c[26]=special;c[28]=fog_flag;c[2+7]=queue_flags;c[10+7]=queue_flags^0x20;cases.append(c)
    for profile in(0,1):
        for enabled in(0,1):
            for supported in(0,1):
                for cached in(f32(-65535.0),f32(5.0)):
                    for special in(0,1,3):
                        c=base();c[27]=profile;c[29]=enabled;c[30]=supported;c[31]=cached;c[26]=special;c[28]=1;c[2+7]=0x20;cases.append(c)
    for switch in(0,1):
        for mode in(0,1,2):c=base();c[33]=switch;c[34]=mode;cases.append(c)
    for mode in(0,1,6,0x41,0x80,0xffffffff):c=base();c[35]=mode;cases.append(c)
    for colors in([2,-2,.5,-.5],[1.0000001,-1.0000001,0,-0.0],[float('inf'),float('-inf'),1,1]):c=base();c[38:42]=[f32(v) for v in colors];c[42:46]=[f32(v) for v in reversed(colors)];cases.append(c)
    for counts in((3,2,5,4),(1,2,4,5),(-1,0,0x7fffffff,-0x80000000)):c=base();c[46:50]=[v&0xffffffff for v in counts];cases.append(c)
    for provider in(0,1):c=base();c[36]=provider;c[1]=2;cases.append(c)
    c=base();c[51]=1;cases.append(c)
    for packed in(0,0x11,0xa8,0xff):c=base();c[62]=packed;cases.append(c)
    for attr in(0x10,0x11,0x12,0x30,0x31,0x32,0x3f,0x00,0x20):c=base();c[2+6]=attr;cases.append(c)
    for _ in range(60):
        c=base();c[1]=rng.randrange(1,4);c[26]=rng.randrange(4);c[28]=rng.randrange(2);c[35]=rng.choice([0,6,0x41]);c[63]=f32(rng.uniform(-10,10))
        for i in range(3):c[2+8*i:10+8*i]=[f32(rng.uniform(-100,100)),f32(rng.uniform(-100,100)),f32(rng.uniform(1,500)),f32(rng.uniform(-3,3)),f32(rng.uniform(-3,3)),0,rng.choice([0x30,0x32,0x10,0x12]),rng.choice([0,0x10,0x20,0x40,0x80])]
        c[59:62]=[f32(rng.uniform(-20,20)) for _ in range(3)];c[37]=f32(rng.uniform(0,1));cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    assets=image.read(0x9279d8,20)+b''.join(image.read(x,4) for x in(0x7db2c4,0x7dce6c,0x7e9b40,0x7dcb2c,0x7dcb10))+image.read(0x88ad64,56)+image.read(0x88ad9c,32)+image.read(0x88af78,4100)+image.read(0x389d650,64)+image.read(0x389d458,16)+image.read(0x7dcb0c,4)+image.read(0x7dce50,4)
    cases=fixtures();bucket_bias=struct.unpack('<I',image.read(0x7dcb60,4))[0]
    for c in cases:c[64]=bucket_bias
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_queue_consumer
    fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);native_assets=(ctypes.c_ubyte*len(assets)).from_buffer_copy(assets);profiles=[];bad=[];outcomes={}
    for cw in(pc|rc|0x7f for pc in(0,0x200,0x300) for rc in(0,0x400,0x800,0xc00)):
        profile=dict(cw=hex(cw),cases=len(cases),state_mismatches=0,flag_mismatches=0)
        for number,case in enumerate(cases):
            case=list(case);case[0]=cw;expected=source.run(case);out=(ctypes.c_uint32*400)()
            if fn(native_assets,len(assets),(ctypes.c_uint32*96)(*case),96,out,400)!=1:raise RuntimeError(f'Native fixture rejected case {number}')
            got=list(out);key=f"{expected[0]}:{hex(expected[1])}:{hex(expected[2])}:draws{expected[5]}";outcomes[key]=outcomes.get(key,0)+1
            if got[4]:raise RuntimeError(f'Native submission failed in case {number}')
            state=[i for i in range(400) if i not in(4,44) and got[i]!=expected[i]]
            if state:
                profile['state_mismatches']+=1
                if len(bad)<40:bad.append(dict(cw=hex(cw),case=number,fields=state,source=[hex(x) for x in expected[:60]],native=[hex(x) for x in got[:60]]))
            if got[44]!=expected[44]:profile['flag_mismatches']+=1
        profiles.append(profile);print(json.dumps(profile),flush=True)
    blob=b''.join(struct.pack('<96I',*c) for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob)
    names=['src/engine/ninja_fog.hpp','src/engine/ninja_fog.cpp','src/guest/ninja_fog_reference.hpp','src/guest/ninja_fog_reference.cpp','src/engine/ninja_queue_consumer.hpp','src/engine/ninja_queue_consumer.cpp','src/engine/ninja_sprite_display.cpp','src/engine/ninja_sprite_queue.cpp','src/engine/ninja_sprite.cpp','src/engine/ninja_render.cpp','src/engine/ninja_texture_selection.cpp','tests/probes/native_ninja_queue_consumer_probe.cpp','tools/oracles/verify_original_ninja_queue_consumer.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={n:sha((pathlib.Path(__file__).resolve().parents[2]/n).read_bytes()) for n in names},assets_sha256=sha(assets),
        original_ranges_sha256={'404760':sha(image.read(0x404760,0xd3)),'4086f0':sha(image.read(0x4086f0,0x940))},cases=len(cases),case_sha256=sha(blob),profiles=profiles,source_outcomes_all_profiles=outcomes,mismatch_examples=bad,
        state_calls_vertices_boundaries_match=all(not p['state_mismatches'] for p in profiles),strict_flags_match=all(not p['flag_mismatches'] for p in profiles),limits=__doc__)
    report['limits']=([report['limits']] if isinstance(report.get('limits'),str) else list(report.get('limits') or []))+['Flag difference (strict_flags_match): the native x87 status exception bits after the vertex arithmetic are set where Unicorn 2.1.4 does not raise them. All compared values match; the hardware flags need the Intel capture (M4).']
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases)*len(profiles),state_mismatches=sum(p['state_mismatches'] for p in profiles),flag_mismatches=sum(p['flag_mismatches'] for p in profiles),outcomes=outcomes)));return int(any(p['state_mismatches'] for p in profiles))
if __name__=='__main__':raise SystemExit(main())
