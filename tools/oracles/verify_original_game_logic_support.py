"""Actual40ec80/413920 instructions with synthetic raw bytes/borrowed clocks.
Missing-fact profiles stop immediately before the source read; they do not
pretend source memory itself has C++ optional values. #DE is a local boundary,
not a reconstructed Windows exception dispatcher.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_ninja_texture import Original,STACK,STOP
RAW=0x3b0e3e2;AGG=0x3b0eaf0;SPECIAL=0x3b0e3ba;AGGSPECIAL=0x3b0eae8
TIMING=[0x3b11178,0x3b1117c,0x3b11180,0x3b11188,0x3b1118c,0x3b11198,0x3b2a2e4]
sha=lambda b:hashlib.sha256(b).hexdigest()
def signed(v):return v if v<0x80000000 else v-0x100000000
class Source(Original):
    def __init__(self,image):
        super().__init__(image);self.clean_cpu=self.m.context_save();self.kind=None;self.present=[];self.known=[];self.boundary=0;self.fault=False;self.trace={};self.last=0
        self.m.hook_add(unicorn.UC_HOOK_CODE,self.code);self.m.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.write);self.m.hook_add(unicorn.UC_HOOK_INTR,self.intr)
    def code(self,m,address,size,user):
        self.last=address;index=None
        if self.kind=='ticks':
            reads={0x413920:5,0x413929:2,0x41392f:3,0x41393b:0,0x413950:6};index=reads.get(address)
            if address==0x413935:self.trace['quotient']=m.reg_read(UC_X86_REG_EAX)
            if address==0x41393b:self.trace['incremented']=m.reg_read(UC_X86_REG_EAX)
            if address==0x413942:self.trace['product']=m.reg_read(UC_X86_REG_EAX)
            if address==0x413959:self.trace['clamped']=True
        elif self.kind=='edges':
            eax=m.reg_read(UC_X86_REG_EAX);ecx=m.reg_read(UC_X86_REG_ECX)
            if address==0x40ede2:index=(eax-RAW)
            r1=[0x40ed35,0x40ed40,0x40ed50,0x40ed61,0x40ed72,0x40ed83,0x40ed94,0x40eda5]
            a1=[0x40ed38,0x40ed48,0x40ed59,0x40ed6a,0x40ed7b,0x40ed8c,0x40ed9d,0x40edae]
            r2=[0x40eca9,0x40ecb7,0x40ecc3,0x40ecd1,0x40ecdf,0x40eced,0x40ecfb,0x40ed09]
            a2=[0x40eca3,0x40ecae,0x40ecbb,0x40ecc8,0x40ecd6,0x40ece4,0x40ecf2,0x40ed00]
            if address in r1:index=ecx-RAW-3+r1.index(address)*3
            if address in a1:index=768+eax+a1.index(address)
            if address in r2:index=eax-RAW-3+r2.index(address)*3
            if address in a2:index=768+ecx+a2.index(address)
            if address in (0x40edf5,0x40edcc,0x40ed19):index=1024
            if address in (0x40edc6,0x40ed1f):index=1025
        if index is not None and not self.present[index]:self.boundary=address;m.emu_stop()
    def write(self,m,access,address,size,value,user):
        if self.kind=='ticks' and address in TIMING:self.known[TIMING.index(address)]=1
        if self.kind=='edges':
            if RAW<=address<RAW+768:self.known[address-RAW]=1
            if AGG<=address<AGG+256:self.known[768+address-AGG]=1
            if address==SPECIAL:self.known[1024]=1
            if address==AGGSPECIAL:self.known[1025]=1
    def intr(self,m,number,user):
        if self.kind=='ticks' and number==0:self.fault=True;self.boundary=self.last;m.emu_stop()
        else:raise RuntimeError(f'Unexpected source interrupt{number}')
    def invoke(self,address,args):
        # Reset caller CPU context per experiment, including prior #DE state.
        self.m.context_restore(self.clean_cpu)
        self.m.mem_write(STACK,struct.pack('<'+str(len(args)+1)+'I',STOP,*[v&0xffffffff for v in args]));self.m.reg_write(UC_X86_REG_ESP,STACK);self.boundary=0;self.fault=False;self.trace={};self.m.emu_start(address,STOP,count=20000)
        if not self.boundary and self.m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Original leaf did not return')
    def edges(self,data,present,mode):
        self.kind='edges';self.present=list(present);self.known=list(present);self.m.mem_write(RAW,data[:768]);self.m.mem_write(AGG,data[768:1024]);self.m.mem_write(SPECIAL,data[1024:1025]);self.m.mem_write(AGGSPECIAL,data[1025:1026]);self.invoke(0x40ec80,[mode])
        return bytes(self.m.mem_read(RAW,768))+bytes(self.m.mem_read(AGG,256))+bytes(self.m.mem_read(SPECIAL,1))+bytes(self.m.mem_read(AGGSPECIAL,1))
    def ticks(self,words,present):
        self.kind='ticks';self.present=list(present);self.known=list(present)
        for address,value in zip(TIMING,words):self.m.mem_write(address,struct.pack('<I',value&0xffffffff))
        self.invoke(0x413920,[]);return [struct.unpack('<I',self.m.mem_read(address,4))[0] for address in TIMING]
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity changed')
    lib=ctypes.CDLL(str(a.library.resolve()));edge=lib.sadx_native_game_edges;edge.argtypes=[ctypes.c_int32]+[ctypes.POINTER(ctypes.c_uint8)]*4+[ctypes.POINTER(ctypes.c_uint64)];edge.restype=ctypes.c_int
    ticks=lib.sadx_native_game_ticks;ticks.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint64)];ticks.restype=ctypes.c_int
    lib.sadx_native_game_logic_support_compiler.restype=ctypes.c_char_p;source=Source(image);rng=random.Random(0x40ec80)
    report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_game_logic_support_compiler().decode(),original_bytes_sha256={hex(at):sha(image.read(at,n)) for at,n in [(0x40ec80,0x181),(0x413920,0x5b)]},edges=dict(cases=0,mismatches=0),ticks=dict(cases=0,mismatches=0,divide_faults=0),first_mismatches=[],limits=['Synthetic raw source bytes and integer timing globals, not observed Win32/input defaults','Missing native facts modeled by a source-read stop before instruction, preserving actual earlier stores','Local IDIV #DE boundaries only; Windows exception dispatcher remains unimplemented','No physical keyboard/HID/controller namespace equivalence inferred'])
    case_hash=hashlib.sha256()
    def edgecase(data,present,mode):
        expected=source.edges(data,present,mode);actual=(ctypes.c_uint8*1026)();known=(ctypes.c_uint8*1026)();d=(ctypes.c_uint64*10)()
        if edge(mode,(ctypes.c_uint8*1026).from_buffer_copy(data),(ctypes.c_uint8*1026).from_buffer_copy(present),actual,known,d):raise RuntimeError('Native edge probe failure')
        different=list(known)!=source.known or any(expected[i]!=actual[i] for i in range(1026) if source.known[i]) or d[1]!=source.boundary or bool(d[2])!=(not bool(source.boundary))
        report['edges']['cases']+=1;report['edges']['mismatches']+=different;case_hash.update(struct.pack('<I',mode&0xffffffff)+data+present)
        if different and len(report['first_mismatches'])<8:report['first_mismatches'].append(dict(kind='edge',mode=mode,boundary=hex(source.boundary),native=list(d),missing=[i for i,v in enumerate(present) if not v]))
    for mode in [0,1,2,-1,3,4,-2147483648,2147483647]:
        for n in range(24):edgecase(bytes(rng.randrange(256) for _ in range(1026)),bytes([1])*1026,mode)
    data=bytes(rng.randrange(256) for _ in range(1026))
    for mode in [0,1,2]:
        for index in [*range(0,768,3),*range(768,1026)]:
            present=bytearray([1]*1026);present[index]=0;edgecase(data,bytes(present),mode)
        present=bytearray([1]*1026)
        for i in range(256):present[i*3+1]=present[i*3+2]=0
        edgecase(data,bytes(present),mode)
    def tickcase(words,present):
        expected=source.ticks(words,present);actual=(ctypes.c_int32*7)();known=(ctypes.c_uint8*7)();d=(ctypes.c_uint64*10)();status=ticks((ctypes.c_int32*7)(*map(signed,[v&0xffffffff for v in words])),(ctypes.c_uint8*7).from_buffer_copy(present),actual,known,d)
        if status:raise RuntimeError('Native tick probe failure')
        different=list(known)!=source.known or any((actual[i]&0xffffffff)!=expected[i] for i in range(7) if source.known[i]) or d[1]!=source.boundary or bool(d[2])!=(not bool(source.boundary))
        for key,at,mask in [('quotient',5,1),('incremented',6,2),('product',7,4)]:
            if bool(d[8]&mask)!=(key in source.trace) or key in source.trace and d[at]!=source.trace[key]:different=True
        if source.fault:report['ticks']['divide_faults']+=1;different|=d[0]!=(6 if words[3]==0 else 7)
        report['ticks']['cases']+=1;report['ticks']['mismatches']+=different;case_hash.update(struct.pack('<7I',*[v&0xffffffff for v in words])+present)
        if different and len(report['first_mismatches'])<8:report['first_mismatches'].append(dict(kind='tick',input=words,present=list(present),boundary=hex(source.boundary),source_values=expected,native_values=list(actual),native=list(d),trace=source.trace))
    extreme=[0,1,-1,6,7,-7,2147483647,-2147483648]
    for enabled in [0,1,-1]:
        for dividend in extreme:
            for divisor in extreme:
                for factor in extreme:
                    for override in [0,1]:tickcase([factor,0x13579bdf,dividend,divisor,0x2468ace0,enabled,override],bytes([1])*7)
    for n in range(1024):tickcase([rng.getrandbits(32) for _ in range(7)],bytes([1])*7)
    for words in [[1,10,50,1,20,1,0],[1,10,50,1,20,0,0],[1,10,-50,1,20,1,0]]:
        for missing in range(7):present=bytearray([1]*7);present[missing]=0;tickcase(words,bytes(present))
    root=pathlib.Path(__file__).resolve().parents[2];report['source_files_sha256']={v:sha((root/v).read_bytes()) for v in ['src/engine/game_logic_support.hpp','src/guest/game_logic_support.cpp','tests/probes/native_game_logic_support_probe.cpp','tools/oracles/verify_original_game_logic_support.py']};report['case_sha256']=case_hash.hexdigest();report['passed']=not(report['edges']['mismatches'] or report['ticks']['mismatches']);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['edges','ticks','passed','first_mismatches']}),flush=True);return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
