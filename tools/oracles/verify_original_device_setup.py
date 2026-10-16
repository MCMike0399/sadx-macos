"""791610 (device setup) against its original instructions, with the real
7899a0 frame-multiplier code.

The whole PE is mapped. Direct3DCreate8 is an IAT fixture (7db27c) returning
the case's IDirect3D8 value; the device is a COM fixture (GetDeviceCaps +0x1c
writes the case's 0xd4 caps bytes, Clear +0x90 records its flags, GetViewport
+0xa4 writes the case's viewport). Nested owners 789dd0, 7943c0 (installs the
COM device in 3d128b0 or leaves it null, per case), 794ec0 (leaves a case value
in 3d1289c), 78d0d0, 796710, 791860, 7943f0 and 7914b0 (the tail jump) are
recording fixtures that can stop the source. Compared: completion, 3d11f60,
3d1289c, the nine record heads, 3d0fae0..3d0fcdf zeroed, caps, viewport,
3d12948/3d1294c, 3d0fd00..3d0fd18, the 3d0fd80 matrix copy, 3d0fd20..3d0fd2c,
389d7dc/3d084a8/3d084ac, whether 3d128b0 holds a device, the nested trace and
the device-method trace. A null device faults at the vtable load (page 0 is
unmapped), as the native owner stops (null_device).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;CREATE=CODE+0x300;METHODS={0x1c:CODE+0x310,0x90:CODE+0x320,0xa4:CODE+0x330}
NESTED=(0x789dd0,0x7943c0,0x794ec0,0x78d0d0,0x796710,0x791860,0x7943f0,0x7914b0)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_write(CREATE,b'\xc2\x04\x00');m.mem_write(0x7db27c,struct.pack('<I',CREATE))
    pops={0x1c:8,0x90:0x1c,0xa4:8}
    for off,stub in METHODS.items():m.mem_write(stub,b'\x31\xc0\xc2'+struct.pack('<H',pops[off]));m.mem_write(VTABLE+off,struct.pack('<I',stub))
    m.mem_write(DEVICE,struct.pack('<I',VTABLE))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);c=st['case']
        if address==CREATE:st['device'].append([0xdc,0]);mu.reg_write(UC_X86_REG_EAX,c[2]);return
        if address==METHODS[0x1c]:st['device'].append([0x1c,0]);mu.mem_write(word(mu,esp+8),struct.pack('<53I',*c[13:66]));return
        if address==METHODS[0x90]:st['device'].append([0x90,word(mu,esp+16)]);return
        if address==METHODS[0xa4]:st['device'].append([0xa4,0]);mu.mem_write(word(mu,esp+8),struct.pack('<6I',*c[4:10]));return
        ret=word(mu,esp);site=ret-5
        if address==0x7914b0:site=0x7917dc # tail jump
        st['nested'].append([address,site])
        if address==0x7943c0 and c[3]:mu.mem_write(0x3d128b0,struct.pack('<I',DEVICE))
        if address==0x794ec0:mu.mem_write(0x3d1289c,struct.pack('<I',c[82]))
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in list(NESTED)+[CREATE]+list(METHODS.values()):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_device_setup;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    matrix=list(struct.unpack('<16I',image.read(0x389d318,64)))
    rng=random.Random(0x791610);bad=[];outcomes={};cases=0
    for n in range(2000):
        c=[0]*83;c[0]=0 if rng.random()<0.75 else rng.randrange(1,10);c[1]=rng.choice([0x7f,0x27f,0x37f,0xc7f,0x47f])
        c[2]=rng.choice([0x12340000,0x12340000,0]);c[3]=int(rng.random()<0.9)
        c[4]=rng.choice([0,0,10,0x80000000]);c[5]=rng.choice([0,0,20]);c[6]=rng.choice([640,1280,0,0x80000000,0xffffffff,rng.getrandbits(32)]);c[7]=rng.choice([480,720,0,1,0x80000001,rng.getrandbits(32)])
        c[8]=0;c[9]=0x3f800000;c[10]=rng.getrandbits(32);c[11]=rng.choice([10000000,3579545,0,rng.getrandbits(32)]);c[12]=rng.choice([0,0,1,rng.getrandbits(32)])
        for i in range(53):c[13+i]=rng.getrandbits(32)
        for i in range(16):c[66+i]=matrix[i] if rng.random()<0.8 else rng.getrandbits(32)
        c[82]=rng.choice([0,1,0x55,rng.getrandbits(32)])
        out=(ctypes.c_uint32*200)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(case=c,nested=[],device=[],stopped=False,fail=c[0])
        m.mem_write(0x3d1289c,struct.pack('<I',c[10]))
        for r_ in range(9):m.mem_write(0x3d0fe00+0x24*r_,struct.pack('<4I',1,2,3,4))
        m.mem_write(0x3d0fae0,b'\x55'*0x200);m.mem_write(0x389d318,struct.pack('<16I',*c[66:82]));m.mem_write(0x3d08538,struct.pack('<II',c[11],c[12]))
        m.mem_write(0x3d128b0,struct.pack('<I',DEVICE))
        for at,n_ in((0x3d11f60,4),(0x3d127c0,0xd4),(0x3d12780,24),(0x3d12948,8),(0x3d0fd00,28),(0x3d0fd80,64),(0x3d0fd20,16),(0x389d7dc,4),(0x3d084a8,8)):m.mem_write(at,bytes(n_))
        m.reg_write(UC_X86_REG_FPCW,c[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        faulted=False
        try:m.emu_start(0x791610,STOP,count=200000)
        except unicorn.UcError:faulted=True # a null device: [eax] with eax = 0 reads page 0 (vtable 0), then the call faults
        if not faulted and not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        g=lambda at:word(m,at);e=[0]*200;e[0]=int(not st['stopped'] and not faulted)
        e[1]=g(0x3d11f60);e[2]=g(0x3d1289c)
        for i in range(9):e[3+i]=g(0x3d0fe00+0x24*i)
        for i in range(6):e[12+i]=g(0x3d12780+4*i)
        e[18]=g(0x3d12948);e[19]=g(0x3d1294c)
        for i in range(7):e[20+i]=g(0x3d0fd00+4*i)
        for i in range(16):e[27+i]=g(0x3d0fd80+4*i)
        for i in range(4):e[43+i]=g(0x3d0fd20+4*i)
        e[47]=g(0x389d7dc);e[48]=g(0x3d084a8);e[49]=g(0x3d084ac);e[50]=int(g(0x3d128b0)!=0)
        e[60]=len(st['nested'])
        for i,t in enumerate(st['nested'][:16]):e[61+2*i:63+2*i]=t
        e[100]=len(st['device'])
        for i,t in enumerate(st['device'][:8]):e[101+2*i:103+2*i]=t
        for i in range(53):e[120+i]=g(0x3d127c0+4*i)
        e[190]=int(bytes(m.mem_read(0x3d0fae0,0x200))==bytes(0x200))
        cases+=1;key=f"completed{e[0]}:faulted{int(faulted)}:nested{e[60]}:device{e[100]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(200) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/engine/ninja_device_init.hpp','src/guest/ninja_device_init.cpp','src/engine/game_frame_control.cpp','tests/probes/native_device_setup_probe.cpp','tools/oracles/verify_original_device_setup.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x791610-0x7917e1':sha(image.read(0x791610,0x1d1)),'0x7899a0':sha(image.read(0x7899a0,0x40))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
