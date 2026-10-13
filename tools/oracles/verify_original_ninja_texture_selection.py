"""Actual source texture selection/cache with explicit external renderer receipts.
No D3D/GPU upload or physical cache snapshot is inferred from these fixtures.
Palette dirty paths stop before their unported surface lock, retaining source
revision stores. Native Metal resources are checked by the separate consumer.
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_ECX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_ninja_texture import Original,BASE,STACK,STOP
LIST=BASE+0x1000;ARRAY=BASE+0x2000;DEVICE=BASE+0x5000;VTABLE=BASE+0x6000;SURFACE=BASE+0x7000;MEM=[BASE+0x8000+i*0x100 for i in range(3)];STUB=STOP+0x100;PALETTE=0x3d11f80;COLORS=BASE+0xa000
sha=lambda b:hashlib.sha256(b).hexdigest()
class Source(Original):
    def __init__(self,image):
        super().__init__(image);self.clean=self.m.context_save();self.m.hook_add(unicorn.UC_HOOK_CODE,self.code);self.w=None;self.boundary=0;self.calls=[]
    def stop(self,address):self.boundary=address;self.m.emu_stop()
    def code(self,m,address,size,user):
        w=self.w
        if w is None:return
        if address==0x77f3d0 and w[2]==2:return self.stop(address)
        if address==0x77f440 and w[2]==2:return self.stop(address)
        if address in (0x77f3da,0x77f449) and w[2]==4:return self.stop(address)
        if address in (0x77f3dc,0x77f44e):
            offset=0 if w[0]==1 else (w[1]*12)&0xffffffff
            if w[2]==3 or offset%12 or offset//12>=w[41] or w[2]==5 and offset==0:return self.stop(address)
        if address==0x78cf24 and m.reg_read(UC_X86_REG_ESI)==0:return self.stop(address)
        reads={0x78cf24:(0,UC_X86_REG_ESI),0x78cdc9:(2,UC_X86_REG_EBP),0x78cdcc:(1,UC_X86_REG_EBP),0x78cdd0:(0,UC_X86_REG_EBP),0x78cde8:(3,UC_X86_REG_EBP),0x78ce3b:(4,UC_X86_REG_EBP),0x78cf3b:(3,UC_X86_REG_ESI),0x78cf60:(2,UC_X86_REG_ESI)}
        if address in reads:
            bit,reg=reads[address];pointer=m.reg_read(reg)
            if pointer in MEM and not(w[16+MEM.index(pointer)*6+5]&(1<<bit)):return self.stop(address)
        if address in (0x78cf33,0x78ce08) and not w[12]:return self.stop(address)
        if address==0x78cdf8:
            if not w[39]:return self.stop(address)
            index=(m.reg_read(UC_X86_REG_EAX)-PALETTE)//16
            if index==0 and not w[42]&1:return self.stop(address)
        if address==0x78ce37 and not w[42]&2:return self.stop(address)
        if address==0x78ce4e and w[37]==0:return self.stop(address)
        if address==0x78ce51 and m.reg_read(unicorn.x86_const.UC_X86_REG_EDI)==0:return self.stop(address)
        if address==0x78ce5a:return self.stop(address)
        if address==0x78cf69 and not w[40]:return self.stop(address)
        loads={0x78cf42,0x78ce10,0x78b840,0x78b852,0x78b867}
        dereferences={0x78cf48,0x78ce15,0x78b845,0x78b857,0x78b86c}
        if address in loads and w[8]==2:return self.stop(address)
        if address in dereferences and w[8]==0:return self.stop(address)
        virtual={0x78cf4e:0,0x78ce1b:0,0x78b84c:1,0x78b861:1,0x78b876:1}
        if address in virtual:
            if w[9]==1:return self.stop(address)
            esp=m.reg_read(UC_X86_REG_ESP);device,a,b=struct.unpack('<3I',m.mem_read(esp,12));self.calls.append((virtual[address],a,b))
            if w[9]==2:return self.stop(address)
    def run(self,w):
        self.w=w;self.boundary=0;self.calls=[];m=self.m;m.context_restore(self.clean)
        for i,pointer in enumerate(MEM):
            a=w[16+i*6:22+i*6];raw=bytearray(68);struct.pack_into('<II',raw,0,a[0],a[1]);struct.pack_into('<I',raw,0x20,a[2]);struct.pack_into('<I',raw,0x34,a[3]);struct.pack_into('<H',raw,0x42,a[4]&65535);m.mem_write(pointer,bytes(raw))
        rows=bytearray(36)
        for i in range(3):struct.pack_into('<I',rows,i*12+8,MEM[i])
        m.mem_write(ARRAY,bytes(rows));m.mem_write(LIST,struct.pack('<II',0 if w[2]==3 else ARRAY,w[4]));m.mem_write(DEVICE,struct.pack('<I',VTABLE));m.mem_write(VTABLE+0xf4,struct.pack('<I',STUB));m.mem_write(VTABLE+0xc8,struct.pack('<I',STUB));m.mem_write(STUB,b'\xb8'+struct.pack('<I',w[11])+b'\xc2\x0c\x00');m.mem_write(SURFACE,struct.pack('<I',VTABLE))
        current=0 if w[0]==1 or w[2]==0 else LIST;requested=0 if w[46]==0 else MEM[(w[46]-1)%3];active=0 if w[47]==0 else MEM[(w[47]-1)%3]
        for p,v in [(0x3d0fa24,current),(0x3ce7128,requested),(0x3d129a0,active),(0x3d0fdf4,w[5]),(0x3d128b0,DEVICE if w[8] else 0)]:m.mem_write(p,struct.pack('<I',v))
        palette=bytearray(128*16)
        for i in range(128):struct.pack_into('<I',palette,i*16,(w[34]+i)&0xffffffff)
        struct.pack_into('<I',palette,0,w[35]);struct.pack_into('<I',palette,8,COLORS if w[37]==1 else 0);struct.pack_into('<H',palette,12,w[36]&65535);m.mem_write(PALETTE,bytes(palette))
        address=[0x78cf20,0x77f3d0,0x77f440,0x78b840][w[0]];m.reg_write(UC_X86_REG_ECX,MEM[w[3]%3] if w[0]==0 else 0 if w[2]==0 else LIST if w[0]==1 else w[1]);m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(address,STOP,count=12000)
        if not self.boundary and m.reg_read(UC_X86_REG_ESP)!=STACK+4:raise RuntimeError('Source selection did not complete')
        def ref(p):v=struct.unpack('<I',m.mem_read(p,4))[0];return 0 if not v else MEM.index(v)+1
        list_value=struct.unpack('<I',m.mem_read(0x3d0fa24,4))[0];cache=struct.unpack('<I',m.mem_read(0x3d0fdf4,4))[0]
        return dict(boundary=self.boundary,completed=not self.boundary,outcome=m.reg_read(UC_X86_REG_EAX),current=bool(list_value),cache=cache,requested=ref(0x3ce7128),active=ref(0x3d129a0),versions=[struct.unpack('<H',m.mem_read(p+0x42,2))[0] for p in MEM],calls=self.calls)
def fixtures():
    def base():
        w=[0]*48;w[2]=1;w[4]=3;w[5]=99;w[8]=1;w[11]=0xffffffff;w[12]=1;w[34]=1000;w[35]=2;w[36]=5;w[37]=1;w[39]=1;w[40]=1;w[41]=3;w[42]=3
        for i in range(3):w[16+i*6:22+i*6]=[7+i,0xffffffff,0,SURFACE,0,31]
        return w
    for operation in range(4):
        for kind in [0,1,2,3,4,5]:
            for device in [0,1,2]:
                for renderer in [0,1,2]:
                    w=base();w[0]=operation;w[2]=kind;w[8]=device;w[9]=renderer;yield w
    for operation in range(3):
        for fmt in [0,0x08000000,0x10000000,0x18000000,0x20000000,0x28000000,0x38000000,0xffffffff]:
            for cache in [7,8,99]:
                for surface in [0,SURFACE]:
                    for palette in [0xffffffff,2,5000]:
                        w=base();w[0]=operation;w[5]=cache;w[18]=fmt;w[19]=surface;w[17]=palette;yield w
    for index in [0,1,2,3,0x40000000,0x80000000,0xc0000000,0xffffffff,0x15555556]:w=base();w[0]=2;w[1]=index;yield w
    for operation in range(3):
        for bit in range(5):w=base();w[0]=operation;w[21]&=~(1<<bit);yield w
    for which in [12,39,40,42,37]:w=base();w[18]=0x28000000;w[17]=2;w[which]=0;yield w
    for version in [0,5,6]:w=base();w[18]=0x28000000;w[17]=2;w[20]=version;yield w
    for count in [0,1,0xffffffff]:w=base();w[0]=1;w[4]=count;yield w
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle identity mismatch')
    lib=ctypes.CDLL(str(a.library.resolve()));fn=lib.sadx_native_ninja_texture_selection;fn.argtypes=[ctypes.POINTER(ctypes.c_uint32)]*2;fn.restype=ctypes.c_int;lib.sadx_native_ninja_texture_selection_compiler.restype=ctypes.c_char_p
    source=Source(image);cases=list(fixtures());blob=b''.join(struct.pack('<48I',*w)for w in cases);a.output.with_suffix('.cases.bin').write_bytes(blob);mismatches=[]
    for index,w in enumerate(cases):
        expected=source.run(w);out=(ctypes.c_uint32*32)();status=fn((ctypes.c_uint32*48)(*w),out)
        if status:raise RuntimeError('Native selection probe failure')
        different=out[1]!=expected['boundary'] or bool(out[2])!=expected['completed']
        if expected['completed'] and w[0]!=3:different|=out[3]!=expected['outcome']
        if out[4]!=0xfffffffe:different|=bool(out[4])!=expected['current']
        if w[12]:different|=out[5]!=expected['cache']
        different|=out[6]!=expected['requested'] or out[7]!=expected['active']
        for i in range(3):
            if w[16+i*6+5]&16 or expected['versions'][i]!=w[20+i*6]:different|=out[8+i]!=expected['versions'][i]
        calls=[tuple(out[12+i*3:15+i*3])for i in range(out[11])];different|=calls!=expected['calls']
        if different and len(mismatches)<30:mismatches.append(dict(case=index,input=w,source=expected,native=list(out[:12]),native_calls=calls))
    root=pathlib.Path(__file__).resolve().parent.parent;report=dict(source_sha256=image.sha256,library_sha256=sha(a.library.read_bytes()),compiler=lib.sadx_native_ninja_texture_selection_compiler().decode(),cases=len(cases),case_sha256=sha(blob),mismatches=mismatches,passed=not mismatches,source_files_sha256={v:sha((root/v).read_bytes())for v in ['src/ninja/ninja_texture_selection.hpp','src/ninja/ninja_texture_selection.cpp','src/ninja/ninja_sprite_queue.hpp','tests/probes/native_ninja_texture_selection_probe.cpp','tools/oracles/verify_original_ninja_texture_selection.py']},limits=['Actual77f3d0/77f440/78cf20/78cdc0/78b840 instructions, source bytes unmodified','Explicit synthetic renderer HRESULT receipts or stops, not physical D3D/NVIDIA binding','Dirty palette paths stop at surface lock while comparing preceding version stores','Missing facts and unowned array fields stop before source read; no recovered Win32 pointer casts','Metal typed resource validation is a separate actual consumer owner'])
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),mismatches=len(mismatches),passed=not mismatches,first=mismatches[:3])),flush=True);return 0 if not mismatches else 1
if __name__=='__main__':raise SystemExit(main())
