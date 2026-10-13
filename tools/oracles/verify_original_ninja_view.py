"""781570 / 781250 / 791170 (the camera block, view matrix and projection WinMain sets at 402d4f..402d5e)
against their original instructions.

Every instruction of 781570, 781250 and 791170 runs, with all their callees from the original bytes:
788290/7883c0 (the 88af78 sine table), 7871f0, 787e30, 7716df, 770804 -> 774310 and 770d10 -> 7757e0
(dispatch slots 389b784 / 389b7c0 written as 77350c leaves them on an SSE CPU without 3DNow!; 77350c is
not executed). The D3D8 device is a fixture: [3d128b0] -> object -> vtable whose +0x94 (SetTransform)
and +0x9c (MultiplyTransform) are `ret 0xc` stubs that record (state, the 16 words passed). The matrix
stack is a fixture block of 1..4 slots at [3d0fa1c] with a random current depth. Mode 0 runs 781570
(thiscall, ECX = 3aad0a0) and then 781250; mode 1 runs 781250 on a random block.

Compared: 3d0f760 (view), matrix-stack slot 0, 3d0f818 (current = base) and 3d0fa2c (depth),
3d0f980, 3d0f9e0, 3d0f808, 3d0f968, 3d12960, 3d129c0, 389d398, 389d358 (sentinel when 7757e0
returns NULL), the block after 781570, the three device calls and the x87 control word.

Two profiles:
  primary  Unicorn executes FPTAN (791240), FPATAN (79124a) and FSINCOS (771706) itself: QEMU computes
           them with host doubles, the native side with the Bochs P5/P6 models.
  seeded   right after each of those instructions the result registers are overwritten with the native
           Bochs results (exported by the probe), so every other instruction is compared exactly.
           Seeding proves the transcription, not Intel FPTAN/FPATAN/FSINCOS (roadmap M4).
RCPSS/RCPPS are Unicorn's IEEE division in both the source and the native probe (not a real CPU's
estimate). MXCSR profiles vary the rounding mode only (Unicorn ignores FZ/DAZ). At most one NaN source per
case (Unicorn applies the x87 NaN rule to SSE operands). VIEW_SMOKE=1 in the environment stops each profile
after 300 cases (for mutant runs); the recorded report never uses it.
"""
import argparse,ctypes,hashlib,json,math,os,pathlib,random,struct
import unicorn
from unicorn.x86_const import (UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ECX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,
    UC_X86_REG_MXCSR,UC_X86_REG_FP0)
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA
sha=lambda b:hashlib.sha256(b).hexdigest()
fb=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
SLOTS=DATA+0x1000;DEV=DATA+0x2000;VTBL=DATA+0x2100;STUB_SET=0x10000800;STUB_MUL=0x10000810
SENT=0xa5a5a5a5
BLOCK=0x3aad0a0
# (address, words) compared after the run, in the probe's output order
FIELDS=[('view_3d0f760',0x3d0f760,16),('copy_3d0f980',0x3d0f980,16),('scale_3d0f9e0',0x3d0f9e0,1),('near_3d0f808',0x3d0f808,1),
        ('far_3d0f968',0x3d0f968,1),('perspective_3d12960',0x3d12960,16),('projection_3d129c0',0x3d129c0,16),
        ('view_389d398',0x389d398,16),('inverse_389d358',0x389d358,16)]
OUT_AT={'view_3d0f760':3,'copy_3d0f980':35,'scale_3d0f9e0':51,'near_3d0f808':52,'far_3d0f968':53,'perspective_3d12960':54,
        'projection_3d129c0':70,'view_389d398':86,'inverse_389d358':102}
