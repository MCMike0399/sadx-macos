"""Original5065e0 request guard/store and delegated CreateSaveData call receipt."""
import argparse,ctypes,json,random,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from verify_original_save_creation import Source
from verify_original_math import STACK,STOP
from pe_image import PEImage
from extract import SOURCE_SHA256
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256;source=Source(image);m=source.m;captures=[]
def hook(m,a,size,user):
 sp=m.reg_read(UC_X86_REG_ESP);captures.append(1);m.reg_write(UC_X86_REG_EIP,int.from_bytes(m.mem_read(sp,4),'little'));m.reg_write(UC_X86_REG_ESP,sp+4)
m.hook_add(unicorn.UC_HOOK_CODE,hook,begin=0x42d630,end=0x42d630);U=ctypes.c_uint32;fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_save_request;fn.argtypes=[ctypes.POINTER(U),ctypes.POINTER(U)];bad=[];cases=0
for disabled in [0,1,2,0xffffffff,0x80000000]:
 for ready in range(256):
  cases+=1;captures.clear();m.context_restore(source.clean);m.mem_write(0x3b29198,struct.pack('<I',disabled));m.mem_write(0x3b291ad,bytes([ready]));m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.emu_start(0x5065e0,STOP,timeout=1000000);assert m.reg_read(UC_X86_REG_EIP)==STOP;expected=[m.reg_read(UC_X86_REG_EAX),m.mem_read(0x3b291ad,1)[0],len(captures)];out=(U*3)();assert fn((U*2)(disabled,ready),out)==1
  if list(out)!=expected:bad.append({'input':[disabled,ready],'expected':expected,'observed':list(out)})
report={'cases':cases,'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'limits':__doc__,'source_ranges':{'0x5065e0':'0x5065fb'},'source_sha256':image.sha256};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':cases,'mismatches':len(bad)}));raise SystemExit(int(bool(bad)))
