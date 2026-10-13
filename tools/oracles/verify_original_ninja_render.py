"""Actual4030d0/77ec60 command production with explicit external COM fixture.

Only the SetRenderState backend boundary is seeded. No original local game
callee is substituted; this does not establish actual GPU or Windows COM use.
"""
import argparse,ctypes,json,pathlib,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from extract import SOURCE_SHA256
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP,DATA,CODE
from verify_original_task_work import call,sha
FIRST=DATA+0x100;SECOND=DATA+0x200;VTABLE=DATA+0x1000;METHOD=CODE+0x200
class RenderOriginal(Original):
    def __init__(self,image):
        super().__init__(image,0x7f);m=self.machine
        for p in(0x3ab9000,0x3d12000):m.mem_map(p,0x1000)
        m.mem_write(FIRST,struct.pack('<I',VTABLE));m.mem_write(SECOND,struct.pack('<I',VTABLE));m.mem_write(VTABLE+0xc8,struct.pack('<I',METHOD));m.mem_write(METHOD,b'\xc2\x0c\x00');self.current=None;self.trace=[];self.target=0;self.stopped=0
        def hook(machine,address,size,user):
            if self.current is None:return
            if address==self.target:self.stopped=address;machine.emu_stop();return
            if address==METHOD:
                stack=machine.reg_read(UC_X86_REG_ESP);device,state,value=struct.unpack('<3I',machine.mem_read(stack+4,12));i=len(self.trace);self.trace.append((1 if device==FIRST else 2,state,value));machine.reg_write(UC_X86_REG_EAX,self.current[5+i]);
                if i==0:
                    if self.current[7]==1:machine.mem_write(0x3d128b0,struct.pack('<I',SECOND))
                    elif self.current[7]==2:self.target=0x77ec7a if self.current[1]==0 else 0x77eca4
        m.hook_add(unicorn.UC_HOOK_CODE,hook)
    def run(self,w):
        m=self.machine;self.current=w;self.trace=[];self.stopped=0;self.target=0;m.mem_write(0x3ab9890,bytes([w[2]&255]));m.mem_write(0x3d128b0,struct.pack('<I',FIRST if w[4]==1 else 0))
        if w[0]<2 and not w[3]:self.target=0x4030dd if w[0]==0 else 0x403103
        elif w[1]>=16:self.target=0x77ecb7 # native is supplied only the 88-byte .rdata backing
        elif not w[4]:self.target=(0x77ec68 if w[1]==0 else 0x77ec92)if w[1]<2 else 0x77ecbe
        elif w[4]==2:self.target=(0x77ec6d if w[1]==0 else 0x77ec97)if w[1]<2 else 0x77ecc3
        elif w[0]>=2 and w[1]>=2:self.target=0x77ecca
        m.mem_write(STACK,struct.pack('<3I',STOP,w[0],w[1]));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x4030d0,STOP,count=1000)
        return dict(packed=m.mem_read(0x3ab9890,1)[0],trace=list(self.trace),boundary=self.stopped)
def fixtures():
    cases=[dict(name='all packed bytes/source selectors/valid modes',words=[selector,mode,old,1,1,0x80004005,0,0])for old in range(256)for selector in(0,1)for mode in range(16)]
    for selector,mode in((99,0),(99,1),(2,8),(2,13),(0,16),(1,0xffffffff)):
        cases.append(dict(name='source selector bypass/unsupported table domain',words=[selector,mode,0xa5,1,1,0,0,0]))
    for mode in(0,1,8):
        for device in(0,2):cases.append(dict(name='unknown/null device boundary',words=[0,mode,0xa5,1,device,0,0,0]))
    cases.append(dict(name='unknown packed byte',words=[0,8,0xa5,0,1,0,0,0]))
    for mode in(0,1):
        for mutation in(1,2):cases.append(dict(name='source second call reads live device',words=[0,mode,0xa5,1,1,0,0,mutation]))
    return cases
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Wrong source/oracle')
    table=image.read(0x88ad64,88);cases=fixtures();blob=b''.join(struct.pack('<8I',*c['words'])for c in cases);a.output.with_suffix('.cases.bin').write_bytes(blob);lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_ninja_render;fn.argtypes=[ctypes.POINTER(ctypes.c_ubyte),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t];fn.restype=ctypes.c_int;source=RenderOriginal(image);mismatches=[]
    for index,c in enumerate(cases):
        w=c['words'];expected=source.run(w);out=(ctypes.c_uint32*20)();status=fn((ctypes.c_ubyte*88).from_buffer_copy(table),88,(ctypes.c_uint32*8)(*w),8,out,20)
        if status:raise RuntimeError('Native rejected fixture')
        actual_trace=[tuple(out[6+i*4:9+i*4])for i in range(out[4])]
        if expected['boundary']!=out[1]or(w[3]and expected['packed']!=out[3])or expected['trace']!=actual_trace:mismatches.append(dict(case=index,name=c['name'],words=w,source=expected,native=list(out),native_trace=actual_trace))
    root=pathlib.Path(__file__).resolve().parents[2];report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),tables_sha256=sha(table),cases=len(cases),case_blob_sha256=sha(blob),native_sources={n:sha((root/n).read_bytes())for n in('src/engine/ninja_render.hpp','src/engine/ninja_render.cpp','tests/probes/native_ninja_render_probe.cpp','tools/oracles/verify_original_ninja_render.py')},source_ranges={hex(x):dict(bytes=y-x,sha256=sha(image.read(x,y-x)))for x,y in((0x4030d0,0x403125),(0x77ec60,0x77ecda))},mismatches=mismatches,packed_commands_boundaries_match=not mismatches,limits=['Actual game state wrappers only; COMSetRenderState uses a declared test backend, not an originalGPU observation','Source failed HRESULT ignored as actual instructions; native APIs can omit originalHRESULT while preserving availability','Unsupported constant-table domains/unknown facts stop at source boundaries; no source validation invented','No FP arithmetic in this owner; GPU, texture lifetime and full sprite/game execution remain unverified']);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(mismatches))),flush=True);return int(bool(mismatches))
if __name__=='__main__':raise SystemExit(main())
