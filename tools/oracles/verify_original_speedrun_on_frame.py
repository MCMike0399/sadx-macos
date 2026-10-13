"""Speedrun 2.0 OnFrame (sadx-speedrun-mod.dll +0x2e60), which the pinned mod
loader's ProcessCodes (426063) raises each 40c090 pass, against its original
instructions.

The pinned DLL runs at its preferred base. sonic.exe globals 3b22de4 (u16),
3abdc7c and 3b22dcc (u16) are mapped data. Nested owners are recording
fixtures that can stop the source: 10004f80 (quick save, entered by CALL) and
the Chao stat viewer body at 10002e7f (the fixture resumes at its stack-balanced
end 10002f4f). Compared: completion, the five DLL bytes 100107e8/100107e9 and
the 10010818..1a timer, and the nested trace. Not covered: the viewer body and
the quick-save routine themselves (unported nested owners).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn import UC_PROT_ALL,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
sha=lambda b:hashlib.sha256(b).hexdigest()
SPEEDRUN_SHA256='f3fc68a3058fb726142547018da860b170f137248e54b82712b52527e7b74802'
STOP=0x20000000;STACK=0x21000000+0xf000;ON_FRAME=0x10002e60
BYTES=[0x100107e9,0x100107e8,0x10010818,0x10010819,0x1001081a]
def main():
    p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    dll=PEImage(a.dll)
    if dll.sha256!=SPEEDRUN_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned DLL/Unicorn mismatch')
    m=unicorn.Uc(UC_ARCH_X86,UC_MODE_32)
    for s in dll.sections:
        base=dll.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,dll.slice(s['file_offset'],min(s['raw_size'],size)))
    for base,size in((STOP,0x1000),(0x21000000,0x10000),(0x3b22000,0x1000),(0x3abd000,0x1000)):m.mem_map(base,size,UC_PROT_ALL)
    m.mem_write(STOP,b'\xc3');clean=m.context_save();st={}
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==0x10004f80:st['nested'].append([0x10004f80,struct.unpack('<I',mu.mem_read(esp,4))[0]-5])
        else:st['nested'].append([0x10002e7f,0x10002e7f])
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        if address==0x10004f80:ret=struct.unpack('<I',mu.mem_read(esp,4))[0];mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
        else:mu.reg_write(UC_X86_REG_EIP,0x10002f4f)
    for addr in(0x10004f80,0x10002e7f):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_speedrun_on_frame;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x2e60);bad=[];outcomes={};cases=0
    edge=[0,1,0x3b,0x3c,0x3d,0x62,0x63,0x64,0x7f,0x80,0xbb,0xff]
    for n in range(6000):
        c=[rng.choice([0,0,1,0x80]),rng.choice([0,0,1,0xff]),rng.choice(edge+[rng.getrandbits(8)]),rng.choice(edge+[rng.getrandbits(8)]),rng.choice(edge+[rng.getrandbits(8)]),
           rng.choice([7,4,9,0,0x107,rng.getrandbits(16)]),rng.choice([4,9,4,9,0,5,0x104,0xffffffff,rng.getrandbits(32)]),rng.choice([0x27,0,0x127,rng.getrandbits(16)]),0 if rng.random()<0.8 else rng.randrange(1,3)]
        out=(ctypes.c_uint32*16)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(nested=[],stopped=False,fail=c[8])
        for i,at in enumerate(BYTES):m.mem_write(at,bytes([c[i]]))
        m.mem_write(0x3b22de4,struct.pack('<H',c[5]));m.mem_write(0x3abdc7c,struct.pack('<I',c[6]));m.mem_write(0x3b22dcc,struct.pack('<H',c[7]))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(ON_FRAME,STOP,count=10000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x} input {c}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        e=[0]*16;e[0]=int(not st['stopped'])
        for i,at in enumerate(BYTES):e[1+i]=m.mem_read(at,1)[0]
        e[6]=len(st['nested'])
        for i,t in enumerate(st['nested'][:2]):e[7+2*i:9+2*i]=t
        cases+=1;key=f"completed{e[0]}:nested{e[6]}:timer{'reset' if e[3:6]==[0,0,0] else 'kept'}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(16) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,input=c,fields=d,source=[hex(e[i]) for i in d],native=[hex(got[i]) for i in d]))
    names=['src/game/mod_loader_frame.hpp','src/game/mod_loader_frame.cpp','tests/probes/native_speedrun_on_frame_probe.cpp','tools/oracles/verify_original_speedrun_on_frame.py']
    report=dict(dll_sha256=dll.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256={'0x10002e60-0x10002fe2':sha(bytes(m.mem_read(0x10002e60,0x182)))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