def cases(rng):
    for n in range(3000):
        cw=rng.choice([0x27f,0x27f,0x7f,0x37f,0x67f,0xa7f,0xe7f]);mxcsr=rng.choice([0x1f80,0x1f80,0x3f80,0x5f80,0x7f80])
        mode=0 if n%8==0 else 1
        pos=[fb(rng.uniform(-5000,5000)) for _ in range(3)]
        k=n%11
        if k==0:d=[0.0,rng.choice([1.0,-1.0,0.5]),0.0]                 # vertical: x^2+z^2 = 0
        elif k==1:d=[rng.uniform(-5e-4,5e-4),rng.uniform(-1,1),rng.uniform(-5e-4,5e-4)]  # around the 1e-6 threshold
        elif k==2:d=[0.0,0.0,-1.0]
        elif k==3:  # x^2 + z^2 rounds to exactly the 7dce54 threshold (FCOMP equal: the vertical branch)
            sx,sz=rng.choice([1,-1]),rng.choice([1,-1]);fl=lambda b:struct.unpack('<f',struct.pack('<I',b))[0]
            d=[sx*fl(0x3a395ce6),rng.uniform(-1,1),sz*fl(0x3a395d5e)]
        else:
            s=10**rng.uniform(-2,2);d=[rng.uniform(-1,1)*s for _ in range(3)]
        roll=rng.choice([0,0,rng.randrange(-0x10000,0x10000),rng.getrandbits(32)])&0xffffffff
        angle=rng.choice([0x2aaa,0x31c7,0x1555,rng.randrange(0x400,0x7000),-rng.randrange(1,0x7000),rng.randrange(1,0x8000)|1])&0xffffffff
        near=fb(-rng.uniform(0.1,10));far=fb(-rng.choice([60000.0,rng.uniform(100,100000)]))
        w,h=rng.choice([(640,480),(1280,720),(320,240),(rng.randrange(1,4096),rng.randrange(1,4096))])
        viewport=[rng.randrange(0,64),rng.randrange(0,64),w,h,fb(0.0),fb(1.0)]
        screen=[fb(rng.uniform(-2,2)) for _ in range(16)]
        if n%37==0:screen=[fb(1.0) if i%5==0 else 0 for i in range(16)]
        slots=rng.randrange(1,5);depth=rng.randrange(1,slots+1)
        block=pos+[fb(x) for x in d]+[roll,angle]
        if n%53==0:block[rng.randrange(6)]=rng.choice([0x7fc00000,0xff800000,0x80000000])
        yield [cw,mxcsr,mode]+block+[near,far]+viewport+screen+[slots,depth]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_write(0x389b784,struct.pack('<I',0x774310));m.mem_write(0x389b7c0,struct.pack('<I',0x7757e0))
    m.mem_write(STUB_SET,b'\xc2\x0c\x00');m.mem_write(STUB_MUL,b'\xc2\x0c\x00')
    m.mem_write(0x3d128b0,struct.pack('<I',DEV));m.mem_write(DEV,struct.pack('<I',VTBL))
    m.mem_write(VTBL,bytes(0x100));m.mem_write(VTBL+0x94,struct.pack('<I',STUB_SET));m.mem_write(VTBL+0x9c,struct.pack('<I',STUB_MUL))
    table=struct.unpack('<1025f',image.read(0x88af78,4100))
    lib=ctypes.CDLL(str(a.native_library.resolve()))
    fn=lib.sadx_native_ninja_view;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_float),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    x87=lib.sadx_native_ninja_view_x87;x87.argtypes=[ctypes.c_int,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint64,ctypes.c_uint32,ctypes.c_uint64,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint64)];x87.restype=ctypes.c_int
    ctable=(ctypes.c_float*1025)(*table)
    calls=[];seed={'on':False,'cw':0,'pending':None}
    def st(i):
        top=(m.reg_read(UC_X86_REG_FPSW)>>11)&7;return UC_X86_REG_FP0+((top+i)&7)
    def stub(uc,address,size,user):
        esp=uc.reg_read(UC_X86_REG_ESP);state,ptr=struct.unpack('<II',uc.mem_read(esp+8,8))
        calls.append((0x94 if address==STUB_SET else 0x9c,state,list(struct.unpack('<16I',uc.mem_read(ptr,64)))))
    m.hook_add(unicorn.UC_HOOK_CODE,stub,begin=STUB_SET,end=STUB_MUL)
    def seeding(uc,address,size,user):
        if not seed['on']:return
        out=(ctypes.c_uint32*3)();sig=(ctypes.c_uint64*2)()
        if address in(0x791240,0x79124a,0x771706):
            a0=uc.reg_read(st(0));a1=uc.reg_read(st(1))
            if address==0x791240:ok=x87(0,seed['cw'],a0[1],a0[0],0,0,out,sig)
            elif address==0x79124a:ok=x87(1,seed['cw'],a1[1],a1[0],a0[1],a0[0],out,sig)
            else:ok=x87(2,seed['cw'],a0[1],a0[0],0,0,out,sig)
            seed['pending']=(address,list(out),list(sig)) if ok and out[0] else None
            return
        if not seed['pending']:return
        at,out,sig=seed['pending'];seed['pending']=None
        if at==0x791240 and address==0x791242:uc.reg_write(st(1),(sig[0],out[1]))          # ST1 = tan, ST0 = 1.0
        elif at==0x79124a and address==0x79124c:uc.reg_write(st(0),(sig[0],out[1]))        # ST0 = atan
        elif at==0x771706 and address==0x771708:uc.reg_write(st(0),(sig[1],out[2]));uc.reg_write(st(1),(sig[0],out[1]))
    for at in(0x791240,0x791242,0x79124a,0x79124c,0x771706,0x771708):m.hook_add(unicorn.UC_HOOK_CODE,seeding,begin=at,end=at)
    clean=m.context_save()
    def original(c,seeded):
        m.context_restore(clean);calls.clear();seed.update(on=seeded,cw=c[0],pending=None)
        for _,at,n in FIELDS:m.mem_write(at,struct.pack('<I',SENT)*n)
        slots,depth=c[35],c[36]
        m.mem_write(SLOTS,struct.pack('<I',SENT)*64)
        m.mem_write(0x3d0fa1c,struct.pack('<I',SLOTS));m.mem_write(0x3d0f818,struct.pack('<I',SLOTS+64*(depth-1)));m.mem_write(0x3d0fa2c,struct.pack('<I',depth))
        m.mem_write(BLOCK,struct.pack('<10I',*c[3:13])+struct.pack('<I',SENT)*6)
        m.mem_write(0x3d12780,struct.pack('<6I',*c[13:19]));m.mem_write(0x3d0fd80,struct.pack('<16I',*c[19:35]))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff);m.reg_write(UC_X86_REG_MXCSR,c[1])
        if c[2]==0:
            m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,BLOCK)
            m.emu_start(0x781570,STOP,count=100)
            if m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('781570 ABI')
        m.mem_write(STACK,struct.pack('<2I',STOP,BLOCK));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x781250,STOP,count=200000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('781250 did not return (cdecl)')
        o={name:list(struct.unpack(f'<{n}I',m.mem_read(at,4*n))) for name,at,n in FIELDS}
        o['slot0']=list(struct.unpack('<16I',m.mem_read(SLOTS,64)))
        o['current_depth']=[struct.unpack('<I',m.mem_read(0x3d0f818,4))[0]==SLOTS,struct.unpack('<I',m.mem_read(0x3d0fa2c,4))[0]]
        o['calls']=[[off,state]+words for off,state,words in calls]
        o['block']=list(struct.unpack('<16I',m.mem_read(BLOCK,64)))
        o['cw']=m.reg_read(UC_X86_REG_FPCW)
        return o
    def native(c):
        out=(ctypes.c_uint32*200)()
        if not fn((ctypes.c_uint32*37)(*c),ctable,out):return None
        o={name:list(out[OUT_AT[name]:OUT_AT[name]+n]) for name,_,n in FIELDS}
        o['slot0']=list(out[19:35]);o['current_depth']=[True,out[118]]
        o['calls']=[list(out[120+18*k:138+18*k]) for k in range(min(out[119],3))]
        o['block']=list(out[175:183])+[out[189],out[190]]+list(out[183:189])
        o['cw']=out[174];o['failure']=(out[0],out[1])
        return o
    profiles={}
    for profile in('primary','seeded'):
        row=dict(cases=0,native_failures=0,value_mismatches=0,field_mismatches={},examples=[])
        for c in cases(random.Random(0x781250)):
            if os.environ.get('VIEW_SMOKE') and row['cases']>=300:break  # quick runs for tools/gate/mutate.py
            n=native(c)
            if n is None or n['failure'][0]:row['native_failures']+=1;row['examples'].append(dict(input=[hex(x) for x in c],native_failure=n and n['failure']));continue
            o=original(c,profile=='seeded');row['cases']+=1
            if c[2]==1:o['block']=n['block']=None
            bad=[k for k in o if o[k]!=n[k]]
            if bad:
                row['value_mismatches']+=1
                for k in bad:row['field_mismatches'][k]=row['field_mismatches'].get(k,0)+1
                if len(row['examples'])<10:row['examples'].append(dict(input=[hex(x) for x in c],fields={k:dict(source=[hex(x) if isinstance(x,int) else x for x in (o[k] if isinstance(o[k],list) else [o[k]])][:18],native=[hex(x) if isinstance(x,int) else x for x in (n[k] if isinstance(n[k],list) else [n[k]])][:18]) for k in bad if k!='calls'}))
        profiles[profile]=row;print(json.dumps({k:v for k,v in row.items() if k!='examples'}),flush=True)
    root=pathlib.Path(__file__).resolve().parents[2]
    names=['src/engine/ninja_view.hpp','src/engine/ninja_view.cpp','src/math/d3dx_math.cpp','src/math/ninja_math.cpp','src/cpu/x87_transcendental.cpp','tests/probes/native_ninja_view_probe.cpp','tools/oracles/verify_original_ninja_view.py']
    pr,se=profiles['primary'],profiles['seeded']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((root/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x781570-0x7815b4':sha(image.read(0x781570,0x44)),'0x781250-0x781565':sha(image.read(0x781250,0x315)),'0x791170-0x7912d8':sha(image.read(0x791170,0x168)),
                                '0x787e30-0x787e4b':sha(image.read(0x787e30,0x1b)),'0x7716df-0x771773':sha(image.read(0x7716df,0x94))},
        dispatch_seed={'0x389b784':'0x774310','0x389b7c0':'0x7757e0'},profiles=profiles,
        seeded_values_match=se['value_mismatches']==0 and se['native_failures']==0 and se['cases']>0,
        primary_values_with_unicorn_transcendentals_match=pr['value_mismatches']==0,
        limits=[__doc__.strip(),
                'primary value mismatches come from FPTAN/FPATAN/FSINCOS: Unicorn uses host doubles, the native Bochs models are not changed to match; settling them needs the Intel capture (roadmap M4). The report stays fail while they exist; the seeded profile is the transcription check',
                'x87 and MXCSR exception flags are not compared (Unicorn 2.1.4 raises few x87 and no SSE flags)'])
    a.output.write_text(json.dumps(report,indent=1)+'\n')
    return int(not report['seeded_values_match'])
if __name__=='__main__':raise SystemExit(main())
