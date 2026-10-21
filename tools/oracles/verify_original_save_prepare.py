"""Full5065d0 actual initializer composition, including pinned Input DLL rumble hook.

Both source callees execute; all1392save bytes plus1976story/sequence/completion
bytes and rings are compared. Wider cutscene state is independently covered by
the unchanged bootstrap oracle. No source profile behavior is replaced by receipt.
"""
import argparse,ctypes,hashlib,json,struct
from pathlib import Path
from pe_image import PEImage
from extract import SOURCE_SHA256
from verify_original_save_data import Source,SAVE,RINGS
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('probe',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--input-dll',type=Path,default=Path('reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll'));a=p.parse_args();image=PEImage(a.exe);mod=PEImage(a.input_dll);assert image.sha256==SOURCE_SHA256 and mod.sha256=='c5ccdcc7929da1f3d0c03b24da33c350d388bda41d3d1651797758205180b7f5';source=Source(image,mod);B=ctypes.c_uint8;U=ctypes.c_uint32;fn=ctypes.CDLL(str(a.probe.resolve())).sadx_native_save_prepare;fn.argtypes=[U,ctypes.c_int32,ctypes.POINTER(B),ctypes.POINTER(B),ctypes.POINTER(U)];bad=[];cases=0
for pattern in range(256):
 for rings in [-32768,0,32767]:
  cases+=1;m=source.machine;m.mem_write(SAVE,bytes([pattern])*1392);m.mem_write(RINGS,struct.pack('<h',rings));regions=[(0x3b18808,512),(0x3b18608,512),(0x3b18408,512),(0x3b183a8,96),(0x3b18250,344)]
  for address,size in regions:m.mem_write(address,bytes([pattern])*size)
  source.run(0x5065d0);expected=bytes(m.mem_read(SAVE,1392));story=b''.join(bytes(m.mem_read(address,size)) for address,size in regions);saved_rings=int.from_bytes(m.mem_read(RINGS,2),'little');out=(B*1392)();state=(B*1976)();result=(U*1)();assert fn(pattern,rings,out,state,result)==1
  if bytes(out)!=expected or bytes(state)!=story or result[0]!=saved_rings:bad.append({'pattern':pattern,'rings':rings,'save_matches':bytes(out)==expected,'story_matches':bytes(state)==story,'rings_match':result[0]==saved_rings})
report={'cases':cases,'mismatch_count':len(bad),'values_match':not bad,'passed':not bad,'mismatches':bad[:8],'limits':__doc__,'source_sha256':image.sha256,'input_mod_sha256':mod.sha256,'source_ranges':{'0x5065d0':'0x5065da','0x413300':'0x41337a','0x42d3b0':'0x42d62e','0x10009240':'0x1000924e'}};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':cases,'mismatches':len(bad)}));raise SystemExit(int(bool(bad)))
