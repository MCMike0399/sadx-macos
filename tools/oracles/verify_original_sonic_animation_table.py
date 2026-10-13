"""Actual7cec90 integer descriptor producer versus generated native recipes."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_FPCW,UC_X86_REG_FPSW
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_math import Original,DATA,STACK,STOP
TABLE=0x3c56210;EXPORT=DATA+0x1000
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('native_library',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args();image=PEImage(a.exe)
 if image.sha256!=SOURCE_SHA256 or unicorn.__version__!='2.1.4':raise RuntimeError('Source/oracle mismatch')
 original=Original(image,0x7f);m=original.machine;m.mem_map(0x38f6000,0x1000);m.mem_map(0x3c56000,0x2000);m.mem_write(0x38f6e7c,struct.pack('<I',EXPORT))
 lib=ctypes.CDLL(str(a.native_library.resolve()));probe=lib.sadx_native_sonic_animation_table;probe.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t];probe.restype=ctypes.c_int
 rng=random.Random(0x7cec90);cases=[[0x21000000+32*i for i in range(149)],[0]*149,[0xffffffff]*149]+[[rng.getrandbits(32)for _ in range(149)]for __ in range(100)]
 report=dict(source_sha256=image.sha256,native_library_sha256=sha(a.native_library.read_bytes()),source_body=dict(address='0x7cec90',bytes=7674,sha256=sha(image.read(0x7cec90,7674))),records=147,export_slots=149,cases=0,byte_mismatches=0,fp_mismatches=0,examples=[],limits=['Export identities are experimental native/source fixture tokens; no DLL or game startup is executed','Actual export loader401ac0 resolves___SONIC_ACTIONS; its runtime success is a separate prerequisite','147 output records follow actual source stores; SDK count146 is not the authority'])
 digest=hashlib.sha256()
 for case,identities in enumerate(cases):
  raw=struct.pack('<149I',*identities);digest.update(raw);m.mem_write(EXPORT,raw);m.mem_write(TABLE,bytes([0xa5])*2352);m.mem_write(STACK,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_FPCW,0x7f);m.reg_write(UC_X86_REG_FPSW,0x20);m.emu_start(0x7cec90,STOP,count=5000)
  expected=bytes(m.mem_read(TABLE,2352));output=(ctypes.c_uint8*2352)()
  if probe((ctypes.c_uint32*149)(*identities),149,output,2352):raise RuntimeError('Native producer rejected fixture')
  diffs=[i for i,(x,y)in enumerate(zip(expected,output))if x!=y];report['byte_mismatches']+=len(diffs);report['fp_mismatches']+=int(m.reg_read(UC_X86_REG_FPCW)!=0x7f or m.reg_read(UC_X86_REG_FPSW)&63!=0x20)
  if diffs and len(report['examples'])<10:report['examples'].append(dict(case=case,offsets=diffs[:20]))
  report['cases']+=1
 root=pathlib.Path(__file__).resolve().parents[2];report['native_sources']={n:sha((root/n).read_bytes())for n in ('src/player/sonic_animation_table.hpp','src/player/sonic_animation_table.cpp','src/player/sonic_animation_table_data.hpp','tools/emit/emit_sonic_animation_table.py','tests/probes/native_sonic_animation_table_probe.cpp','tools/oracles/verify_original_sonic_animation_table.py')};report.update(case_sha256=digest.hexdigest(),passed=not(report['byte_mismatches']or report['fp_mismatches']));a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('cases','records','export_slots','byte_mismatches','fp_mismatches','passed')}));return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
