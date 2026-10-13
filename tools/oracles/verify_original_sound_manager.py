"""Sound-effect manager 4250d0 (with its 424b10 entry check) against original
instructions. Every helper the native port treats as an owner (425710,
4246c0, 4244a0 [index in EAX], 423c20, 410260, 423cc0, 410100, 424b10) is a
fixture: it records callee, call site and two arguments and returns a
case-derived EAX, or stops the source at a chosen call. Compared: completion,
the call trace, the frame counter 3b29ce8, the 3b292a8 table, and every record
field (timer, +4, flags, id, frequency, volume, pan, the 3b29c28 volume and
3b29b90 entity tables). Active records whose 4246c0 body continues through
unported owners end there natively; those cases are counted, not compared.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP
sha=lambda b:hashlib.sha256(b).hexdigest()
# 424b10 is entered by JMP and tests its listener itself: its owner point is the
# with-listener continuation 424b26, unwound through its own epilogue at 424c14.
OWNERS=(0x425710,0x4246c0,0x4244a0,0x423c20,0x410260,0x423cc0,0x410100,0x424b26)
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x37f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    clean=m.context_save();st={}
    def owner(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP);ret,a0,a1=struct.unpack('<III',mu.mem_read(esp,12))
        site=ret-5
        if address==0x4244a0:a0,a1=mu.reg_read(UC_X86_REG_EAX),0xdeadbeef
        elif address in(0x425710,0x4246c0,0x410100):a1=0xdeadbeef
        elif address==0x424b26:a0=a1=0xdeadbeef;site=0x4253b1
        st['trace'].append([0x424b10 if address==0x424b26 else address,site,a0,a1])
        if st['fail'] and len(st['trace'])==st['fail']:st['stopped']=True;mu.emu_stop();return
        mu.reg_write(UC_X86_REG_EAX,(st['eax']+len(st['trace'])*0x101)&0xffffffff)
        if address==0x424b26:mu.reg_write(UC_X86_REG_EIP,0x424c14);return
        mu.reg_write(UC_X86_REG_ESP,esp+4);mu.reg_write(UC_X86_REG_EIP,ret)
    for o in OWNERS:m.hook_add(unicorn.UC_HOOK_CODE,owner,begin=o,end=o)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_sound_manager;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    rng=random.Random(0x4250d0);bad=[];outcomes={};explicit=0;cases=0
    FLAGS=[0,0,0,1,0x11,0x100|0x22,0x22,0x2,0x102,0x120,0x200,0x800,0x2000,0x10,0x4000|0x822,0xa22|0x2100,0x4000]
    for n in range(1500):
        c=[0]*(32+12*36)
        c[0]=0 if rng.random()<0.9 else rng.getrandbits(32);c[1]=0 if rng.random()<0.7 else 1;c[2]=rng.choice([0,1,2,3,4,5,0xff]);c[3]=0 if rng.random()<0.8 else 0x12345678
        c[4]=0 if rng.random()<0.75 else rng.randrange(1,6);c[5]=rng.getrandbits(32)
        for i in range(18):c[8+i]=rng.getrandbits(32)
        active=rng.random()<0.5
        for r in range(36):
            w=c[32+12*r:32+12*r+12]
            w[0]=rng.choice([0,0,1,2,3,0xffffffff,rng.getrandbits(32)]) if rng.random()<0.4 else 0
            w[1]=rng.getrandbits(32);w[2]=rng.choice(FLAGS) if (active or rng.random()<0.3) else 0
            if not active and w[2]&1:w[2]&=~1
            w[3]=rng.choice([0x100,0x568,0x569,0x600,0x30,rng.getrandbits(32)]);w[4]=rng.choice([0,1,126,127,-200&0xffffffff,rng.getrandbits(32)]);w[5]=rng.getrandbits(8)
            w[6]=rng.getrandbits(32);w[7]=rng.choice([w[5],rng.getrandbits(32)]);w[8]=rng.getrandbits(32)
            c[32+12*r:32+12*r+12]=w
        out=(ctypes.c_uint32*600)()
        if fn((ctypes.c_uint32*len(c))(*c),len(c),out,600)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        trace=[got[464+4*i:468+4*i] for i in range(min(got[1],24))]
        reach=[i for i,t in enumerate(trace) if t[0]==0x4246c0]
        if reach and (not c[4] or c[4]>reach[0]+1):explicit+=1;continue # native ends the active body there; the source continues into unported code
        m.context_restore(clean);st.update(trace=[],fail=c[4],stopped=False,eax=c[5])
        put=lambda addr,v:m.mem_write(addr,struct.pack('<I',v&0xffffffff))
        put(0x3b29ce0,c[0]);put(0x3b29ce4,c[1]);m.mem_write(0x3b29ce8,bytes([c[2]&0xff]));put(0x3b2cbb0,c[3])
        for i in range(18):put(0x3b292a8+4*i,c[8+i])
        for r in range(36):
            w=c[32+12*r:32+12*r+12];at=0x3b292fc+0x38*r
            for off,v in zip((0,4,8,0xc,0x10,0x14,0x1c),w[:7]):put(at+off,v)
            put(0x3b29c28+4*r,w[7]);put(0x3b29b90+4*r,w[8])
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK)
        try:m.emu_start(0x4250d0,STOP,count=500000)
        except unicorn.UcError as err:raise RuntimeError(f'case {n}: {err} at {m.reg_read(UC_X86_REG_EIP):#x} trace {st["trace"][-3:]}')
        if not st['stopped'] and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        g=lambda addr:struct.unpack('<I',m.mem_read(addr,4))[0]
        e=[0]*600;e[0]=int(not st['stopped']);e[1]=len(st['trace']);e[2]=m.mem_read(0x3b29ce8,1)[0]
        for i in range(18):e[8+i]=g(0x3b292a8+4*i)
        for r in range(36):
            at=0x3b292fc+0x38*r;e[32+12*r:32+12*r+9]=[g(at),g(at+4),g(at+8),g(at+0xc),g(at+0x10),g(at+0x14),g(at+0x1c),g(0x3b29c28+4*r),g(0x3b29b90+4*r)]
        for i,t in enumerate(st['trace'][:24]):e[464+4*i:468+4*i]=t
        cases+=1;key=f"completed{e[0]}:calls{e[1]}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(600) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/game/sound_manager.hpp','src/game/sound_manager.cpp','tests/probes/native_sound_manager_probe.cpp','tools/oracles/verify_original_sound_manager.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256=sha(image.read(0x4250d0,0x2eb)+image.read(0x424b10,0x20)),cases=cases,native_explicit_boundaries=explicit,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],records_trace_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,explicit=explicit,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
