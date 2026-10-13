"""The pinned Input mod's pdVibMxStop_r (sadx-input-mod.dll +0x8d80, the Jumps
replacement for 785330) against its own instructions.

The DLL runs at its preferred base. The rumble task pairs (1000ec60..) and each
DreamPad's rumble flags (+0x3f) and word (+0x40) are case inputs; the pad's SDL
controller word (+0x34) is its index. The game's 4b4160 and the
SDL_GameControllerRumble pointer at 1000e35c are recording fixtures; one call
can stop the source. Compared: completion, the 16 task slots, the flags and
words, and the call trace.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn import UC_PROT_ALL,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
sha=lambda b:hashlib.sha256(b).hexdigest()
INPUT_SHA256='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5'
STOP=0x20000000;STACK=0x21000000+0xf000;SDL=0x20000100;FREE=0x4b4160
def main():
    p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    dll=PEImage(a.dll)
    if dll.sha256!=INPUT_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned DLL/Unicorn mismatch')
    m=unicorn.Uc(UC_ARCH_X86,UC_MODE_32)
    for s in dll.sections:
        base=dll.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size,UC_PROT_ALL);m.mem_write(base,dll.slice(s['file_offset'],min(s['raw_size'],size)))
    m.mem_map(STOP,0x1000,UC_PROT_ALL);m.mem_map(0x21000000,0x10000,UC_PROT_ALL);m.mem_map(0x4b4000,0x1000,UC_PROT_ALL)
    m.mem_write(STOP,b'\xc3');m.mem_write(SDL,b'\xc3');m.mem_write(FREE,b'\xc3');m.mem_write(0x1000e35c,struct.pack('<I',SDL))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==FREE:st['calls']+=[1,word(mu,esp+4)]
        else:st['calls']+=[2,word(mu,esp+4)]
        if st['fail'] and len(st['calls'])//2==st['fail']:st['stopped']=True;mu.emu_stop()
    for addr in(FREE,SDL):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_vibration_stop;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x10008d80);bad=[];outcomes={};cases=0
    for n in range(3000):
        c=[rng.choice([0,0,0,rng.getrandbits(32)]) for _ in range(16)]+[rng.choice([0,0,1,2,3,0xfc,0xff]) for _ in range(8)]+[rng.getrandbits(32) for _ in range(8)]+[0 if rng.random()<0.8 else rng.randrange(1,6)]
        out=(ctypes.c_uint32*80)()
        if fn((ctypes.c_uint32*33)(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(calls=[],stopped=False,fail=c[32])
        m.mem_write(0x1000ec60,struct.pack('<16I',*c[:16]))
        for i in range(8):
            pad=0x1000e950+0x60*i;m.mem_write(pad+0x34,struct.pack('<I',i));m.mem_write(pad+0x3f,bytes([c[16+i]]));m.mem_write(pad+0x40,struct.pack('<I',c[24+i]))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x10008d80,STOP,count=10000)
        e=[0]*80;e[0]=int(not st['stopped'])
        for i in range(16):e[1+i]=word(m,0x1000ec60+4*i)
        for i in range(8):pad=0x1000e950+0x60*i;e[17+i]=m.mem_read(pad+0x3f,1)[0];e[25+i]=word(m,pad+0x40)
        e[40]=len(st['calls'])//2
        for i,v in enumerate(st['calls'][:38]):e[41+i]=v
        cases+=1;key=f"completed{e[0]}:calls{e[40]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(80) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:10],source=[hex(e[i]) for i in d[:10]],native=[hex(got[i]) for i in d[:10]]))
    names=['src/input/input_mod_pad.hpp','src/input/input_mod_pad.cpp','tests/probes/native_vibration_stop_probe.cpp','tools/oracles/verify_original_vibration_stop.py']
    report=dict(dll_sha256=dll.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256={'0x10008d80-0x10008df9':sha(bytes(m.mem_read(0x10008d80,0x79)))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
