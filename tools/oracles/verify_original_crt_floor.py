"""Actual CRT floor instructions versus native ARM64 source.

Both feature-selected SSE and fallback instructions execute. Unsupported CRT
error dispatchers stop at the original CALL boundary; no callback overlays.
Explicit runtime profiles are experiments, not observed Windows startup state.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct,time
import unicorn
from unicorn import UC_HOOK_CODE,UC_PROT_READ,UC_PROT_WRITE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW,UC_X86_REG_FPTAG,UC_X86_REG_MXCSR,UC_X86_REG_EAX,UC_X86_REG_EFLAGS,UC_X86_REG_EBP
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,CODE,DATA,STACK,STOP

def sha(b):return hashlib.sha256(b).hexdigest()
def bits(x):return struct.unpack('<Q',struct.pack('<d',x))[0]

class FloorOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x37f)
        self.machine.mem_map(0x3d0f000,0x1000,UC_PROT_READ|UC_PROT_WRITE)
        self.machine.mem_map(0x2bcc000,0x1000,UC_PROT_READ|UC_PROT_WRITE)
        self.machine.mem_write(CODE,b'\x83\xc4\x08\xdb\x3d'+struct.pack('<I',DATA)+b'\xc3')
        self.boundary=0;self.pending=None;self.trace=None
        self.machine.hook_add(UC_HOOK_CODE,self.hook)
    def hook(self,m,address,size,user):
        if self.trace is not None and address in (0x649199,0x6491a9,0x6491b0):
            esp=m.reg_read(UC_X86_REG_ESP);ebp=m.reg_read(UC_X86_REG_EBP)
            self.trace.append(dict(address=hex(address),eax=hex(m.reg_read(UC_X86_REG_EAX)),eflags=hex(m.reg_read(UC_X86_REG_EFLAGS)),
                argument=bytes(m.mem_read(ebp+8,8)).hex(),stored_argument=bytes(m.mem_read(esp,8)).hex()))
        if address in (0x644aa5,0x6491cb,0x64923f):
            self.boundary=address;esp=m.reg_read(UC_X86_REG_ESP)
            slot=esp+(20 if address==0x644aa5 else 4 if address==0x6491cb else 16)
            self.pending=struct.unpack('<Q',m.mem_read(slot,8))[0];m.emu_stop()
    def run(self,u,cw,mxcsr,cpu,fallback):
        self.boundary=0;self.pending=None
        self.trace=[] if cpu==0 and cw==0x37f and ((mxcsr==0x1f80 and u==0x7ff0000000000123) or (mxcsr==0x9f80 and u in (0x8000000000000001,0x800fffffffffffff))) else None
        m=self.machine;m.mem_write(STACK,struct.pack('<IQI',CODE,u,STOP))
        m.mem_write(0x3d0f400,struct.pack('<I',cpu));m.mem_write(0x2bcc034,struct.pack('<I',fallback))
        m.reg_write(UC_X86_REG_FPSW,0);m.reg_write(UC_X86_REG_FPTAG,0xffff)
        m.reg_write(UC_X86_REG_FPCW,cw);m.reg_write(UC_X86_REG_MXCSR,mxcsr);m.reg_write(UC_X86_REG_ESP,STACK)
        m.emu_start(0x6449f0,STOP,count=2000)
        if not self.boundary and m.reg_read(UC_X86_REG_ESP)!=STACK+16:raise RuntimeError('CRTfloor original ABI did not complete')
        value=None if self.boundary else struct.unpack('<QH',m.mem_read(DATA,10))
        return value,self.boundary,self.pending,m.reg_read(UC_X86_REG_FPCW),m.reg_read(UC_X86_REG_FPSW)&63,m.reg_read(UC_X86_REG_MXCSR)

def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path)
    p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    if unicorn.__version__!='2.1.4':raise RuntimeError('Pinned Unicorn2.1.4 required')
    image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256:raise RuntimeError('Source identity mismatch')
    constants=image.read(0x840d90,80);buffer=(ctypes.c_ubyte*80).from_buffer_copy(constants)
    native=ctypes.CDLL(str(a.native_library.resolve())).sadx_native_crt_floor
    native.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.c_uint64,ctypes.c_uint16,
        ctypes.c_uint32,ctypes.c_uint32,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];native.restype=ctypes.c_int
    cases=list(map(bits,[0.,-0.,.25,-.25,.5,-.5,1.,-1.,1.25,-1.25,1.75,-1.75,2**52-0.5,-(2**52-0.5),2**52,-2**52,2**53,-2**53,float('inf'),-float('inf')]))
    cases += [1,0x8000000000000001,0x000fffffffffffff,0x800fffffffffffff,
        0x7ff8000000000123,0xfff8000000000123,0x7ff0000000000123,0xfff0000000000123]
    rng=random.Random(0x6449f0);cases +=[bits(rng.uniform(-1e8,1e8)) for _ in range(500)]+[rng.getrandbits(64) for _ in range(500)]
    report=dict(profile='actual-CRTfloor-feature-dispatch/source-arm64/ext80-return/no-callee-overlays',source_sha256=image.sha256,
        native_library_sha256=sha(a.native_library.read_bytes()),constants_sha256=sha(constants),case_sha256=sha(b''.join(struct.pack('<Q',u) for u in cases)),
        source_functions={hex(lo):dict(size=hi-lo,sha256=sha(image.read(lo,hi-lo))) for lo,hi in [(0x6449f0,0x644b11),(0x64917a,0x64924b),(0x64dbc9,0x64dbdc),(0x64b1fd,0x64b221),(0x64b0cd,0x64b128)]},profiles=[],
        limits=['Explicit CPU feature/CW/MXCSR/fallback global seeds are not original startup observations',
                'NaN and unmasked-precision policy routes stop before unresolved real CRT error dispatch calls',
                'Unicorn2.1.4 is not native Intel hardware; arithmetic exception flags reported separately',
                'Full path/scene/game/mod runtime and physical input are not covered'])
    # Bounded diagnostic: the original x87-only FRNDINT helper must not acquire
    # SSE FTZ semantics. Retain emulator disagreement instead of changing native.
    round_native=ctypes.CDLL(str(a.native_library.resolve())).sadx_native_round_to_integer
    round_native.argtypes=[ctypes.c_uint64,ctypes.c_uint16,ctypes.POINTER(ctypes.c_uint32)]
    round_native.restype=ctypes.c_int
    leaf=Original(image,0x173f);leaf.machine.mem_write(CODE,b'\x83\xc4\x08\xdb\x3d'+struct.pack('<I',DATA)+b'\xc3')
    diagnostics=[]
    for u in (0x8000000000000001,0x800fffffffffffff):
        for mx in (0x1f80,0x9f80):
            machine=leaf.machine;machine.mem_write(STACK,struct.pack('<IQI',CODE,u,STOP))
            machine.reg_write(UC_X86_REG_ESP,STACK);machine.reg_write(UC_X86_REG_FPSW,0);machine.reg_write(UC_X86_REG_FPTAG,0xffff)
            machine.reg_write(UC_X86_REG_FPCW,0x173f);machine.reg_write(UC_X86_REG_MXCSR,mx)
            machine.emu_start(0x64dbc9,STOP,count=100);original_value=struct.unpack('<QH',machine.mem_read(DATA,10))
            out=(ctypes.c_uint32*4)();status=round_native(u,0x173f,out)
            if status:raise RuntimeError('FRNDINT diagnostic native rejected')
            diagnostics.append(dict(input=hex(u),mxcsr=hex(mx),original_ext80=list(original_value),
                native_ext80=[out[0]|out[1]<<32,out[2]],native_flags=out[3]))
    report['frndint_leaf_diagnostics']=diagnostics
    start=time.monotonic()
    profiles=[(cw,m,cpu,0x173f) for cpu in (0,1) for cw in (pc|rc|0x7f for pc in (0,0x200,0x300) for rc in (0,0x400,0x800,0xc00)) for m in (0x1f80,0x1fc0,0x9f80,0x3f80)]
    profiles += [(0x033f,0x1f80,1,0x173f),(0x037f,0x1f00,1,0x173f)]
    original=FloorOriginal(image)
    for cw,m,cpu,fallback in profiles:
        item=dict(control_word=hex(cw),mxcsr=hex(m),cpu_sse2=cpu,fallback_control_word=hex(fallback),cases=len(cases),value_mismatches=0,boundary_mismatches=0,pending_mismatches=0,control_word_mismatches=0,x87_flag_mismatches=0,mxcsr_mismatches=0,examples=[],status_examples=[])
        for i,u in enumerate(cases):
            value,site,pending,final_cw,flags,mx=original.run(u,cw,m,cpu,fallback);out=(ctypes.c_uint32*12)()
            if original.trace is not None:report.setdefault('fallback_instruction_traces',[]).append(dict(input=hex(u),mxcsr=hex(m),instructions=original.trace))
            status=native(buffer,80,u,cw,m,cpu,fallback,out,12)
            if status:raise RuntimeError(f'Native rejectedcase{i} profile{cw:x}/{m:x}/{cpu}:status{status}')
            actual_value=(out[5]|out[6]<<32,out[7]);npending=(out[10]|out[11]<<32) if out[9] else None
            vm=value is not None and value!=actual_value;bm=site!=out[1] or bool(site)!=bool(out[0]);pm=pending!=npending
            cm=final_cw!=out[2];fm=flags!=out[3];mm=mx!=out[4]
            for key,flag in [('value_mismatches',vm),('boundary_mismatches',bm),('pending_mismatches',pm),('control_word_mismatches',cm),('x87_flag_mismatches',fm),('mxcsr_mismatches',mm)]:item[key]+=flag
            examples=item['examples'] if any((vm,bm,pm,cm)) else item['status_examples']
            if any((vm,bm,pm,cm,fm,mm)) and len(examples)<8:examples.append(dict(case=i,input=hex(u),value=[value,actual_value],boundary=[hex(site),hex(out[1])],pending=[None if pending is None else hex(pending),None if npending is None else hex(npending)],cw=[final_cw,out[2]],flags=[flags,out[3]],mxcsr=[mx,out[4]]))
        report['profiles'].append(item)
        print(f"CPU{cpu} CW{cw:#x} MXCSR{m:#x}: {len(cases)} cases, values{item['value_mismatches']} boundaries{item['boundary_mismatches']} pending{item['pending_mismatches']} CW{item['control_word_mismatches']}",flush=True)
    report['returned_values_and_boundaries_match']=all(not any(p[k] for k in ['value_mismatches','boundary_mismatches','pending_mismatches','control_word_mismatches']) for p in report['profiles'])
    report['full_emulator_match']=report['returned_values_and_boundaries_match'] and all(not p['x87_flag_mismatches'] and not p['mxcsr_mismatches'] for p in report['profiles'])
    report['duration_seconds']=time.monotonic()-start;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    if not report['returned_values_and_boundaries_match']:raise SystemExit(1)
if __name__=='__main__':main()
