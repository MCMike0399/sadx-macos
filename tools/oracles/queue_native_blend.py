"""Actual4030d0 body;77ec60 forwarding is the explicit device-service fixture."""
import ctypes,json,pathlib,random,struct,sys
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from pe_image import PEImage
from verify_original_math import Original,STACK,STOP
from extract import SOURCE_SHA256
image=PEImage(sys.argv[1]);assert image.sha256==SOURCE_SHA256
m=Original(image,127).machine;m.mem_map(0x3ab9000,4096)
get=lambda a:struct.unpack('<I',m.mem_read(a,4))[0]
def hook(m,address,size,user):
 sp=m.reg_read(UC_X86_REG_ESP);observed.extend([get(sp+4),get(sp+8),m.mem_read(0x3ab9890,1)[0]])
 m.reg_write(UC_X86_REG_EIP,get(sp));m.reg_write(UC_X86_REG_ESP,sp+4)
m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=0x77ec60,end=0x77ec60)
lib=ctypes.CDLL(str(pathlib.Path(sys.argv[2]).resolve()));fn=lib.sadx_native_queue_blend;U=ctypes.c_uint32;fn.argtypes=[U,U,U,U,ctypes.POINTER(U)]
rng=random.Random(20271028);cases=[(p,s,v) for p in [0,1,15,16,127,240,255] for s in [0,1,2,0xffffffff] for v in [0,1,8,15,16,255,256,0xffffffff]]+[(rng.randrange(256),rng.randrange(4),rng.getrandbits(32)) for _ in range(1000)];bad=[]
for n,(packed,selector,value) in enumerate(cases):
 m.mem_write(0x3ab9890,bytes([packed]));m.mem_write(STACK,struct.pack('<III',STOP,selector,value));m.reg_write(UC_X86_REG_ESP,STACK);observed=[];m.emu_start(0x4030d0,STOP,count=100)
 expected=[m.mem_read(0x3ab9890,1)[0],*observed];out=(U*4)();assert fn(packed,selector,value,0,out)==1
 if list(out)!=expected:bad.append({'case':n,'input':(packed,selector,value),'expected':expected,'actual':list(out)})
 out=(U*4)();assert fn(packed,selector,value,1,out)==0
 if list(out)!=expected:bad.append({'case':n,'unavailable':True})
report={'cases':len(cases)*2,'state_mismatch_count':len(bad),'mismatches':bad[:10],'source_sha256':image.sha256,'scope':__doc__};pathlib.Path(sys.argv[3]).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':report['cases'],'state_mismatch_count':len(bad)}));raise SystemExit(bool(bad))
