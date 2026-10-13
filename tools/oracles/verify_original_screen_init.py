"""780c30 (screen setup, 781070's 78113f) against its original instructions,
with the real 7899a0, 7912e0, 77ec60, 77ff70 and 780090 code.

The whole PE is mapped. The device is a COM fixture recording SetViewport
(+0xa0, its six words), SetTransform (+0x94) and MultiplyTransform (+0x9c)
(state and the sixteen matrix words) and SetRenderState (+0xc8). 3d0fd1c is
seeded per case (1 runs 7912e0's body; the loader's reset_device is its only
writer). Seeded first: 3ce712c = 5, 3d0f840 = 0x1234, 3d0f9c4 = 0x5678,
3ce71b0 = 0x11, 3ce71a4 = 0x22 and the 3ce7174 colours 1..4, so their stores
are visible. A null device faults at the vtable load (page 0 is unmapped), as
the native owner stops (null_device). Compared: completion, every global the
source writes, 7899a0's words and the device-method trace with its arguments.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;METHODS={0xa0:(CODE+0x300,8),0x94:(CODE+0x310,0xc),0x9c:(CODE+0x320,0xc),0xc8:(CODE+0x330,0xc)}
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    m=Original(image,0x7f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    for off,(stub,pop) in METHODS.items():m.mem_write(stub,b'\x31\xc0\xc2'+struct.pack('<H',pop));m.mem_write(VTABLE+off,struct.pack('<I',stub))
    m.mem_write(DEVICE,struct.pack('<I',VTABLE))
    clean=m.context_save();st={}
    def word(mu,at):return struct.unpack('<I',mu.mem_read(at,4))[0]
    def hook(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==METHODS[0xa0][0]:st['calls'].append([0xa0,0,0]);st['viewport']=list(struct.unpack('<6I',mu.mem_read(word(mu,esp+8),24)));return
        if address==METHODS[0x94][0]:st['calls'].append([0x94,word(mu,esp+8),0]);st['set']=list(struct.unpack('<16I',mu.mem_read(word(mu,esp+12),64)));return
        if address==METHODS[0x9c][0]:st['calls'].append([0x9c,word(mu,esp+8),0]);st['multiply']=list(struct.unpack('<16I',mu.mem_read(word(mu,esp+12),64)));return
        if address==METHODS[0xc8][0]:st['calls'].append([0xc8,word(mu,esp+8),word(mu,esp+12)]);return
    for stub,_ in METHODS.values():m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=stub,end=stub)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_screen_init;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    matrix=list(struct.unpack('<16I',image.read(0x389d318,64)));tables=list(struct.unpack('<22I',image.read(0x88ad64,88)))
    rf=lambda rng:struct.unpack('<I',struct.pack('<f',rng.choice([1.0,0.5,-2.0,1.0e-3,3.0e5])*rng.random()))[0]
    rng=random.Random(0x780c30);bad=[];outcomes={};cases=0
    for n in range(3000):
        c=[0]*68;c[0]=rng.choice([0x7f,0x27f,0x37f,0xc7f,0x47f,0x87f])
        c[1]=rng.choice([0,0x10,0x20,0x30,0x10030,0x10000,0x10010,0x10020,rng.getrandbits(32)]);c[2]=rng.getrandbits(32);c[3]=rng.choice([1,2,3,0,0xffffffff,rng.getrandbits(32)])
        c[4]=rng.choice([0,0,10,0x80000000]);c[5]=rng.choice([0,0,20,0xfffffff0]);c[6]=rng.choice([640,1280,0,1,0x80000000,0xffffffff,0x7fffffff,rng.getrandbits(32)]);c[7]=rng.choice([480,720,0,1,0x80000001,0xffffffff,rng.getrandbits(32)])
        c[8]=rng.choice([0,0x3f000000]);c[9]=rng.choice([0x3f800000,0x3f000000])
        c[10]=rng.choice([0,1,1,1,2,0xffffffff])
        for i in range(16):c[11+i]=matrix[i] if rng.random()<0.8 else rng.getrandbits(32);c[27+i]=rf(rng) if rng.random()<0.9 else rng.getrandbits(32)
        c[43]=rng.choice([10000000,3579545,0,rng.getrandbits(32)]);c[44]=rng.choice([0,0,1,rng.getrandbits(32)]);c[45]=int(rng.random()<0.93)
        c[46:68]=tables
        out=(ctypes.c_uint32*200)()
        if fn((ctypes.c_uint32*len(c))(*c),out)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        m.context_restore(clean);st.update(calls=[],viewport=[0]*6,set=[0]*16,multiply=[0]*16)
        for at,n_ in((0x3d0f804,4),(0x3d0f964,4),(0x3d0f814,4),(0x3d0f9e4,8),(0x3d0fa20,4),(0x3d0f838,4),(0x3d0fa04,16),(0x3ce7160,4),(0x3d0f820,16),(0x3ce7164,0x40),(0x3d0fd80,64),(0x3d0fd20,16),(0x389d7dc,4),(0x3d084a8,8)):m.mem_write(at,bytes(n_))
        m.mem_write(0x3ce712c,struct.pack('<I',5));m.mem_write(0x3d0f840,struct.pack('<I',0x1234));m.mem_write(0x3d0f9c4,struct.pack('<I',0x5678))
        m.mem_write(0x3ce71b0,struct.pack('<I',0x11));m.mem_write(0x3ce71a4,struct.pack('<I',0x22));m.mem_write(0x3ce7174,struct.pack('<4I',1,2,3,4))
        m.mem_write(0x3d12780,struct.pack('<6I',*c[4:10]));m.mem_write(0x3d0fd1c,struct.pack('<I',c[10]))
        m.mem_write(0x389d318,struct.pack('<16I',*c[11:27]));m.mem_write(0x3d129c0,struct.pack('<16I',*c[27:43]));m.mem_write(0x3d08538,struct.pack('<II',c[43],c[44]))
        m.mem_write(0x3d128b0,struct.pack('<I',DEVICE if c[45] else 0))
        m.reg_write(UC_X86_REG_FPCW,c[0]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.mem_write(STACK,struct.pack('<4I',STOP,c[1],c[2],c[3]));m.reg_write(UC_X86_REG_ESP,STACK)
        faulted=False
        try:m.emu_start(0x780c30,STOP,count=200000)
        except unicorn.UcError:faulted=True
        if not faulted and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'case {n}: ended at {m.reg_read(UC_X86_REG_EIP):#x}')
        g=lambda at:word(m,at);e=[0]*200;e[0]=int(not faulted)
        for i,at in enumerate((0x3d0f804,0x3d0f964,0x3d0f814,0x3d0f9e4,0x3d0fa20,0x3d0f9e8,0x3d0f838,0x3d0fa04,0x3d0fa08,0x3d0fa0c,0x3d0fa10,0x3ce7160,0x3d0f820,0x3d0f824,0x3d0f828,0x3d0f82c,0x3d0f840,0x3d0f9c4,0x3ce712c,0x3ce71b0,0x3ce71a4)):e[1+i]=g(at)
        for i in range(4):e[22+i]=g(0x3ce7174+4*i)
        for i in range(8):e[26+i]=g(0x3ce7184+4*i)
        e[34]=g(0x3ce7164);e[35]=g(0x3ce7168);e[36]=g(0x3ce7170);e[37]=g(0x3d0fd1c)
        for i in range(16):e[38+i]=g(0x3d0fd80+4*i)
        for i in range(4):e[54+i]=g(0x3d0fd20+4*i)
        e[58]=g(0x389d7dc);e[59]=g(0x3d084a8);e[60]=g(0x3d084ac)
        e[100]=len(st['calls'])
        for i,t in enumerate(st['calls'][:8]):e[101+3*i:104+3*i]=t
        e[130:136]=st['viewport'];e[140:156]=st['set'];e[160:176]=st['multiply']
        cases+=1;key=f"completed{e[0]}:calls{e[100]}:invalid{int(c[10]==1)}";outcomes[key]=outcomes.get(key,0)+1
        d=[i for i in range(200) if got[i]!=e[i]]
        if d:bad.append(dict(case=n,fields=d[:12],source=[hex(e[i]) for i in d[:12]],native=[hex(got[i]) for i in d[:12]]))
    names=['src/ninja/ninja_screen.hpp','src/ninja/ninja_screen.cpp','src/game/game_frame_control.cpp','src/ninja/ninja_render.cpp','tests/probes/native_screen_init_probe.cpp','tools/oracles/verify_original_screen_init.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={'0x780c30-0x780e70':sha(image.read(0x780c30,0x240)),'0x7912e0-0x7913cc':sha(image.read(0x7912e0,0xec)),'0x77ff70':sha(image.read(0x77ff70,0x14)),'0x780090':sha(image.read(0x780090,0xb))},
        cases=cases,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],state_traces_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
