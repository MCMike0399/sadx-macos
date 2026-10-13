"""Ninja fog owners against original instructions: device fog init 794ec0 (with
795930, CRT pow 6447a0 on both its x87 and SSE2 paths, 794db0, 7949f0), fog
colour 7949b0, njGenerateFogTable3 787260 (CRT frexp 7b5eef, _ftol2 644340,
794db0, 7949f0), fog-table edges 794e40 (7949f0) and the switches 411af0/411b40
with 4119b0, 411a20, 787220/787230 and 794950/794980.

The whole PE image (including .data and its zero tail) is mapped; the D3D8
device's SetRenderState is a fixture (recorded site, state, value) and one
call can fail. Compared: completion, every fog global, the three 128-entry
tables, the 411af0 cache/flag words and the render trace; x87 status flags are
reported separately. Native explicit boundaries (FYL2X EXP2 density 794afc,
frexp of Inf/NaN 78728e, 787260 reference past the table 78753a) are counted,
not compared: the source continues past them.
"""
import argparse,ctypes,hashlib,json,math,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DEVICE=DATA+0x100;VTABLE=DATA+0x400;METHOD=CODE+0x300;TABLE=0x3b10f78
ENTRY={0:0x794ec0,1:0x7949b0,2:0x787260,3:0x794e40}
GLOBALS=[(0x3d12798,4),(0x3d128a4,4),(0x3d128a0,4),(0x3d12898,4),(0x3d0fce0,4),(0x3d12950,4),(0x3d0fac0,4),(0x3d12894,4),(0x3d0849c,4),(0x90a098,4),(0x90a09c,4),(0x3abdcfe,1)]
EXPLICIT=(0x794afc,0x78728e,0x78753a)
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
def f32(v):return struct.unpack('<I',struct.pack('<f',v))[0]
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x7f).machine
        for s in image.sections:
            if s['name'] in('.text','.rdata'):continue
            base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
            m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
        m.mem_write(METHOD,b'\x31\xc0\xc2\x0c\x00');m.mem_write(VTABLE+0xc8,struct.pack('<I',METHOD));m.mem_write(DEVICE,struct.pack('<I',VTABLE))
        self.clean=m.context_save();m.hook_add(unicorn.UC_HOOK_CODE,self.code,begin=METHOD,end=METHOD)
    def code(self,m,address,size,user):
        esp=m.reg_read(UC_X86_REG_ESP);self.trace.append((get(m,esp+8,4),get(m,esp+12,4),get(m,esp,4)-6))
        if self.c[2] and len(self.trace)==self.c[2]:self.stopped=True;m.emu_stop()
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.trace=[];self.stopped=False
        for (a,n),v in zip([(0x3d12798,4),(0x3d128a4,4),(0x3d128a0,4),(0x3d12898,4),(0x3d0fce0,4),(0x3d12950,4),(0x3d0fac0,4),(0x3d12894,4),(0x3d0849c,4),(0x3d127e4,4),(0x3d08ae0,4)],c[4:15]):put(m,a,n,v)
        put(m,0x3b1117c,4,c[17]);put(m,0x3abdc6c,1,c[18]);put(m,0x909eb4,1,c[19]);put(m,0x90a098,4,c[20]);put(m,0x90a09c,4,c[21])
        put(m,0x3abdc60,4,c[22]);put(m,0x3abdc64,4,c[23]);put(m,0x3abdc68,4,c[24]);put(m,0x3abdcfe,1,c[25]);put(m,0x3d0f400,4,c[27]);put(m,0x3d128b0,4,DEVICE)
        m.mem_write(0x3d12a00,struct.pack('<128I',*c[32:160]));m.mem_write(0x3d12c00,struct.pack('<128I',*c[160:288]));m.mem_write(TABLE,struct.pack('<128I',*c[288:416]))
        kind=c[0];entry=ENTRY.get(kind,0x411af0 if c[26] else 0x411b40)
        args={1:[c[15]],2:[TABLE,c[15],c[16]],3:[TABLE]}.get(kind,[])
        m.mem_write(STACK,struct.pack(f'<{1+len(args)}I',STOP,*args));m.reg_write(UC_X86_REG_ESP,STACK)
        m.reg_write(UC_X86_REG_FPCW,c[1]);m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.emu_start(entry,STOP,count=5000000)
        if not self.stopped and m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=[0]*480;out[0]=int(not self.stopped);out[1]=len(self.trace)
        for i,(a,n) in enumerate(GLOBALS):out[2+i]=get(m,a,n)
        out[14]=m.reg_read(UC_X86_REG_FPSW)&0x3f
        for i,(s,v,site) in enumerate(self.trace[:24]):out[16+3*i:19+3*i]=[s,v,site]
        out[96:224]=struct.unpack('<128I',m.mem_read(0x3d12c00,512));out[224:352]=struct.unpack('<128I',m.mem_read(0x3d12a00,512));out[352:480]=struct.unpack('<128I',m.mem_read(TABLE,512))
        return out
