"""Audio stream-player setup 40d0d0 (reached from 40c030 through the 4253c0
thunk, with 40ff10) against original instructions. Operator new 6443e7 is a
fixture that stops the source (the native port reports it as its unported
nested owner); compared: completion, the stopping call site and the player and
flag words 3abdf94/9c/a0/a4/98/a8.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
WORDS=(0x3abdf94,0x3abdf9c,0x3abdfa0,0x3abdfa4,0x3abdf98,0x3abdfa8)
def get(m,a):return struct.unpack('<I',m.mem_read(a,4))[0]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x37f).machine
    for page in(0x3abd000,0x3a7c000,0x38f6000,0x3b0e000):m.mem_map(page,0x1000)
    m.mem_map(0,0x1000) # fs:0 SEH chain (fs base 0)
    clean=m.context_save();state={}
    def new(mu,address,size,user):
        state['site']=struct.unpack('<I',mu.mem_read(mu.reg_read(UC_X86_REG_ESP),4))[0]-5;mu.emu_stop()
    m.hook_add(unicorn.UC_HOOK_CODE,new,begin=0x6443e7,end=0x6443e7)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_stream_players;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    rng=random.Random(0x40d0d0);bad=[];outcomes={};cases=[]
    for n in range(600):cases.append([rng.choice([0,0,1,rng.getrandbits(32)]),rng.choice([0,1,rng.getrandbits(32)]),rng.choice([0,1,rng.getrandbits(32)])]+[rng.getrandbits(32) for _ in range(6)])
    for c in cases:
        m.context_restore(clean);state.clear()
        m.mem_write(0x3b0ef2c,struct.pack('<I',c[0]));m.mem_write(0x3a7cf3c,struct.pack('<I',c[1]));m.mem_write(0x38f6ec0,struct.pack('<I',c[2]))
        for w,v in zip(WORDS,c[3:]):m.mem_write(w,struct.pack('<I',v))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x4253c0,STOP,count=10000)
        completed='site' not in state
        if completed and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        e=[int(completed),state.get('site',0)]+[get(m,w) for w in WORDS]
        out=(ctypes.c_uint32*10)()
        if fn((ctypes.c_uint32*9)(*c),9,out,10)!=1:raise RuntimeError('Native rejected')
        got=list(out)[:8];key=f"completed{e[0]}:site{e[1]:#x}";outcomes[key]=outcomes.get(key,0)+1
        if got!=e:bad.append(dict(input=[hex(x) for x in c],source=[hex(x) for x in e],native=[hex(x) for x in got]))
    names=['src/game/game_reset_leaves.hpp','src/game/game_reset_leaves.cpp','tests/probes/native_stream_players_probe.cpp','tools/oracles/verify_original_stream_players.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256=sha(image.read(0x40d0d0,0x11b)+image.read(0x4253c0,5)+image.read(0x40ff10,6)),cases=len(cases),source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],stores_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),outcomes=outcomes,mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
