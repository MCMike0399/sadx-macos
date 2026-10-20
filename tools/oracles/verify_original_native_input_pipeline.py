"""Native OnInput -> GetControllers against the actual patched two-image chain.

SDL event queue, raw gamepad samples, connection facts, stage and absent gameplay
camera are explicit caller fixtures. Keyboard events, KeyboardUpdate, all8 pad
polls/publications, OnInput reset/demo predicate, full WriteAnalogs + real Input
return-hook and full GetControllers execute actual instructions. No analog stub.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_EIP,UC_X86_REG_MXCSR,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from verify_original_native_input import Source,KEYS,DATA,STACK,STOP,get,put,SHA
GAME_SHA='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
def key_map(image):
    m=Source(image,KEYS).m;m.reg_write(UC_X86_REG_EBP,STACK);m.reg_write(UC_X86_REG_ESP,STACK-0x354);m.emu_start(0x1000105e,0x1000131e,count=1000)
    values=struct.unpack('<204I',m.mem_read(STACK-0x340,816));return dict(zip(values[::2],values[1::2]))
class Pipeline:
    def __init__(self,game,image,mapping):
        m=self.m=Source(image,KEYS,game,mapping).m
        def ensure(page):
            if not any(low<=page<=high for low,high,_ in m.mem_regions()):m.mem_map(page,4096)
        for section in game.sections:
            if section['name'] not in ('.text','.rdata'):continue
            size=(section['raw_size']+4095)&~4095;address=game.base+section['rva']
            for offset in range(0,size,4096):ensure(address+offset)
            m.mem_write(address,game.slice(section['file_offset'],section['raw_size']))
        for page in (0x909000,0x3b22000,0x3b2c000,0x3abd000,0x3b2a000):ensure(page)
        for address in (0x1000e050,0x1000e054,0x1000e058,0x1000e048,0x1000e04c):ensure(get(m,address,4)&~4095)
        m.mem_write(0x909f94,game.read(0x909f94,28))
        m.mem_write(0x40f30c,struct.pack('<I',0x1000e73c));m.mem_write(0x40fe0d,struct.pack('<I',0x1000e73c));m.mem_write(0x40fe2f,struct.pack('<I',0x1000e73d))
        m.mem_write(0x40f343,b'\xe9'+struct.pack('<i',0x10004b70-(0x40f343+5)))
        m.mem_write(0x1000e73c,b'\x01\x01'+b'\x00'*6)
        for index in range(8):
            pad=0x1000e950+index*96
            put(m,pad+0x4c,1,index==0);put(m,pad+0x4e,2,7849);put(m,pad+0x50,1,1);put(m,pad+0x52,2,8689);put(m,pad+0x55,1,30)
            if index<4:put(m,0x3b0e77c+4*index,4,0x1000e748+52*index)
        # SDL discovery is supplied as a connection fact; all raw reads still use actual DLL calls.
        self.axes=[0]*6;self.buttons=0;self.connected=False
        for pointer,entry,code in [(0x1000e360,STOP+0x400,b'\x31\xc0\xc3'),(0x1000e364,STOP+0x410,b'\xc3'),(0x1000e354,STOP+0x420,b'\xc3'),(0x1000e358,STOP+0x430,b'\xc3')]:
            put(m,pointer,4,entry);m.mem_write(entry,code)
        def sample(machine,address,size,user):
            esp=machine.reg_read(UC_X86_REG_ESP);index=get(machine,esp+8,4)
            machine.reg_write(UC_X86_REG_EAX,(self.axes[index]&65535) if address==STOP+0x420 else ((self.buttons>>index)&1))
        m.hook_add(unicorn.UC_HOOK_CODE,sample,begin=STOP+0x420,end=STOP+0x430)
        m.reg_write(UC_X86_REG_FPCW,0x7f);m.mem_write(STOP+0x500,b'\xdb\xe2');m.emu_start(STOP+0x500,STOP+0x502,count=1)
    def call(self,entry):
        put(self.m,STACK,4,STOP);self.m.reg_write(UC_X86_REG_ESP,STACK);self.m.emu_start(entry,STOP,count=50000)
        if self.m.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'Source call incomplete:{entry:x}')
    def run(self,trace):
        m=self.m;output=[]
        for operation in trace:
            kind,key,down,connected,buttons,*rest=operation
            if kind==0:m.reg_write(UC_X86_REG_ECX,key);m.reg_write(UC_X86_REG_EDX,down);self.call(0x10005170)
            elif kind==1:m.emu_start(0x10005852,0x100058fa,count=500)
            self.axes=operation[5:11];self.buttons=buttons
            if connected and not self.connected:put(m,0x1000e954,4,0x3f0fff)
            self.connected=bool(connected);put(m,0x1000e98c,1,connected);put(m,0x1000e984,4,1 if connected else 0)
            put(m,0x3abdc7c,4,operation[11]);put(m,0x909fb0,1,operation[12]);put(m,0x3b22dcc,2,operation[13]);put(m,0x3b2a2e4,4,operation[14]);put(m,get(m,0x1000e060,4),4,operation[15]);put(m,0x3b2a2e8,4,-1)
            failed=bool(operation[14] and operation[15])
            if failed:
                put(m,STACK,4,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x10006510,0x10006595,count=50000)
                if m.reg_read(UC_X86_REG_EIP)!=0x10006595:raise RuntimeError('Expected explicit source demo boundary')
            else:self.call(0x10006510);self.call(0x40fdc0)
            values=[int(failed),get(m,0x3b0eaa0,1)]
            for base in (0x3b0e3a8,0x3b0e354,0x3b0e7e0):values += [get(m,base+4*i,4) for i in range(4)]
            values += list(struct.unpack('<104I',m.mem_read(0x3b0e7f0,416)))
            values += list(struct.unpack('<16I',m.mem_read(0x3b0e7a0,64)))
            values += [get(m,address,4) for address in(0x909f94,0x909f98,0x909fa4,0x909fa8,0x909fac)]
            values += [m.reg_read(UC_X86_REG_MXCSR),m.reg_read(UC_X86_REG_FPSW)&63]
            output.append(values)
        return output

def fixtures(seed):
    def base():return [2,0,0,0,0]+[0]*6+[8,1,0,0,0]
    traces=[]
    # Keyboard holds/edges, source focus ramp, triggers, halfpress and fullsource pad-read gate.
    for key in (88,13,39,37,38,40,81,87,160,115):
        t=[]
        for index in range(9):
            c=base();c[:3]=[0,key,1] if index==0 else ([0,key,0] if index==7 else [2,0,0]);c[12]=int(index not in(3,4));c[13]=0x27 if index%2 else 0;t.append(c)
        traces.append(t)
    repeat=[]
    for index in range(70):
        c=base();c[:3]=[0,39,1] if index==0 else ([0,39,0] if index==68 else [2,0,0]);repeat.append(c)
    traces.append(repeat)
    combo=(1<<0)|(1<<1)|(1<<2)|(1<<3)
    for mode in (1,4,8):
        t=[]
        for buttons in(combo|(1<<6),combo):c=base();c[3]=1;c[4]=buttons;c[11]=mode;t.append(c)
        traces.append(t)
    rng=random.Random(seed)
    for _ in range(20):
        t=[]
        for _ in range(20):
            c=base();c[:3]=[rng.choice([0,1,2]),rng.choice(KEYS),rng.randrange(2)];c[3]=rng.randrange(2);c[4]=rng.getrandbits(15);c[5:9]=[rng.randint(-32768,32767)&65535 for _ in range(4)];c[9:11]=[rng.randrange(32768) for _ in range(2)];c[12]=rng.randrange(2);t.append(c)
        traces.append(t)
    c=base();c[14:16]=[1,1];traces.append([c])
    return traces

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('input_dll',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--seed',type=int,default=20261021);a=p.parse_args()
    game,image=PEImage(a.exe),PEImage(a.input_dll)
    if game.sha256!=GAME_SHA or image.sha256!=SHA or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned identity/version mismatch')
    probe=ctypes.CDLL(str(a.native_library.resolve())).sadx_native_input_pipeline;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32)];probe.restype=ctypes.c_int
    mapping=key_map(image);bad=[];flags=[0,0];count=0;traces=fixtures(a.seed)
    for n,trace in enumerate(traces):
        expected=Pipeline(game,image,mapping).run(trace);operations=(ctypes.c_uint32*(16*len(trace)))(*[v&0xffffffff for c in trace for v in c]);output=(ctypes.c_uint32*(141*len(trace)))()
        if not probe(operations,len(trace),output):raise RuntimeError('Native pipeline failed')
        for index,source in enumerate(expected):
            actual=list(output[index*141:(index+1)*141]);count+=1
            if actual[:139]!=source[:139]:bad.append({'trace':n,'frame':index,'fields':[i for i in range(139) if actual[i]!=source[i]],'source':source,'native':actual})
            for flag in range(2):flags[flag]+=actual[139+flag]!=source[139+flag]
    r={'cases':count,'traces':len(traces),'seed':a.seed,'value_mismatches':len(bad),'mxcsr_mismatches':flags[0],'x87_exception_mismatches':flags[1],'mismatches':bad[:12],'values_match':not bad,'passed':not bad and not any(flags),'scope':__doc__}
    a.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({k:r[k] for k in ['cases','value_mismatches','mxcsr_mismatches','x87_exception_mismatches']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
