"""Ninja keyboard: poll 791bd0 (reached through 77f090) and init 77f0a0 with
791b10 and 77ffc0, against original instructions. DirectInput8 CreateDevice,
the device's GetDeviceState/Acquire/SetDataFormat/SetCooperativeLevel are
fixtures (state copied from the case, Acquire returns DIERR_INPUTLOST a chosen
number of times). Compared: completion, Acquire calls, the four records, the
3ce6e68 words, 3d08818/3d0881c, the callback count and last slot, and whether
a device was stored. Record bases other than 3ce6e38 or counts above 4 are
native explicit boundaries (counted, not compared).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,DATA,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
DI8=DATA+0x100;DI8_VT=DATA+0x200;DEV=DATA+0x300;DEV_VT=DATA+0x400;STUB=CODE+0x400
def put(m,a,n,v):m.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
def get(m,a,n):return int.from_bytes(m.mem_read(a,n),'little')
class Source:
    def __init__(self,image):
        self.m=m=Original(image,0x37f).machine
        for page in(0x3ce6000,0x3d08000,0x3d0f000,0x3ce7000):m.mem_map(page,0x1000)
        m.mem_map(0x389d000,0x2000);m.mem_write(0x389d000,image.read(0x389d000,0x2000))
        m.mem_map(0x2bcb000,0x1000);m.mem_write(0x2bcb000,image.read(0x2bcb000,0x1000)) # stack cookie 2bcbddc
        self.stubs={}
        for i,(name,args) in enumerate([('create',4),('format',2),('coop',3),('state',3),('acquire',1)]):
            a=STUB+0x10*i;m.mem_write(a,b'\xc2'+struct.pack('<H',4*args));self.stubs[a]=name;m.hook_add(unicorn.UC_HOOK_CODE,self.hook,begin=a,end=a)
        rev={v:k for k,v in self.stubs.items()}
        m.mem_write(DI8,struct.pack('<I',DI8_VT));m.mem_write(DI8_VT+0xc,struct.pack('<I',rev['create']))
        m.mem_write(DEV,struct.pack('<I',DEV_VT));m.mem_write(DEV_VT+0x2c,struct.pack('<I',rev['format']));m.mem_write(DEV_VT+0x34,struct.pack('<I',rev['coop']))
        m.mem_write(DEV_VT+0x24,struct.pack('<I',rev['state']));m.mem_write(DEV_VT+0x1c,struct.pack('<I',rev['acquire']))
        self.clean=m.context_save()
    def hook(self,m,address,size,user):
        name=self.stubs[address];esp=m.reg_read(UC_X86_REG_ESP);c=self.c
        if name=='create':put(m,get(m,esp+12,4),4,DEV if c[1] else 0);m.reg_write(UC_X86_REG_EAX,0 if c[1] else 0x80004005)
        elif name=='state':
            if get(m,esp+8,4)!=256:raise RuntimeError('GetDeviceState size')
            m.mem_write(get(m,esp+12,4),bytes(c[72:328]));m.reg_write(UC_X86_REG_EAX,c[2])
        elif name=='acquire':self.acquires+=1;m.reg_write(UC_X86_REG_EAX,0x8007001e if self.acquires<=c[3] else 0)
        else:m.reg_write(UC_X86_REG_EAX,0)
    def run(self,c):
        m=self.m;m.context_restore(self.clean);self.c=c;self.acquires=0
        put(m,0x3d08810,4,DI8);put(m,0x3d08820,4,DEV if (c[0]==0 and c[1]) else 0);put(m,0x3d0881c,4,c[4]);put(m,0x3d08818,4,c[5]);put(m,0x3ce712c,4,c[6]);put(m,0x3d0fd30,4,0x1234)
        m.mem_write(0x3ce6e38,bytes(c[8:56]));m.mem_write(0x3ce6e68,bytes(c[56:72]));m.mem_write(0x3d0fa40,bytes(64))
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x77f090 if c[0]==0 else 0x77f0a0,STOP,count=200000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('Source return ABI')
        out=[0]*80;out[0]=1;out[1]=self.acquires
        out[8:56]=list(m.mem_read(0x3ce6e38,48));out[56:72]=list(m.mem_read(0x3ce6e68,16))
        out[72]=get(m,0x3d08818,4);out[73]=get(m,0x3d0881c,4);n=get(m,0x3ce712c,4);out[74]=n;out[75]=get(m,0x3d0fa40+4*(n-1),4) if 0<n<=16 else 0;out[76]=int(get(m,0x3d08820,4)!=0)
        return out
def fixtures(image):
    rng=random.Random(0x791bd0);table=list(image.read(0x389d918,0xed*4));keys=[i for i in range(0xed) if table[4*i]|(table[4*i+1]<<8) in(0,1)]
    cases=[]
    for n in range(3000):
        c=[0]*1276;c[0]=0 if n%6 else 1;c[1]=rng.choice([1,1,1,0]);c[2]=rng.choice([0,0,0,0x8007000c,1]);c[3]=rng.choice([0,0,1,3])
        c[4]=rng.choice([4,4,4,1,2,3,0,0xffffffff]+([5] if n%50==0 else []));c[5]=0x3ce6e38 if n%60 else 0x3ce6e44;c[6]=rng.randrange(0,12)
        for i in range(4):
            rec=[rng.getrandbits(8),rng.getrandbits(8)]+[0]*6
            held=rng.sample(keys,rng.randrange(0,7));codes=[table[4*k+2] for k in held]
            if rng.random()<0.5:codes=[x for x in codes]
            else:codes=[rng.getrandbits(8) for _ in range(rng.randrange(0,7))]
            for k,v in enumerate(codes[:6]):rec[2+k]=v
            if rng.random()<0.3:rec[2+rng.randrange(6)]=0 # a zero hole ends the source's held-key scans early
            c[8+12*i:8+12*i+12]=rec+list(struct.pack('<I',rng.choice([0x3ce6e68+4*i,rng.getrandbits(32)])))
        c[56:72]=[rng.getrandbits(8) for _ in range(16)]
        down=rng.sample(keys,rng.choice([0,1,2,3,5,6,7,9,12]))+rng.sample(range(256),rng.choice([0,0,2]))
        for k in down:c[72+k]=rng.choice([0x80,0xff,0x81])
        for k in rng.sample(range(256),4):c[72+k]|=rng.getrandbits(7)
        c[328:1276]=table;cases.append(c)
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_keyboard;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int
    source=Source(image);cases=fixtures(image);bad=[];outcomes={};explicit=0
    for n,c in enumerate(cases):
        out=(ctypes.c_uint32*80)()
        if fn((ctypes.c_uint32*1276)(*c),1276,out,80)!=1:raise RuntimeError('Native rejected')
        got=list(out)
        if not got[0]:explicit+=1;continue
        e=source.run(c);key=f"kind{c[0]}:device{c[1]}:hr{c[2]:#x}:count{c[4]:#x}";outcomes[key]=outcomes.get(key,0)+1
        diff=[i for i in range(80) if i!=2 and got[i]!=e[i]]
        if diff:bad.append(dict(case=n,fields=diff[:12],source=[e[i] for i in diff[:12]],native=[got[i] for i in diff[:12]]))
    names=['src/engine/ninja_keyboard.hpp','src/engine/ninja_keyboard.cpp','tests/probes/native_keyboard_probe.cpp','tools/oracles/verify_original_keyboard.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_ranges_sha256={hex(s):sha(image.read(s,n)) for s,n in((0x791bd0,0x189),(0x791b10,0x7f),(0x77f090,5),(0x77f0a0,0x4d),(0x77ffc0,0x17),(0x389d918,0xed*4))},
        cases=len(cases),native_explicit_boundaries=explicit,source_outcomes=outcomes,mismatch_count=len(bad),mismatches=bad[:20],records_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=len(cases),explicit=explicit,outcome_kinds=len(outcomes),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
