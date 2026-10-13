"""Model-DLL bindings 401ac0 (this = 3ab9170, from the 7c4e90 static
initializer) against original instructions. LoadLibraryA and GetProcAddress
(IAT 7db040/7db044) are fixtures: a slot's DLL is present or absent per case
(case-insensitive name match), and GetProcAddress resolves names against that
DLL's real export table at the native base layout (0x20000000 upward, 64 KiB
aligned; slot 0 resolves CHRMODELS_orig.dll behind the 0x10000000 proxy
handle). Compared: the seven handles at 3ab9170 and every bound global (an
absent slot leaves its globals untouched, so they are pre-filled distinctly).
"""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX,UC_X86_REG_ECX
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,STACK,STOP,CODE
sha=lambda b:hashlib.sha256(b).hexdigest()
FILES=["CHRMODELS_orig.dll","ADV00MODELS.DLL","ADV01MODELS.DLL","ADV01CMODELS.DLL","ADV02MODELS.DLL","ADV03MODELS.DLL","BOSSCHAOS0MODELS.DLL"]
NAMES=["system\\chrmodels.dll","system\\adv00models.dll","system\\adv01models.dll","system\\adv01cmodels.dll","system\\adv02models.dll","system\\adv03models.dll","system\\bosschaos0models.dll"]
def pe_exports(path):
    b=path.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0];opt=pe+24;base=struct.unpack_from('<I',b,opt+28)[0];size=struct.unpack_from('<I',b,opt+56)[0]
    erva,esize=struct.unpack_from('<II',b,opt+96);nsec=struct.unpack_from('<H',b,pe+6)[0];sh=opt+struct.unpack_from('<H',b,pe+20)[0]
    secs=[struct.unpack_from('<IIII',b,sh+40*i+8) for i in range(nsec)]
    def off(rva):
        for vs,va,rs,ro in secs:
            if va<=rva<va+max(vs,rs):return rva-va+ro
    e=off(erva);nfun,nnames,afun,anames,aord=struct.unpack_from('<IIIII',b,e+20);out={}
    for i in range(nnames):
        n=off(struct.unpack_from('<I',b,off(anames)+4*i)[0]);name=b[n:b.index(b'\0',n)].decode()
        out[name]=struct.unpack_from('<I',b,off(afun)+4*struct.unpack_from('<H',b,off(aord)+2*i)[0])[0]
    return size,out