def w_table():return [f32((2.0**(i>>4))*((i&15)+16.0)*0.0625) for i in range(128)]
def fixtures():
    rng=random.Random(0x794ec0);cases=[];w=w_table()
    recip=[f32(32768.0/struct.unpack('<f',struct.pack('<I',x))[0]) for x in w]
    def interesting():return rng.choice([0.0,-0.0,0.3,0.29999998,0.30000004,0.5,0.6,0.59999996,0.9,0.90000004,1.0,1.0000001,0.1,0.75,rng.random(),rng.uniform(-1,2)])
    def base(kind):
        c=[0]*416;c[0]=kind;c[1]=rng.choice([0x7f,0x7f,0x27f,0x37f,0xc7f,0x47f,0x87f]);c[2]=0 if rng.random()<0.8 else rng.randrange(1,6)
        c[4]=rng.choice([0,1]);c[5]=rng.choice([0,1,1]);c[6]=rng.getrandbits(32);c[7]=rng.choice([0xffffff,0,rng.getrandbits(32)])
        c[8]=rng.randrange(0,128);c[9]=rng.randrange(0,128);c[10]=f32(interesting());c[11]=f32(interesting());c[12]=rng.choice([0,1,0xdeadbeef])
        c[13]=rng.getrandbits(32)|(0x100000 if rng.random()<0.6 else 0);c[14]=rng.choice([0,0,0x20,rng.getrandbits(32)]);c[27]=rng.choice([0,1])
        c[32:160]=w if rng.random()<0.8 else [f32(rng.uniform(0.5,300)) for _ in range(128)]
        c[160:288]=recip if rng.random()<0.7 else [f32(rng.uniform(0,70000)) for _ in range(128)]
        c[288:416]=[rng.getrandbits(32) for _ in range(128)];return c
    def table():
        lead=rng.randrange(0,130);tail=rng.randrange(0,130);t=[]
        for i in range(128):
            if i<lead:t.append(1.0)
            elif i>=128-tail:t.append(0.0)
            else:t.append(rng.choice([interesting(),rng.random(),max(0.0,1.0-(i-lead)/max(1,128-tail-lead))]))
        if rng.random()<0.05:t[rng.randrange(128)]=float('nan')
        return [f32(x) for x in t]
    for n in range(400):cases.append(base(0))
    for n in range(300):
        c=base(1);c[15]=rng.choice([c[7],c[7]|0xff000000,rng.getrandbits(32),0]);cases.append(c)
    ends=[-65535.0,-65536.0,-1000.0,-1.0,-0.0,0.0,1.0,100.0,3000.0,65535.0,1e30,-1e30,1e-30,-3.5,-300.0,-12000.0]
    # _ftol2 keeps bits 8..15: values whose negation ends in 0xff expose a wrong carry/borrow.
    ends+=[32769.0,65281.0,33025.0,4096.125,131076.0,-32769.0,-65281.0,-4096.125]
    for n in range(1200):
        c=base(2);start=rng.choice(ends+[rng.uniform(-70000,70000),-rng.uniform(0,5000),float('nan')]);end=rng.choice(ends+[rng.uniform(-70000,70000),-rng.uniform(0,5000)])
        c[15]=f32(start);c[16]=f32(end);cases.append(c)
    for n in range(1200):
        c=base(3);c[288:416]=table();cases.append(c)
    for n in range(1600):
        c=base(4);c[17]=rng.choice([0,0,0,1]);c[26]=rng.choice([0,1,1]);c[18]=rng.choice([0,0,1,7]);c[19]=rng.choice([0,1,2,3])
        sentinel=f32(-65535.0);c[20]=rng.choice([sentinel,sentinel,f32(65535.0),f32(rng.uniform(-5000,0))]);c[21]=rng.choice([sentinel,sentinel,c[20],f32(rng.uniform(-5000,0))])
        start=rng.choice([300.0,-300.0,1000.0,-1000.0,0.0,rng.uniform(-8000,8000)]);end=rng.choice([3000.0,-3000.0,-12000.0,8000.0,rng.uniform(-20000,20000)])
        c[22]=f32(start);c[23]=f32(end);c[24]=rng.getrandbits(32)
        if rng.random()<0.3:c[20]=f32(-abs(start));c[21]=f32(-abs(end)) if rng.random()<0.7 else c[21]
        c[25]=rng.choice([0,1,3]);cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_fog;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);bad=[];flags=0;flag_kinds={};outcomes={};explicit={};cases=fixtures()
    for n,c in enumerate(cases):
        e=source.run(c);out=(ctypes.c_uint32*480)()
        if fn((ctypes.c_uint32*416)(*c),416,out,480)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        if not got[0] and got[15] in EXPLICIT and (not c[2] or got[1]<c[2]):
            key=f"kind{c[0]}:{got[15]:#x}";explicit[key]=explicit.get(key,0)+1;continue
        key=f"kind{c[0]}:completed{e[0]}:calls{e[1]}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(480) if i not in(14,15) and got[i]!=e[i]]
        if diff:bad.append(dict(case=n,kind=c[0],input=[hex(x) for x in c[:28]],fields=diff[:12],source=[hex(e[i]) for i in diff[:12]],native=[hex(got[i]) for i in diff[:12]],native_boundary=hex(got[15])))
        if got[14]!=e[14]:flags+=1;fk=f'source{e[14]:#x}:native{got[14]:#x}';flag_kinds[fk]=flag_kinds.get(fk,0)+1
    names=['src/engine/ninja_fog.hpp','src/engine/ninja_fog.cpp','tests/probes/native_fog_probe.cpp','tools/oracles/verify_original_fog.py']
    blob=b''.join(struct.pack('<416I',*c) for c in cases)
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={hex(s):sha(image.read(s,n)) for s,n in((0x794ec0,0x134),(0x7949b0,0x35),(0x7949f0,0x3b4),(0x794db0,0x90),(0x787260,0x385),(0x4119b0,0x68),(0x411a20,0x120))},
        cases=len(cases),case_sha256=sha(blob),source_outcomes=outcomes,native_explicit_boundaries=explicit,state_mismatch_count=len(bad),fp_flag_mismatches=flags,fp_flag_mismatch_kinds=flag_kinds,mismatches=bad[:20],state_trace_tables_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),compared=sum(outcomes.values()),explicit=explicit,mismatches=len(bad),fp_flag_mismatches=flags,flag_kinds=flag_kinds)))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
