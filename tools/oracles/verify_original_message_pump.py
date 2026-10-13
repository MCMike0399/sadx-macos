"""The 789bd0 message pump against its original instructions.

PeekMessageA (IAT 7db1ec) pops the case's scripted queue into the caller's MSG
(PM_REMOVE) or returns 0 without writing; TranslateMessage (7db1fc) and
DispatchMessageA (7db1f8) record the message, and one dispatch can stop the
source (the native window-procedure owner stops there). Compared: completion,
3d08534, the dispatch count, and the ordered trace of peeked window handles and
dispatched (hwnd, message, wParam). When the first peek finds nothing the source
stores an uninitialised stack word: the native owner reports it unknown, and
those cases are counted as explicit.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
PEEK,TRANSLATE,DISPATCH=CODE+0x400,CODE+0x410,CODE+0x420
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for slot,stub,pop in((0x7db1ec,PEEK,0x14),(0x7db1fc,TRANSLATE,4),(0x7db1f8,DISPATCH,4)):m.mem_write(stub,b'\xc2'+struct.pack('<H',pop));m.mem_write(slot,struct.pack('<I',stub))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);c=st['case']
        if address==PEEK:
            msg,hwnd=word(mu,esp+4),word(mu,esp+8);st['trace'].append(0xb0000000|hwnd)
            if st['next']>=c[1]:mu.reg_write(UC_X86_REG_EAX,0);return
            w=c[4+7*st['next']:11+7*st['next']];st['next']+=1;mu.mem_write(msg,struct.pack('<7I',*w));mu.reg_write(UC_X86_REG_EAX,1);return
        if address==TRANSLATE:mu.reg_write(UC_X86_REG_EAX,0);return
        msg=word(mu,esp+4);w=struct.unpack('<3I',mu.mem_read(msg,12));st['trace']+=list(w);st['dispatches']+=1
        if c[2] and st['dispatches']==c[2]:st['stopped']=True;mu.emu_stop();return
        mu.reg_write(UC_X86_REG_EAX,0)
    for addr in(PEEK,TRANSLATE,DISPATCH):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_message_pump;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x789bd0);bad=[];outcomes={};cases=explicit=0
    for n in range(2000):
        k=rng.choice([0,0,1,1,2,3,7]);c=[rng.getrandbits(32),k,0 if rng.random()<0.8 else rng.randrange(1,4),rng.getrandbits(32)]
        for i in range(k):c+=[rng.getrandbits(32) for _ in range(7)]
        out=(ctypes.c_uint32*256)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(case=c,trace=[],next=0,dispatches=0,stopped=False)
        m.mem_write(0x3d0fd30,struct.pack('<I',c[0]));m.mem_write(0x3d08534,struct.pack('<I',c[3]))
        m.mem_write(STACK-0x100,bytes(rng.getrandbits(8) for _ in range(0x100)))   # stack garbage under the MSG local
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x789bd0,STOP,count=100000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        if k==0:explicit+=1;outcomes['explicit:uninitialised-wparam']=outcomes.get('explicit:uninitialised-wparam',0)+1
        e=[0]*256;e[0]=int(not st['stopped']);e[1]=word(m,0x3d08534);e[2]=1;e[3]=st['dispatches']-int(st['stopped'])
        if k==0:e[1]=0xdeadbeef;e[2]=0   # the native owner must report the uninitialised word unknown
        e[4]=len(st['trace'])
        for i,t in enumerate(st['trace'][:200]):e[5+i]=t
        cases+=1;key=f"completed{e[0]}:messages{k}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(256) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:10],source=[hex(e[i]) for i in d[:10]],native=[hex(got[i]) for i in d[:10]]))
    names=['src/game/game_window.hpp','src/game/game_window.cpp','tests/probes/native_message_pump_probe.cpp','tools/oracles/verify_original_message_pump.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x789bd0-0x789c45':sha(image.read(0x789bd0,0x75))},cases=cases,explicit_uninitialised=explicit,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,explicit=explicit,mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