def main():
    p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('system',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
    if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Pinned source/Unicorn mismatch')
    dlls=[(pe_exports(a.system/f) if (a.system/f).exists() else None) for f in FILES]
    m=Original(image,0x37f).machine
    for s in image.sections:
        if s['name'] in('.text','.rdata'):continue
        base=image.base+s['rva'];size=(max(s['virtual_size'],s['raw_size'])+0xfff)&~0xfff
        m.mem_map(base,size);m.mem_write(base,image.slice(s['file_offset'],min(s['raw_size'],size)))
    LOAD,PROC=CODE+0x500,CODE+0x510;m.mem_write(LOAD,b'\xc2\x04\x00');m.mem_write(PROC,b'\xc2\x08\x00')
    m.mem_write(0x7db040,struct.pack('<I',LOAD));m.mem_write(0x7db044,struct.pack('<I',PROC))
    m.mem_write(0x51f680,b'\xc3') # already a RET in the image; kept explicit
    clean=m.context_save();state={}
    def cstr(addr):
        out=b''
        while not out.endswith(b'\0'):out+=bytes(m.mem_read(addr+len(out),1))
        return out[:-1].decode('latin1')
    def stub(mu,address,size,user):
        esp=mu.reg_read(UC_X86_REG_ESP)
        if address==LOAD:
            name=cstr(struct.unpack('<I',mu.mem_read(esp+4,4))[0]).lower();slot=NAMES.index(name)
            mu.reg_write(UC_X86_REG_EAX,state['handles'][slot] if state['present'][slot] else 0)
        else:
            handle,name=struct.unpack('<II',mu.mem_read(esp+4,8));name=cstr(name)
            slot=state['handles'].index(handle);size,ex=dlls[slot]
            mu.reg_write(UC_X86_REG_EAX,state['bases'][slot]+ex[name] if name in ex else 0)
    m.hook_add(unicorn.UC_HOOK_CODE,stub,begin=LOAD,end=LOAD);m.hook_add(unicorn.UC_HOOK_CODE,stub,begin=PROC,end=PROC)
    lib=ctypes.CDLL(str(a.native_library.resolve()));fn=lib.sadx_native_model_dlls;fn.argtypes=[ctypes.c_char_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];fn.restype=ctypes.c_int
    import re
    globals_=[int(g,16) for g in re.findall(r'^\s*\{\d,"[^"]+",(0x[0-9a-f]+)\},',(pathlib.Path(__file__).resolve().parents[2]/'src/game/model_dlls.hpp').read_text(),re.M)]
    if len(globals_)!=85:raise RuntimeError('binding table parse')
    rng=random.Random(0x401ac0);bad=[];cases=0
    masks=[0x7f,0x3f,0x3e,0x01,0x00]+[rng.getrandbits(7) for _ in range(40)]
    for mask in masks:
        present=[bool(mask>>i&1) and dlls[i] is not None for i in range(7)]
        base=0x20000000;bases=[0]*7;handles=[0]*7
        for i in range(7):
            if present[i]:bases[i]=base;base=(base+dlls[i][0]+0xffff)&~0xffff
        handles=[0x10000000 if i==0 else bases[i] for i in range(7)]
        state.update(present=present,bases=bases,handles=handles)
        m.context_restore(clean);fill=[0xa5a50000+i for i in range(len(globals_))]
        for g,v in zip(globals_,fill):m.mem_write(g,struct.pack('<I',v))
        m.mem_write(0x3ab9170,b'\xcc'*28)
        m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_ECX,0x3ab9170);m.emu_start(0x401ac0,STOP,count=200000)
        if m.reg_read(UC_X86_REG_EIP)!=STOP or m.reg_read(UC_X86_REG_EAX)!=0x3ab9170:raise RuntimeError('Source return ABI')
        source=list(struct.unpack('<7I',m.mem_read(0x3ab9170,28)))+[struct.unpack('<I',m.mem_read(g,4))[0] for g in globals_]
        out=(ctypes.c_uint32*(7+len(globals_)))()
        if fn(str(a.system).encode(),mask,out)!=1:raise RuntimeError('Native rejected')
        native=list(out)
        # an absent slot: the source leaves the pre-fill, the native port leaves the value unknown
        for i,g in enumerate(globals_):
            if native[7+i]==0xdeadbeef and source[7+i]==fill[i]:native[7+i]=fill[i]
        cases+=1
        if native!=source:
            d=[i for i in range(len(source)) if native[i]!=source[i]]
            bad.append(dict(mask=hex(mask),fields=d[:10],source=[hex(source[i]) for i in d[:10]],native=[hex(native[i]) for i in d[:10]]))
    names=['src/game/model_dlls.hpp','src/game/model_dlls.cpp','src/game/executable_image.cpp','tests/probes/native_model_dlls_probe.cpp','tools/oracles/verify_original_model_dlls.py']
    report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_files={x:sha((pathlib.Path(__file__).resolve().parents[2]/x).read_bytes()) for x in names},
        original_range_sha256=sha(image.read(0x401ac0,0x40254e-0x401ac0)+image.read(0x7c4e90,0x16)),bound_globals=len(globals_),cases=cases,mismatch_count=len(bad),mismatches=bad[:10],bindings_match=not bad,limits=__doc__)
    a.output.write_text(json.dumps(report,indent=1)+'\n');print(json.dumps(dict(cases=cases,bound_globals=len(globals_),mismatches=len(bad))))
    for b in bad[:3]:print(json.dumps(b))
    return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
