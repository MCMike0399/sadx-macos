"""781070 "Sonic Library Initialize" against its original instructions.

The whole PE is mapped; the 892940 config block and the viewport words
3d12788/3d1278c are case inputs. Every callee is a recording fixture (hooked at
entry; 793ef0's local rect is recorded by value; 788000 returns a scripted
result per try) and any nested call can stop the source; the 78c5a0 calls run
(RET). Compared: completion, the 788000 tries left, 389d864..389d884,
38a5dc4/38a5dc8/38a5dcc, 3d0f800, 3d0f9c0 (x87 under the case's control word)
and the nested trace (callee, site, arguments). Not covered: the callees.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
CALLEES=(0x794050,0x793ef0,0x789f80,0x77ff40,0x793d80,0x780f20,0x780c30,0x77f150,0x793e70,0x788000,0x7881a0,0x793e80,0x77f220,0x780e70,0x780f40,0x793d90,0x789d30,0x789c50)
ARGS={0x77ff40:1,0x780f20:2,0x780c30:3,0x77f150:3,0x788000:4}
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);ret=word(mu,esp);D=0xdeadbeef
        if address==0x793ef0:p_=word(mu,esp+4);xs=[word(mu,p_+4*k) for k in range(4)]
        else:n=ARGS.get(address,0);xs=[word(mu,esp+4+4*k) for k in range(n)]+[D]*(4-n)
        st['nested'].append([address,ret-5]+xs)
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        if address==0x788000:mu.reg_write(UC_X86_REG_EAX,st['results'][min(st['tries'],7)]);st['tries']+=1
        mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in CALLEES:m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_library_init;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    config=list(struct.unpack('<23I',image.read(0x892940,0x5c)))
    rng=random.Random(0x781070);bad=[];outcomes={};cases=0
    for n in range(2400):
        c=[0]*49;c[0]=0 if rng.random()<0.75 else rng.randrange(1,30);c[1]=rng.choice([0x7f,0x27f,0x37f,0xc7f,0x47f,0x87f])
        for i in range(23):c[2+i]=config[i] if rng.random()<0.6 else rng.getrandbits(32)
        c[2+0x4c//4]=rng.choice([0,1,1,2])
        c[25]=rng.choice([640,1280,0,0x80000000,0xffffffff,rng.getrandbits(32)]);c[26]=rng.choice([480,720,0,0x80000001,rng.getrandbits(32)])
        for i in range(8):c[27+i]=rng.choice([0,0,0xffffffe9,0xffffffdf,1,0x80000000])
        for i in range(14):c[35+i]=rng.getrandbits(32)
        out=(ctypes.c_uint32*300)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(nested=[],stopped=False,fail=c[0],results=c[27:35],tries=0)
        m.mem_write(0x892940,struct.pack('<23I',*c[2:25]));m.mem_write(0x3d12788,struct.pack('<II',c[25],c[26]))
        m.mem_write(0x389d864,struct.pack('<9I',*c[35:44]));m.mem_write(0x38a5dc4,struct.pack('<3I',*c[44:47]));m.mem_write(0x3d0f800,struct.pack('<I',c[47]));m.mem_write(0x3d0f9c0,struct.pack('<I',c[48]))
        m.reg_write(UC_X86_REG_FPCW,c[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<II',STOP,0x892940));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x781070,STOP,timeout=1000000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        g=lambda at:word(m,at);e=[0]*300;e[0]=int(not st['stopped'])
        for i in range(9):e[2+i]=g(0x389d864+4*i)
        e[11]=g(0x38a5dc4);e[12]=g(0x38a5dc8);e[13]=g(0x38a5dcc);e[14]=g(0x3d0f800);e[15]=g(0x3d0f9c0)
        e[20]=len(st['nested'])
        for i,t in enumerate(st['nested'][:40]):e[21+6*i:27+6*i]=t
        if e[0]:e[1]=got[1] # EBX is popped before return; tries are compared through the nested trace
        got[1]=e[1]
        cases+=1;key=f"completed{e[0]}:nested{e[20]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(300) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    root = pathlib.Path(__file__).resolve().parents[2]
    names = ['tests/probes/native_library_init_probe.cpp',
             'tools/oracles/verify_original_library_init.py', 'src/cpu/fidelity_float.hpp']
    for pattern in ('src/game/game_library_init*', 'src/guest/game_library_init_reference*'):
        names.extend(str(path.relative_to(root)) for path in sorted(root.glob(pattern))
                     if path.is_file())
    names = sorted(set(names))
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x781070-0x781243':sha(image.read(0x781070,0x1d3))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
