"""The 785190 vibration update against its original instructions.

The four 0x18-stride entries at 3d08428 (state +0, strength byte +6, duration
+0xc, counter +0x14) are case inputs. 793da0, 793e00 (writes the case's
elapsed word into its second argument), 791f80 and 792000 are recording
fixtures; any nested call can stop the source. Compared: completion, the four
states and counters, and the nested trace (callee, site, arguments).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
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
        esp=mu.reg_read(UC_X86_REG_ESP);ret=word(mu,esp);D=0xdeadbeef;args=[D,D]
        if address in(0x791f80,):args=[word(mu,esp+4),word(mu,esp+8)]
        elif address==0x792000:args=[word(mu,esp+4),D]
        st['nested'].append([address,ret-5]+args)
        if st['fail'] and len(st['nested'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        if address==0x793e00:mu.mem_write(word(mu,esp+8),struct.pack('<I',st['case'][17+min(st['elapsed'],7)]));st['elapsed']+=1
        mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for addr in(0x793da0,0x793e00,0x791f80,0x792000):m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=addr,end=addr)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_vibration;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    rng=random.Random(0x785190);bad=[];outcomes={};cases=0
    for n in range(4000):
        c=[0]*25;c[0]=0 if rng.random()<0.8 else rng.randrange(1,10)
        for i in range(4):c[1+i]=rng.choice([0,0,1,2,3,4,0xffffffff]);c[5+i]=rng.choice([0,1,0x7f,0x80,0xff,rng.getrandbits(8)]);c[9+i]=rng.choice([0,10,100,0xffffffff,rng.getrandbits(32)]);c[13+i]=rng.getrandbits(32)
        for i in range(8):c[17+i]=rng.choice([0,9,10,11,100,0xffffffff,rng.getrandbits(32)])
        out=(ctypes.c_uint32*120)()
        if fn((ctypes.c_uint32*25)(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(case=c,nested=[],stopped=False,fail=c[0],elapsed=0)
        for i in range(4):
            at=0x3d08428+0x18*i;m.mem_write(at,struct.pack('<I',c[1+i]));m.mem_write(at+6,bytes([c[5+i]]));m.mem_write(at+0xc,struct.pack('<I',c[9+i]));m.mem_write(at+0x14,struct.pack('<I',c[13+i]))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x785190,STOP,count=100000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(0):#x}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        e=[0]*120;e[0]=int(not st['stopped'])
        for i in range(4):e[1+i]=word(m,0x3d08428+0x18*i);e[5+i]=word(m,0x3d08428+0x18*i+0x14)
        e[10]=len(st['nested'])
        for i,t in enumerate(st['nested'][:24]):e[11+4*i:15+4*i]=t
        cases+=1;key=f"completed{e[0]}:nested{e[10]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(120) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,input=c,fields=d[:10],source=[hex(e[i]) for i in d[:10]],native=[hex(got[i]) for i in d[:10]]))
    names=['src/engine/ninja_vibration.hpp','src/engine/ninja_vibration.cpp','tests/probes/native_vibration_probe.cpp','tools/oracles/verify_original_vibration.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x785190-0x785233':sha(image.read(0x785190,0xa3))},cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
