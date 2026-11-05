"""E102Display arm call and reached ordinary tail, with ordered numeric-leaf receipts.

The unchanged ordinary-body judge retains its pre-arm digest. This adds source
480135..48071b. Numeric matrix, normalization and sine implementations are common
seams; tail stores, branches and integer/FP expressions execute as source code.
"""
import argparse, ctypes, hashlib, json, random, struct
from pathlib import Path
from verify_original_gamma_object_callback import Source as BaseSource, BASE, STACK, STOP, packed, bits
from unicorn.x86_const import *

EXTRA, PLAYER, PRIMARY, EXPORTS = BASE+0x100, BASE+0x300, BASE+0x500, BASE+0x600
MATERIALS = BASE+0x800
VECTOR = 0x3c545fc
MATRIX = {0x3c53ad8:0, 0x3c53b20:1, 0x3c53bb8:2, 0x3c53c00:3}
OFFSETS = [148,160,52,64,84,96]

def fixtures(count, seed):
    rng=random.Random(seed)
    for i in range(count):
        case=[bits(rng.uniform(0,20)), BASE+0x900, rng.getrandbits(32), 0,0,0,0]
        case += [rng.choice([0,65536,32768,65535,1]) for _ in range(3)]
        case += [bits(rng.uniform(-100,100)) for _ in range(9)]
        case += [rng.choice([0,21,16,25]),rng.getrandbits(32),bits(rng.uniform(-1,1)),0,bits(0)]
        # All ordinary reached paths, signed mode boundaries and explicit unrecovered paths.
        if i%12==0: case[3]=1024
        if i%12==1: case[4]=2
        if i%12==2: case[5]=3
        if i%12==3: case[6]=1
        if i%12==4: case[4]=255
        if i%12==5: case[5]=255
        if i%12==6: case[22],case[23]=69,bits(40)
        if i%12==7: case[22],case[23]=69,0x7fc12345
        yield case

