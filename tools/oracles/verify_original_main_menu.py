"""Full original MainMenu init/phase/control/background entries; service calls are ordered receipts."""
import argparse, ctypes, hashlib, json, os, random, struct, sys
from pathlib import Path

ROOT = Path(os.environ.get('SADX_SOURCE_ROOT', Path(__file__).resolve().parents[2]))
sys.path[:0] = [str(ROOT/'tools/lib'), str(ROOT/'tools/oracles')]
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI, UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original, DATA, STACK, STOP

WORK, TASK, SEQ_TASK, SEQ, CANON, CANON_TASK, TITLE, TITLE_TASK = [DATA+n*4096 for n in range(1,9)]
SAVE, BACKUP = 0x3b2b3a8, 0x3b29d70
GLOBALS = [(0x3b0f0e8,16,4),(0x3b0ef38,17,4),(0x3b2a2fa,18,1),(0x3b2a2fc,19,1),
           (0x3b1118c,15,4),(0x3c5e8e0,37,4),(0x3b2a2f1,38,1),(0x3abdf7c,39,4)]
CALLS = {0x5068a0:1,0x506840:2,0x506010:3,0x505990:4,0x506e10:5,0x506e40:6,
         0x50b630:7,0x506040:8,0x433170:9,0x433010:10,0x432f40:11,0x403070:12,
         0x433f80:13,0x506780:21,0x506410:22,0x506460:23,0x5057f0:24,0x5057d0:24,
         0x432550:25,0x5061f0:26,0x506210:27,0x505e60:28,0x505b40:29,0x5058b0:30,
         0x4325b0:31,0x4325c0:32,0x432580:33,0x506620:34,0x643ea7:0}

