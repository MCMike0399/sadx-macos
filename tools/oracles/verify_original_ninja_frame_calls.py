"""411d00's D3D8 frame thunks 781010 (78bab0: the real 780030 callback list and
hooks, then the real 7959a0 hook table), 781020 (78b960 EndScene path with the
real 77ec60), 781030 (vanilla 78ba30 Present: the native `original` policy),
781040 (78ba50 Clear, then the real 794800 on the nine 3d0fe00 records) and
781050 (78b880 BeginScene path), against original instructions. With
--loader, 781030 is also compared under the profile's policy against the pinned
mod loader's own Direct3D_Present_r (DLL 3293cf35 +0x50290: Present through
[100f167c] = 3d128b0, reset_device 10050300 on DEVICELOST, the empty
onRenderSceneStart vector) in a separate machine with a Present COM fixture.

The device is a COM fixture: BeginScene/EndScene/Present/Clear/SetRenderState/
GetRenderState stubs record their arguments (GetRenderState writes a case
value). Nested owners are recording fixtures that can stop the source: 7912e0,
793cc0 (ECX), 77dfc0 (four arguments), 789dd0 (Present's DEVICELOST tail jump),
the 411e20 input callback at 3d0fa34, callback/hook stubs, and a vertex-buffer
COM object whose Lock is 794800's nested owner. One fixture can rewrite 3ce712c
mid-list (as 77ffe0 would), proving 780030's per-iteration reread. Compared:
completion, 3d08588, 3d1289c, 3d128ac, 3ce712c, the 3d0fe00 records, the nested
trace (callee, site, ECX/arguments) and the device trace. Not covered:
reset_device itself (a nested owner), null/unknown devices
(native-only boundaries), and callback indices past the 8 modelled slots
(native index_out_of_range, counted as explicit).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;VB=DATA+0x800;VB_VTABLE=DATA+0x900
METHODS={0x88:(CODE+0x300,1,4),0x8c:(CODE+0x310,2,4),0x3c:(CODE+0x320,3,0x14),0x90:(CODE+0x330,4,0x1c),0xc8:(CODE+0x340,6,0xc),0xcc:(CODE+0x350,5,0xc)}
LOCK,UNLOCK=CODE+0x360,CODE+0x370
LOADER_SHA256='3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba'
STUBS=[CODE+0x600+0x10*k for k in range(12)] # callback/hook targets (each a RET)
FIXTURES=(0x7912e0,0x793cc0,0x77dfc0,0x789dd0,0x411e20)
THUNKS=[0x781010,0x781020,0x781030,0x781040,0x781050]
SITES=[0x780042,0x78005c,0x780067,0x780072,0x780084,0x7959ac,0x78b977,0x78b9e3,0x78b922,0x794826]
DEVICE_LOST=0x88760868
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--loader',type=pathlib.Path);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for off,(stub,kind,pop) in METHODS.items():m.mem_write(stub,(b'' if kind==3 else b'\x31\xc0')+b'\xc2'+struct.pack('<H',pop));m.mem_write(VTABLE+off,struct.pack('<I',stub)) # Present keeps the fixture EAX
    m.mem_write(DEVICE,struct.pack('<I',VTABLE))
    m.mem_write(LOCK,b'\x31\xc0\xc2\x14\x00');m.mem_write(UNLOCK,b'\x31\xc0\xc2\x04\x00');m.mem_write(VB_VTABLE+0x2c,struct.pack('<II',LOCK,UNLOCK));m.mem_write(VB,struct.pack('<I',VB_VTABLE))
    for stub in STUBS:m.mem_write(stub,b'\xc3')
    returns={}
    for site in SITES:
        op=image.read(site,2);returns[site+(5 if op[0]==0xe8 else 3 if op==b'\xff\x52' else 2)]=site
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def nested(mu,callee,site,xs):
        st['nested'].append([callee,site]+xs)
        if st['rewrite'] and len(st['nested'])==st['rewrite']:mu.mem_write(0x3ce712c,struct.pack('<I',st['rewrite_value']))
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return True
        return False
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);ret=word(mu,esp);D=0xdeadbeef
        for off,(stub,kind,_) in METHODS.items():
            if address!=stub:continue
            if kind==4:
                count,rects,flags,color,z,stencil=struct.unpack('<6I',mu.mem_read(esp+8,24))
                st['device'].append([4,flags,z] if count==0 and rects==0 else [99,count,rects]);st['device'].append([7,color,stencil])
            elif kind==3:
                args=struct.unpack('<4I',mu.mem_read(esp+8,16));st['device'].append([3,0,0] if args==(0,0,0,0) else [98,0,0]);mu.reg_write(UC_X86_REG_EAX,st['present'])
            elif kind in(5,6):
                state,value=struct.unpack('<II',mu.mem_read(esp+8,8))
                if kind==5:st['device'].append([5,state,0]);mu.mem_write(value,struct.pack('<I',st['get'][state]))
                else:st['device'].append([6,state,value])
            else:st['device'].append([kind,0,0])
            if kind!=3:mu.reg_write(UC_X86_REG_EAX,0)
            return
        if address==LOCK:
            nested(mu,0x794800,returns.get(ret,ret),[mu.reg_read(UC_X86_REG_ESI),D,D,D]);return
        if address in STUBS:
            xs=[word(mu,esp+4),D,D,D] if returns.get(ret)==0x780084 else [D,D,D,D]
            nested(mu,address,returns.get(ret,ret),xs);return
        if address==0x789dd0:
            if not nested(mu,0x789dd0,0x78ba4a,[D,D,D,D]):mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
            return
        if address==0x411e20:xs=[D,D,D,D]
        elif address==0x793cc0:xs=[mu.reg_read(UC_X86_REG_ECX),D,D,D]
        elif address==0x77dfc0:xs=list(struct.unpack('<4I',mu.mem_read(esp+4,16)))
        else:xs=[D,D,D,D]
        if not nested(mu,address,returns.get(ret,ret),xs):mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in [s for s,_,_ in METHODS.values()]+[LOCK]+STUBS+list(FIXTURES):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    blend=image.read(0x88ad64,88)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_frame_calls;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x78bab0);bad=[];outcomes={};cases=explicit=0
    for n in range(4000):
        thunk=THUNKS[n%5];c=[0]*80;c[0]=thunk
        c[1]=0 if rng.random()<0.75 else rng.randrange(1,6)
        c[2]=rng.choice([0,0,DEVICE_LOST,0x88760869,rng.getrandbits(32)])
        c[3]=rng.choice([0,1,2,3,rng.getrandbits(32)]);c[4]=rng.choice([0,1,0x100,rng.getrandbits(32)]);c[5]=rng.choice([0,0,0x1000,rng.getrandbits(32)])
        hook_or_zero=lambda q:rng.choice(STUBS) if rng.random()<q else 0
        c[6]=hook_or_zero(0.4);c[7]=hook_or_zero(0.4);c[8]=hook_or_zero(0.4);c[9]=rng.getrandbits(32);c[10]=0x411e20 if rng.random()<0.7 else 0
        c[11]=rng.choice([0,1,1,2,3,8]);
        for i in range(8):c[12+i]=rng.choice(STUBS)
        for i in range(16):c[20+i]=hook_or_zero(0.2)
        for r in range(9):
            c[36+4*r]=VB if rng.random()<0.35 else 0;c[37+4*r]=rng.getrandbits(32);c[38+4*r]=rng.getrandbits(32);c[39+4*r]=rng.choice([0,0x1000,rng.getrandbits(32)])
        c[72]=rng.getrandbits(32);c[73]=rng.choice([1,4,8,rng.getrandbits(32)]);c[74]=rng.getrandbits(8)
        c[75]=0 if rng.random()<0.7 else rng.randrange(1,4);c[76]=rng.choice([0,1,2,3,4,9])
        out=(ctypes.c_uint32*320)()
        if fn((ctypes.c_uint32*len(c))(*c),(ctypes.c_uint8*88).from_buffer_copy(blend),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        if got[1]==9: # native index_out_of_range: the source would read a slot past the modelled 8
            explicit+=1;outcomes['explicit:index_out_of_range']=outcomes.get('explicit:index_out_of_range',0)+1;continue
        m.context_restore(clean);st.update(device=[],nested=[],stopped=False,fail=c[1],present=c[2],get={0xe:c[72],0x17:c[73]},rewrite=c[75],rewrite_value=c[76])
        w=lambda at,v:m.mem_write(at,struct.pack('<I',v&0xffffffff))
        for at,k in((0x3d08588,3),(0x3d1289c,4),(0x3d128ac,5),(0x3d0f7a4,6),(0x3d0f7a8,7),(0x3d0fa30,8),(0x3d0fa38,9),(0x3d0fa34,10),(0x3ce712c,11)):w(at,c[k])
        for i in range(8):w(0x3d0fa40+4*i,c[12+i])
        for i in range(16):w(0x3d08af0+4*i,c[20+i])
        for r in range(9):
            for k in range(4):w(0x3d0fe00+0x24*r+4*k,c[36+4*r+k])
        m.mem_write(0x3ab9890,bytes([c[74]]));w(0x3d128b0,DEVICE)
        m.reg_write(UC_X86_REG_FPCW,0x7f);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(thunk,STOP,count=200000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n} {thunk:#x}: {err} at {m.reg_read(UC_X86_REG_EIP):#x} input {c[:12]}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: source return ABI')
        g=lambda at:word(m,at)
        e=[0]*320;e[0]=int(not st['stopped'])
        e[3]=g(0x3d08588);e[4]=g(0x3d1289c);e[5]=g(0x3d128ac);e[6]=g(0x3ce712c)
        for r in range(9):
            for k in range(4):e[36+4*r+k]=g(0x3d0fe00+0x24*r+4*k)
        e[80]=len(st['nested'])
        for i,t in enumerate(st['nested'][:16]):e[81+6*i:87+6*i]=t
        e[200]=len(st['device'])
        for i,t in enumerate(st['device'][:32]):e[201+3*i:204+3*i]=t
        got[1]=got[2]=0 # native failure kind/boundary have no source register equivalent; completion is compared
        cases+=1;key=f"{thunk:#x}:completed{e[0]}:nested{e[80]}:device{e[200]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(320) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,thunk=hex(thunk),fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    loader_cases=0
    if a.loader:
        ld=PEImage(a.loader)
        if ld.sha256!=LOADER_SHA256:raise RuntimeError('Pinned loader mismatch')
        L=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32)
        for s_ in ld.sections:
            base=ld.base+s_['rva'];size=(max(s_['virtual_size'],s_['raw_size'])+0xfff)&~0xfff
            L.mem_map(base,size,unicorn.UC_PROT_ALL);L.mem_write(base,ld.slice(s_['file_offset'],min(s_['raw_size'],size)))
        SCR=0x30000000;LDEV,LVT,LPRESENT,LSTOP,LSTACK=SCR+0x100,SCR+0x400,SCR+0x800,SCR+0x900,SCR+0xf000
        for base,size in((0x3d12000,0x1000),(SCR,0x10000)):L.mem_map(base,size,unicorn.UC_PROT_ALL)
        L.mem_write(LPRESENT,b'\xc2\x14\x00');L.mem_write(LVT+0x3c,struct.pack('<I',LPRESENT));L.mem_write(LDEV,struct.pack('<I',LVT));L.mem_write(0x3d128b0,struct.pack('<I',LDEV));L.mem_write(LSTOP,b'\xc3')
        lclean=L.context_save();ls={}
        def lhook(mu,address,size,user):
            esp=mu.reg_read(UC_X86_REG_ESP)
            if address==LPRESENT:
                args=struct.unpack('<4I',mu.mem_read(esp+8,16));ls['device'].append([3,0,0] if args==(0,0,0,0) and word(mu,esp+4)==LDEV else [98,0,0]);mu.reg_write(UC_X86_REG_EAX,ls['present']);return
            ret=word(mu,esp);ls['nested'].append([0x10050300,ret-5,0xdeadbeef,0xdeadbeef,0xdeadbeef,0xdeadbeef])
            if ls['fail'] and len(ls['nested'])==ls['fail']:ls['stopped']=True;mu.emu_stop();return
            mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
        for addr in(LPRESENT,0x10050300):L.hook_add(unicorn.UC_HOOK_CODE,lhook,begin=addr,end=addr)
        for n in range(800):
            c=[0]*80;c[0]=0x781030;c[77]=1;c[2]=rng.choice([0,DEVICE_LOST,DEVICE_LOST,0x88760869,rng.getrandbits(32)]);c[1]=0 if rng.random()<0.7 else 1
            out=(ctypes.c_uint32*320)()
            if fn((ctypes.c_uint32*len(c))(*c),(ctypes.c_uint8*88).from_buffer_copy(blend),out)!=1:raise RuntimeError('Native rejected')
            got=list(out);L.context_restore(lclean);ls.update(device=[],nested=[],stopped=False,fail=c[1],present=c[2])
            L.mem_write(LSTACK,struct.pack('<I',LSTOP));L.reg_write(UC_X86_REG_ESP,LSTACK)
            try:L.emu_start(0x10050290,LSTOP,count=10000)
            except unicorn.UcError as err:raise RuntimeError(f'loader case {n}: {err} at {L.reg_read(UC_X86_REG_EIP):#x}')
            if not ls['stopped'] and L.reg_read(UC_X86_REG_EIP)!=LSTOP:raise RuntimeError('loader return ABI')
            e=[0]*320;e[0]=int(not ls['stopped'])
            for k in(3,4,5,6):e[k]=got[k] # state words the loader function does not touch
            e[36:72]=got[36:72];e[80]=len(ls['nested'])
            for i,t in enumerate(ls['nested'][:16]):e[81+6*i:87+6*i]=t
            e[200]=len(ls['device'])
            for i,t in enumerate(ls['device'][:32]):e[201+3*i:204+3*i]=t
            got[1]=got[2]=0;loader_cases+=1;key=f"loader-present:completed{e[0]}:nested{e[80]}";outcomes[key]=outcomes.get(key,0)+1
            d=[i for i in range(320) if got[i]!=e[i]]
            if d:bad.append(dict(case=f'loader {n}',fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
        ranges_loader={'0x10050290-0x100502f0':sha(bytes(L.mem_read(0x10050290,0x60)))}
    else:ranges_loader={}
    names=['src/ninja/ninja_frame_calls.hpp','src/ninja/ninja_frame_calls.cpp','src/ninja/ninja_render.cpp','tests/probes/native_ninja_frame_calls_probe.cpp','tools/oracles/verify_original_ninja_frame_calls.py']
    ranges=((0x78bab0,0x78baba),(0x780030,0x780088),(0x7959a0,0x7959bb),(0x78b960,0x78ba23),(0x78ba30,0x78ba50),(0x78ba50,0x78baa4),(0x78b880,0x78b958),(0x794800,0x794835),(0x77ec60,0x77ecda),(0x781010,0x781065))
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={f'{x:#x}-{y:#x}':sha(image.read(x,y-x)) for x,y in ranges},cases=cases,loader_present_cases=loader_cases,loader_ranges_sha256=ranges_loader,explicit_native_boundaries=explicit,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,loader_cases=loader_cases,explicit=explicit,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
