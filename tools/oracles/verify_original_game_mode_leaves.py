"""40c090 game-mode leaves against original instructions: 4144e0, 421f70,
421f80, 4f88a0, 4221e0, 426050, 42cea0, 421f90, 427ee0, 427ec0, 425af0,
43b490, 42f5d0, 413300, 427f10, 420e90, 414470 (with 46b230), 42c730 (with
the real 42e470, 402f10/780c00, 42c4e0, 432ea0 and 78c5a0) and the composites
415000 and 42c7b0.

The whole PE image is mapped. Unported or elsewhere-owned callees 422680,
42d3b0, 403290, 4030d0, 4073b0, 785330, 40b1a0, 40b460, 421180 (LoadPVM, the
loader's LoadPVM_r in the profile), 411e30 and 77ed40 are fixtures: each call either returns (EAX 0) or, at a
chosen call, stops the source as the native nested owner stops. Compared:
completion, nested-call count, every owned scalar and block byte, the
borrowed words (3b290e4, 3b2a2fa, 3b11178, 3b2a2e4, 3b2a2e8, seed 2bcbdf8),
whether the level pointer 3b0ef4c and the event task 3b2c578 are null, and
EAX where the native result defines it. Cases that reach 420e90's texture
release walk (count 1, nonzero list) are a native explicit nested owner and
are counted, not compared.

422680 and its callers run the real per-character code against the model
DLLs mapped at the native layout with relocations applied by this script (an
implementation independent of the native rebase); the three table pointers
(38f6e7c, 3aad104, 3a7cf28) are inputs. Characters whose 422680 case loops
forever (1, 4, above 7) are native explicit boundaries.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
SCALARS=[(a,1) for a in(0x3b0ef34,0x3b0ef35,0x3b0f128,0x3b0ef48,0x3abe134,0x3b0f0e0,0x3b290d8,0x3b290e0,0x3b290e8,0x3b2a2f8,0x3b2a2f9,0x3b2a2fb,0x3b2c565,0x3b2c574)]+\
 [(a,2) for a in(0x3b22dc0,0x3b0f0e4,0x3b2a300,0x3b22e1c,0x3b18db8)]+[(a,4) for a in(0x3b0f13c,0x3b0f108,0x3b0ef44,0x3b290dc,0x3b2a2f0,0x3b2a2f4,0x3b2a2fc,0x3b2b358,0x3b2c47c,0x3b2919c,0x3b0f0f8,0x3b0f0fc,0x3b0f100,0x3b0f10c,0x3b0f0f4,0x3b2c560,0x914788,0x3b18244,0x3b22e20,0x3d08420)]+\
 [(0x3b290e4,4),(0x3b2a2fa,1),(0x3b0ef4c,4),(0x3b11178,4),(0x3b2a2e4,4),(0x3b2a2e8,4),(0x3b2c578,4),(0x2bcbdf8,4)]+\
 [(0x38f6e7c,4),(0x3aad104,4),(0x3a7cf28,4),(0x3b22de4,2),(0x3b22dcc,2)]+[(a,4) for a in(0x3c566c0,0x3c566d0,0x3c559c0,0x3c559d0,0x3c559e0,0x3c559f0,0x3c55a00,0x3c53510,0x3c53520)]+\
 [(0x3b29d5c,4),(0x8928c0,4),(0x8928c4,4)]+\
 [(0x3b2c420,4),(0x3b29d6c,4),(0x3b29d68,4),(0x3ce7174,4),(0x3ce7178,4),(0x3ce717c,4),(0x3ce7180,4)]
NULLNESS={0x3b0ef4c,0x3b2c578}
BLOCKS=[(0x3b18250,0xb64),(0x3b0ef9c,0x3c),(0x3c4ae08,0x3638),(0x3b290c4,8),(0x90ed54,0x20),(0x3b2b5f9,7),(0x3b2c5e8,0x78)]
NESTED=(0x422680,0x785330,0x42d3b0,0x403290,0x4030d0,0x4073b0,0x40b1a0,0x40b460,0x421180,0x411e30,0x77ed40)
LEAVES=[0x42c4f0,0x42c4f0,0x413c00,0x783260,0x422680,0x422680,0x4144e0,0x421f70,0x421f80,0x4f88a0,0x4221e0,0x426050,0x42cea0,0x421f90,0x427ee0,0x427ec0,0x425af0,0x43b490,0x42f5d0,0x413300,0x427f10,0x420e90,0x414470,0x415000,0x42c7b0,0x42c730,0x42c730]
DEFINED_EAX={0x413c00,0x783260,0x422680,0x4144e0,0x421f70,0x421f80,0x4f88a0,0x4221e0,0x426050,0x42cea0,0x421f90,0x427ee0,0x427ec0,0x43b490,0x42f5d0,0x413300,0x427f10,0x414470,0x42c730}
def layout():
    o=0;out=[]
    for a,w in SCALARS:out.append((a,w,o));o+=w
    for a,n in BLOCKS:out.append((a,n,o));o+=n
    return out,o
FILES=["CHRMODELS_orig.dll","ADV00MODELS.DLL","ADV01MODELS.DLL","ADV01CMODELS.DLL","ADV02MODELS.DLL","ADV03MODELS.DLL","BOSSCHAOS0MODELS.DLL"]
def rebase(path,base):
    b=path.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0];opt=pe+24;pref=struct.unpack_from('<I',b,opt+28)[0];size=struct.unpack_from('<I',b,opt+56)[0]
    nsec=struct.unpack_from('<H',b,pe+6)[0];sh=opt+struct.unpack_from('<H',b,pe+20)[0];img=bytearray(size)
    for i in range(nsec):
        vs,va,rs,ro=struct.unpack_from('<IIII',b,sh+40*i+8);img[va:va+min(rs,size-va)]=b[ro:ro+min(rs,size-va)]
    rrva,rsize=struct.unpack_from('<II',b,opt+96+40);at=rrva;delta=(base-pref)&0xffffffff
    while at+8<=rrva+rsize:
        page,block=struct.unpack_from('<II',img,at)
        if block<8:break
        for e in range(at+8,at+block,2):
            v=struct.unpack_from('<H',img,e)[0]
            if v>>12==3:t=page+(v&0xfff);struct.pack_into('<I',img,t,(struct.unpack_from('<I',img,t)[0]+delta)&0xffffffff)
            elif v>>12!=0:raise RuntimeError('relocation type')
        at+=block
    return bytes(img)
class Source:
    def __init__(self,image,system=None,layout=None):
        self.m=m=Original(image,0x37f).machine
        for s in image.sections:
            if s['name'] in('.text','.rdata'):continue
            base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
        for path,base in (layout or []):img=rebase(path,base);m.mem_map(base,(len(img)+0xfff)&~0xfff);m.mem_write(base,img)
        self.models=False
        self.clean=m.context_save()
        for a in NESTED:m.hook_add(unicorn.UC_HOOK_CODE,self.nested,begin=a,end=a)
    def nested(self,m,address,size,user):
        if address==0x422680 and self.models:return
        self.calls+=1
        if address==0x4073b0:self.rect=list(struct.unpack('<7I',m.mem_read(m.reg_read(UC_X86_REG_ESP)+4,28)))
        if self.fail_at and self.calls==self.fail_at:self.stopped=True;m.emu_stop();return
        esp=m.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',m.mem_read(esp,4))[0];m.reg_write(UC_X86_REG_EAX,0);m.reg_write(UC_X86_REG_ESP,esp+4);m.reg_write(UC_X86_REG_EIP,ret)
    def run(self,callee,argument,fail_at,blob,fields,models=False,cw=0x37f):
        m=self.m;m.context_restore(self.clean);self.calls=0;self.fail_at=fail_at;self.stopped=False;self.models=models;self.rect=[0]*7
        from unicorn.x86_const import UC_X86_REG_FPCW;m.reg_write(UC_X86_REG_FPCW,cw)
        for a,w,o in fields:m.mem_write(a,bytes(blob[o:o+w]))
        m.mem_write(STACK,struct.pack('<II',STOP,argument));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(callee,STOP,count=2000000)
        except unicorn.UcError as error: # a source memory fault (null data pointer): the native port stops there too
            if error.errno not in(unicorn.UC_ERR_READ_UNMAPPED,unicorn.UC_ERR_WRITE_UNMAPPED):raise
            self.stopped=True
        if not self.stopped and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=bytearray(len(blob))
        for a,w,o in fields:
            v=bytes(m.mem_read(a,w))
            if a in NULLNESS:v=struct.pack('<I',int(struct.unpack('<I',v)[0]!=0))
            out[o:o+w]=v
        return out,[int(not self.stopped),self.calls,m.reg_read(UC_X86_REG_EAX)]+self.rect
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('system',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_game_mode_leaves;size=lib.sadx_native_game_mode_leaves_bytes;size.restype=ctypes.c_size_t
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    fields,total=layout()
    if size()!=total:raise RuntimeError('layout mismatch')
    models=lib.sadx_native_game_mode_leaves_models;models.argtypes=[ctypes.c_char_p];models.restype=ctypes.c_int
    if models(str(a.system).encode())!=0x2000:raise RuntimeError('native model set')
    dll_layout=[];base=0x20000000
    for f in FILES:
        path=a.system/f
        if not path.exists():continue
        b=path.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0];size=struct.unpack_from('<I',b,pe+24+56)[0];dll_layout.append((path,base));base=(base+size+0xffff)&~0xffff
    chr_base=dll_layout[0][1];chr_bytes=dll_layout[0][0].read_bytes();chr_size=struct.unpack_from('<I',chr_bytes,struct.unpack_from('<I',chr_bytes,0x3c)[0]+24+56)[0]
    import verify_original_model_dlls as md
    _,ex=md.pe_exports(dll_layout[0][0]);real={0x38f6e7c:chr_base+ex['___SONIC_ACTIONS'],0x3aad104:chr_base+ex['___BIG_ACTIONS'],0x3a7cf28:chr_base+ex['___KNUCKLES_ACTIONS']}
    source=Source(image,a.system,dll_layout);rng=random.Random(0x40c090);bad=[];outcomes={};explicit=0;cases=0
    for n in range(2600):
        callee=LEAVES[n%len(LEAVES)];blob=bytearray(rng.getrandbits(8) for _ in range(total))
        def setw(addr,value,width):
            for x,w,o in fields:
                if x<=addr<x+w:blob[o+addr-x:o+addr-x+width]=value.to_bytes(width,'little');return
        character=rng.choice([0,0,1,2,3,4,5,6,7,6]);setw(0x3b22dc0,character,2)
        for c in range(8):
            setw(0x90ed54+4*c,rng.choice([0,0,rng.getrandbits(32)]),4);setw(0x3b290c4+c,rng.choice([0,1,2,0x80,0xff]),1)
        setw(0x3b0ef4c,rng.choice([0,rng.getrandbits(32)]),4);setw(0x3b2c578,rng.choice([0,rng.getrandbits(32)]),4)
        with_models=rng.random()<0.8
        for g,v in real.items():setw(g,rng.choice([v,v,v,0,chr_base+4*rng.randrange(chr_size//4-0x100)]) if with_models else rng.getrandbits(32),4)
        if callee in(0x414470,0x415000) and rng.random()<0.5:setw(0x3b22dc0,rng.choice([0,2,3,5,6,7]),2)
        if callee in(0x413c00,0x415000) and rng.random()<0.8:setw(0x3b22dcc,rng.choice([25,26,34,35,38,39,0xffff,0x8000,0x7fff,14,15]),2) # 413c00 signed stage boundaries
        argument=rng.choice([0,1,2,5,6,7]) if callee in(0x420e90,0x414470) else rng.choice([0,2,3,5,6,7,7,0,1,4,9]) if callee==0x422680 else rng.getrandbits(32)
        fail_at=0 if rng.random()<0.7 else rng.randrange(1,4)
        out=(ctypes.c_uint8*total)();res=(ctypes.c_uint32*16)()
        cw=rng.choice([0x7f,0x27f,0x37f,0xc7f])
        if callee==0x42c4f0:
            setw(0x3b29d5c,rng.choice([0,rng.getrandbits(24),rng.getrandbits(32)|0x01000000]),4)
            for g in(0x8928c0,0x8928c4):setw(g,struct.unpack('<I',struct.pack('<f',rng.choice([1.0,1.5,0.75,rng.uniform(0.1,4)])))[0],4)
        if fn((ctypes.c_uint32*5)(callee,argument,fail_at,int(with_models),cw),(ctypes.c_uint8*total)(*blob),total,out,res)!=1:raise RuntimeError('Native rejected')
        if res[5]:explicit+=1;continue
        e,er=source.run(callee,argument,fail_at,blob,fields,with_models,cw);cases+=1
        key=f"{callee:#x}:completed{er[0]}:calls{er[1]}";outcomes[key]=outcomes.get(key,0)+1
        got=bytes(out);diff=[(x,w) for x,w,o in fields if got[o:o+w]!=bytes(e[o:o+w])]
        status=[res[0],res[1]]!=er[:2] or (res[0] and callee in DEFINED_EAX and (not res[3] or res[2]!=er[2])) or (callee==0x42c4f0 and list(res)[8:15]!=er[3:10])
        if diff or status:bad.append(dict(case=n,callee=hex(callee),argument=argument,fail_at=fail_at,fields=[hex(x) for x,w in diff[:8]],native_status=list(res)[:5],source_status=er))
    names=['src/guest/game_mode_leaves_reference.hpp','src/guest/game_mode_leaves_reference.cpp','src/cpu/source_bytes.hpp','src/game/model_dlls.cpp','src/loader/executable_image.cpp','tests/probes/native_game_mode_leaves_probe.cpp','tools/oracles/verify_original_game_mode_leaves.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={hex(x):sha(image.read(x,0x40)) for x in LEAVES},cases=cases,native_explicit_boundaries=explicit,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,explicit=explicit,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:4]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