class Source(BaseSource):
    def event(self, tag,a=0,b=0,c=0,d=0): self.trace.extend([tag,a,b,c,d])
    def instruction(self,m,address,size,user):
        sp=m.reg_read(UC_X86_REG_ESP)
        if address==0x407020:
            action,frame=self.read(sp+4,2); self.event(16,frame,action); self.ret(0); return
        if address==0x4b81f0:
            base,action,frame,node,dest=self.read(sp+4,5)
            assert base==0x389d650 and node==2 and dest==0x3c53bb8
            self.event(17,frame,action,node); self.ret(0); return
        if address==0x785140: self.event(2); self.ret(0); return
        if address==0x7850f0:
            ptr=m.reg_read(UC_X86_REG_ECX)
            self.event(1,0 if ptr==0 else 1 if ptr==0x389d650 else 2)
            self.ret(0); return
        if address in (0x784ee0,0x784e20,0x784fb0):
            self.event(3,{0x784ee0:0,0x784e20:1,0x784fb0:2}[address],m.reg_read(UC_X86_REG_EDX))
            self.ret(0); return
        if address==0x785070:
            self.event(4,*self.read(m.reg_read(UC_X86_REG_EDX),3)); self.ret(0); return
        if address==0x788ad0:
            self.event(5,MATRIX[m.reg_read(UC_X86_REG_EDX)]); self.ret(0); return
        if address in (0x781670,0x784d60):
            self.event(6 if address==0x781670 else 7,*self.read(m.reg_read(UC_X86_REG_EDX),3))
            m.mem_write(self.read(sp+4)[0],packed([bits(x) for x in
                ((1.25,-2.5,3.75) if address==0x781670 else (-4,5,-6))]))
            self.ret(4); return
        if address==0x418ac0:
            dest=self.read(sp+4)[0]
            self.event(8,*self.read(dest,3)); m.mem_write(dest,packed([bits(.25),bits(.5),bits(.75)]))
            m.reg_write(UC_X86_REG_EIP,STOP+0x110); return
        if address==0x414d70:
            self.event(9); m.reg_write(UC_X86_REG_EAX,int(self.case[19] in (16,25))); self.ret(0); return
        if address==0x4399d0: self.event(10); self.ret(0); return
        if address==0x788290:
            self.event(11,m.reg_read(UC_X86_REG_ECX))
            # Execute an FLD m32/RET thunk for a common returned x87 sine value.
            m.reg_write(UC_X86_REG_EIP,STOP+0x100); return
        if address in (0x439540,0x4128f0,0x4395d0,0x412420):
            tag={0x439540:12,0x4128f0:13,0x4395d0:14,0x412420:15}[address]
            self.event(tag); self.ret(0); return
        if address in (0x480194,0x4802d8,0x480441,0x48051d,0x480696,0x48070b,0x48071b):
            self.stops={0x480194:1,0x4802d8:2,0x480441:3,0x48051d:3,0x480696:4,0x48070b:5,0x48071b:0}[address]
            m.emu_stop(); return
        super().instruction(m,address,size,user)
    def run(self,case):
        self.case=case;self.trace=[];self.stops=0;m=self.m
        m.mem_write(BASE,bytes(4096));m.mem_write(VECTOR,packed(case[16:19]))
        m.mem_write(EXTRA,bytes([case[5]&255,case[4]&255,0,0]))
        m.mem_write(EXTRA+18,struct.pack('<h',case[6]))
        m.mem_write(EXTRA+132,packed([case[0],case[20],case[2],0]))
        m.mem_write(PLAYER+4,struct.pack('<H',case[3]))
        m.mem_write(PLAYER+292,packed([case[22]]));m.mem_write(PLAYER+304,packed([case[23]]))
        m.mem_write(PRIMARY+20,packed(case[7:10]+case[10:13]))
        m.mem_write(EXPORTS+200,packed([case[1]]));m.mem_write(0x3ab916c,packed([EXPORTS]))
        m.mem_write(0x3aad0ec,packed([MATERIALS]));m.mem_write(MATERIALS+4,packed([BASE+0xa00]))
        m.mem_write(STACK,bytes(256));m.mem_write(STACK+0x30,packed(case[13:16]))
        m.mem_write(BASE+0xf00,packed([case[21]]))
        m.mem_write(STOP+0x100,b'\xd9\x05'+struct.pack('<I',BASE+0xf00)+b'\xc3')
        m.mem_write(STOP+0x110,b'\xd9\xe8\xc3')
        for reg,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_ESI,EXTRA),(UC_X86_REG_EBX,PLAYER),
                          (UC_X86_REG_EBP,PRIMARY),(UC_X86_REG_FPCW,127),(UC_X86_REG_FPSW,0),
                          (UC_X86_REG_FPTAG,65535)):
            m.reg_write(reg,value)
        m.emu_start(0x480135,STOP,count=5000)
        assert m.reg_read(UC_X86_REG_EIP) in (0x480194,0x4802d8,0x480441,0x48051d,0x480696,0x48070b,0x48071b)
        result=[self.stops,self.read(EXTRA+140)[0],self.read(EXTRA+136)[0],self.read(PLAYER+272)[0],
                m.mem_read(BASE+0xa00+123,1)[0],*self.read(VECTOR,3)]
        for offset in OFFSETS:result+=self.read(EXTRA+offset,3)
        return result+[len(self.trace)]+self.trace+[m.reg_read(UC_X86_REG_FPSW)&63]

def main():
    from pe_image import PEImage
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--cases',type=int,default=1024)
    p.add_argument('--seed',type=lambda x:int(x,0),default=0x480135);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    source=Source(PEImage(a.exe));lib=ctypes.CDLL(str(Path(a.library).resolve()));fn=lib.gamma_tail_probe
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_uint32
    failures=[];partials=0;digest=hashlib.sha256()
    for i,case in enumerate(fixtures(a.cases,a.seed)):
        expected=source.run(case);out=(ctypes.c_uint32*1024)();n=fn((ctypes.c_uint32*len(case))(*case),out);actual=list(out[:n]);digest.update(packed(expected))
        if expected[:-1]!=actual[:-1]:failures.append(dict(index=i,case=case,expected=expected,actual=actual))
        elif expected[-1]!=actual[-1]:partials+=1
    report=dict(source_sha256=source.image_sha256 if hasattr(source,'image_sha256') else hashlib.sha256(Path(a.exe).read_bytes()).hexdigest(),
                source_ranges={'0x480135-0x48071b':hashlib.sha256(PEImage(a.exe).read(0x480135,0x48071b-0x480135)).hexdigest()},
                cases=a.cases,seed=a.seed,value_failures=len(failures),values_match=not failures,fp_partials=partials,flag_mismatches=partials,
                status="fail" if failures else "partial" if partials else "pass",passed=not failures and not partials,
                failures=failures[:3],observations_sha256=digest.hexdigest(),
                limits=['Ordered common matrix/normalize/sine leaves; masked exceptions only; no pixels or numeric-leaf parity.'])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    return 1 if failures else 2 if partials else 0
if __name__=='__main__':raise SystemExit(main())
