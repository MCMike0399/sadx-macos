"""Actual401070 copied eight-word packet and centered x/y; finite masked-CW scope.
Only typed pointer identity receipts are normalized, no pointed-to bytes are read.
"""
import ctypes,json,random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from verify_original_late_sprite import Source,root,PEImage,STACK,STOP,DATA
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW

def run(count=1200,seed=401070,output='center-dev.json'):
 image=PEImage(oracle_exe);source=Source(image);m=source.m
 U=ctypes.c_uint32;fn=ctypes.CDLL(str(oracle_library)).sadx_native_center_screen_sprite;fn.argtypes=[ctypes.POINTER(U)]*2
 rng=random.Random(seed);bad=[];flags=0
 bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
 for case in range(count):
  v=[rng.choice([127,639,895,3199]),bits(rng.uniform(-10000,10000)),bits(rng.uniform(-10000,10000)),rng.getrandbits(32),bits(rng.uniform(-10,10)),bits(rng.uniform(-10,10)),rng.getrandbits(32),DATA+0x100,DATA+0x200,bits(rng.uniform(-10,10)),bits(rng.uniform(-10,10))]
  m.context_restore(source.clean);m.mem_write(DATA,struct.pack('<8I',*v[1:9]));m.mem_write(0x8928c0,struct.pack('<2I',*v[9:11]));m.mem_write(STACK,struct.pack('<2I',STOP,DATA));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,v[0]);m.reg_write(UC_X86_REG_FPSW,0);m.emu_start(0x401070,STOP,timeout=0)
  expected=list(struct.unpack('<8I',m.mem_read(0x38f6ce4,32)))+[m.reg_read(UC_X86_REG_FPSW)&63];out=(U*9)();assert fn((U*11)(*v),out)==1
  diff=[(i,x,y) for i,(x,y) in enumerate(zip(expected,out)) if x!=y and i!=8];flags+=expected[8]!=out[8]
  if diff:bad.append({'case':case,'input':v,'differences':diff})
 report={'cases':count,'seed':seed,'mismatch_count':len(bad),'flag_mismatches':flags,'values_match':not bad,'passed':not bad and not flags,'mismatches':bad[:8],'source_sha256':image.sha256,'limits':__doc__};Path(output).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report));return bool(bad)
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('exe',type=Path);parser.add_argument('native_library',type=Path)
 parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--cases',type=int,default=1200);parser.add_argument('--seed',type=int,default=401070)
 args=parser.parse_args();oracle_exe=args.exe;oracle_library=args.native_library.resolve()
 raise SystemExit(run(args.cases,args.seed,args.output))