class Source:
    def __init__(self, image):
        self.m = Original(image,127).machine
        for address in [a for a,_,_ in GLOBALS]+[0x3c5e8d0,0x3c5fee4,0x3c5ff00,0x2bcbddc,
                0x3b2c5e4,0x8928c0,SAVE,SAVE+1391,BACKUP,BACKUP+1391]:
            page=address&~4095
            if not any(a<=page<=b for a,b,_ in self.m.mem_regions()):self.m.mem_map(page,4096)
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.hook)
        self.initial=self.m.context_save()
    def word(self,address,n=4):return int.from_bytes(self.m.mem_read(address,n),'little')
    def put(self,address,value,n=4):self.m.mem_write(address,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
    def hook(self,m,address,size,user):
        if address not in CALLS:return
        if address==0x50b630 and self.v[0]==4:return
        kind=CALLS[address];sp=m.reg_read(UC_X86_REG_ESP);result=0;arg=self.word(sp+4)
        if kind in (1,2):
            index=self.reads[kind];self.reads[kind]+=1
            result=self.v[(24 if kind==1 else 28)+index%4]&255
            signed=result if result<128 else result-256
            self.trace.extend([kind,signed&0xffffffff])
        elif kind in (3,4,8,10,11,28,29):self.trace.extend([kind,arg])
        elif kind in (5,6):
            self.trace.extend([kind,arg]);result=arg^(0x7f123456 if kind==5 else 0x40123456)
        elif kind==7:self.trace.extend([kind,0]);self.put(WORK+28,self.v[32])
        elif kind in (9,32):
            self.trace.extend([kind,0])
            if kind==32:assert arg==0
        elif kind==12:
            assert arg in (0x10d7c78,0x10d7c80)
            self.trace.extend([12,0 if arg==0x10d7c78 else 1])
        elif kind==13:self.trace.extend([13]+[self.word(sp+n) for n in (4,8,12,16,20,24)])
        elif kind in (21,22,23,25,26,27,30,31,33):
            index={21:40,22:41,23:42,25:43,26:44,27:45,30:46,31:47,33:48}[kind]
            result=self.v[index];self.trace.extend([kind,result&65535 if kind==31 else result])
        elif kind==24:
            disabled=[];p=self.word(sp+12)
            for i in range(3):
                value=self.word(p+i,1);disabled.append(value)
                if value==255:break
            disabled += [255]*(3-len(disabled))
            packed=sum(v<<(8*i) for i,v in enumerate(disabled))
            self.trace.extend([24,arg,self.word(sp+8)&255,packed,int(address==0x5057f0)])
        elif kind==34:
            self.trace.extend([34,self.word(SAVE+601,1)])
            if self.v[49]!=0xffffffff:self.put(SAVE,self.v[49])
        m.reg_write(UC_X86_REG_EAX,result);m.reg_write(UC_X86_REG_EIP,self.word(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
    def run(self,v):
        self.v=v;self.trace=[];self.reads={1:0,2:0};self.m.context_restore(self.initial)
        self.m.mem_write(WORK,struct.pack('<10I',*v[1:11]));self.m.mem_write(SEQ,bytes(40))
        self.put(TASK+44,WORK);self.put(SEQ_TASK+44,SEQ);self.put(CANON_TASK+44,CANON)
        self.put(TITLE_TASK+44,TITLE);self.put(0x3c5e8d0,SEQ_TASK)
        self.put(0x3c5fee4,TASK if v[22] else CANON_TASK);self.put(0x3c5ff00,TITLE_TASK)
        self.put(CANON+4,v[21]);self.put(TITLE+8,v[20])
        self.put(SEQ+4,v[11]);self.put(SEQ+8,v[12]);self.put(SEQ+12,v[13]);self.put(SEQ+16,v[23]);self.put(SEQ+30,v[14])
        for a,i,n in GLOBALS:self.put(a,v[i],n)
        self.put(0x8928c0,v[34]);self.put(0x8928c4,v[35]);self.put(0x3b2c5e4,v[36])
        self.m.mem_write(SAVE,bytes((j*17+v[50])&255 for j in range(1392)))
        self.m.mem_write(BACKUP,bytes((j*29+v[51])&255 for j in range(1392)))
        from unicorn.x86_const import UC_X86_REG_FPCW
        self.m.reg_write(UC_X86_REG_FPCW,v[33])
        op=v[0];entry=(0x50bd20,0x50bde0,0x50ba90,0x50bcb0,0x50b630,0x50bcb0)[op]
        arg=v[5] if op==2 else WORK if op==4 else TASK
        self.m.mem_write(STACK-256,struct.pack('<2I',STOP,arg));self.m.reg_write(UC_X86_REG_ESP,STACK-256)
        self.m.reg_write(UC_X86_REG_ESI,WORK)
        self.m.emu_start(entry,STOP,timeout=0);assert self.m.reg_read(UC_X86_REG_EIP)==STOP
        out=[0]*1024;out[:10]=struct.unpack('<10I',self.m.mem_read(WORK,40))
        out[10:20]=[self.word(0x3b0f0e8),self.word(0x3b0ef38),self.word(0x3b2a2fa,1),self.word(0x3b2a2fc,1),
            self.word(TITLE+8),self.word((WORK if v[22] else CANON)+4),self.word(SEQ+12),self.word(SEQ+16),self.word(SEQ+30),self.word(0x3b2c5e4)]
        if op==4:out[26:28]=[self.word(0x3b2a2f1,1),self.word(0x3abdf7c)]
        out[31]=self.m.reg_read(UC_X86_REG_FPSW)&63;out[32]=len(self.trace);out[40:40+len(self.trace)]=self.trace
        out[128:476]=struct.unpack('<348I',self.m.mem_read(SAVE,1392));out[476:824]=struct.unpack('<348I',self.m.mem_read(BACKUP,1392))
        return out

def cases(seed,count):
    rng=random.Random(seed);rows=[]
    def base(op):
        v=[0]*64;v[0]=op;v[1]=0;v[2:5]=[5,1,2];v[5]=0x45a5a000;v[7]=0xffffffff;v[9]=0;v[10]=0x00570011
        v[11:14]=[6,0,14];v[14]=0x12345678;v[15]=1;v[16:24]=[1,1,0,255,2,9,1,101]
        v[24:32]=[1]*8;v[32]=6;v[33]=127;v[34:36]=[0x3f800000]*2;v[36]=99;v[37]=1;v[38]=1
        v[40:49]=[1,1,1,0,1,1,5,480,1];v[49]=0xffffffff;v[50:52]=[17,31];v[52:54]=[640,480];return v
    for phase in (0,1,2,3,4,0xffffffff):
        for ticks in (0,1,2,10,0x7fffffff,0x80000000,0xffffffff):
            for value in (0,0x3f000000,0x3f666666,0x3f800000,0x7fc01234):
                v=base(1);v[1]=phase;v[6]=value;v[15]=ticks;rows.append(v)
    for stage in range(8):
        for choice in (-128,-1,0,1,2,3,4,5,6):
            for optional in (0,1):
                if choice==6 and optional==0 and stage in (0,1):continue
                for ready in (0,1):
                    v=base(4);v[8]=stage;v[43]=choice&0xffffffff;v[42]=optional;v[37]=ready;rows.append(v)
    for text in (-128,-1,0,1,5,127):
        for voice in (-1,0,1,5):
            for alias in (0,1):
                v=base(0);v[24:28]=[text&0xffffffff]*4;v[28:32]=[voice&0xffffffff]*4;v[22]=alias;rows.append(v)
    for op in (2,3):
        for scale in (0,0x3f000000,0x3f800000,0x3faaaaab,0xbf800000,0x7fc01234):
            for cw in (127,0x23f,0x27f,0x37f):
                v=base(op);v[34]=scale;v[35]=0x3f4ccccd;v[33]=cw;rows.append(v)
    for color in (0,1,0xffffffff):
        for cw in (127,0x23f,0x27f,0x37f):
            v=base(5);v[7]=color;v[33]=cw;rows.append(v)
    for _ in range(count):
        v=base(rng.randrange(5));v[1]=rng.randrange(6);v[6]=rng.choice((0,0x3f666666,0x3f800000,0xbf800000,0x7fc01234))
        v[8]=rng.randrange(8);v[10]=rng.getrandbits(24);v[11]=rng.randrange(8);v[12]=rng.randrange(8)
        v[14]=rng.getrandbits(32);v[15]=rng.choice((0,1,2,10,0xffffffff));v[18:20]=[rng.randrange(256),rng.randrange(256)]
        v[22]=rng.randrange(2);v[24:32]=[rng.randrange(256) for _ in range(8)];v[33]=rng.choice((127,0x23f,0x27f,0x37f))
        v[37]=rng.randrange(3);v[40:43]=[rng.randrange(2) for _ in range(3)];v[43]=rng.choice((-128,-1,0,1,2,3,4,5,6))&0xffffffff
        if v[0]==4 and v[8] in (0,1) and v[43]==6:v[42]=1
        v[44:47]=[rng.randrange(2),rng.randrange(2),rng.choice((0,4,5,6))];v[47]=rng.choice((0,479,480,32767,65535));v[48]=rng.randrange(2)
        v[49]=rng.choice((0xffffffff,rng.getrandbits(32)));v[50:52]=[rng.randrange(256),rng.randrange(256)];rows.append(v)
    return rows

def main():
    p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('library');p.add_argument('--output',required=True);p.add_argument('--seed',type=int,default=506300);p.add_argument('--cases',type=int,default=1000);a=p.parse_args()
    image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image)
    mapping=(ctypes.c_uint8*7)(*[image.read(0x7ef8e8+4*i,1)[0] for i in range(7)])
    lib=ctypes.CDLL(a.library);probe=lib.sadx_native_main_menu_probe;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2+[ctypes.POINTER(ctypes.c_uint8)]
    rows=cases(a.seed,a.cases);bad=[];flags=0;tested=0;flag_examples=[]
    for n,v in enumerate(rows):
        expected=source.run(v);out=(ctypes.c_uint32*1024)();probe((ctypes.c_uint32*64)(*v),out,mapping)
        diff=[[i,x,y] for i,(x,y) in enumerate(zip(expected,out)) if i!=31 and x!=y];flags+=expected[31]!=out[31];tested+=1
        if expected[31]!=out[31] and len(flag_examples)<8:flag_examples.append({'case':n,'operation':v[0],'control_word':v[33],'original_flags':expected[31],'native_flags':out[31]})
        if diff:
            bad.append({'case':n,'input':v,'diff':diff[:20]})
            if len(bad)==8:break
    stage=Path(__file__).resolve().parents[2]
    report={'passed':not bad and not flags,'values_match':not bad,'cases':len(rows),'tested':tested,'mismatches':len(bad),'fp_flag_mismatches':flags,'flag_examples':flag_examples,'first_mismatches':bad,'seed':a.seed,'source_sha256':image.sha256,'source_ranges':{f'0x{x:x}-0x{y:x}':hashlib.sha256(image.read(x,y-x)).hexdigest() for x,y in [(0x50b630,0x50ba90),(0x50ba90,0x50bca1),(0x50bcb0,0x50bce5),(0x50bd20,0x50bdd1),(0x50bde0,0x50bf16)]},'native_sources':{str(p.relative_to(stage)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [stage/'src/game/main_menu.hpp',stage/'src/game/main_menu.cpp',stage/'tests/probes/native_main_menu_probe.cpp']},'limits':['FP status-only partials remain visible: values and draw receipts are exact, but Unicorn2.1.4 and native X87Context disagree on status/precision flags; recorded examples identify the actual flag bits. No FP comparison is relaxed.','Original entries execute fully. Service boundaries are ordered receipts; enable predicates, dialog engine, save application, character factory, fades and renderer have separate owners. Phase op uses the control service receipt; control entry is independently judged in op4.','Producer-reachable main dialog results through cancel6 are compared, including signed negative byte cases-128 and-1. An optional-disabled synthetic result6 maps outside the imported table and remains an explicit boundary rather than guessed data.','Runtime-facing draw_profiled compares against the original body only for the pinned loader identity view640x480/scales1; other views stop at an explicit boundary after blending/base color. Loader source bgscale.cpp23 and uiscale.h scale_trampoline define this bounded qualification.','Full serialized1392byte save and backup are compared after each operation; renderer receipts compare all8texture draws, not GPU pixels.']}
    Path(a.output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad or flags)
if __name__=='__main__':raise SystemExit(main())
