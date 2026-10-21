"""Independent holdout for new native owners; unchanged original execution adapter."""
import argparse,ctypes,hashlib,json,pathlib,random,struct
from pe_image import PEImage
from verify_original_ninja_queue_lifecycle import Source,HEAP,f32
from extract import SOURCE_SHA256

def main():
 p=argparse.ArgumentParser();p.add_argument('exe',type=pathlib.Path);p.add_argument('probe',type=pathlib.Path);p.add_argument('--output',required=True,type=pathlib.Path);a=p.parse_args();image=PEImage(a.exe);assert image.sha256==SOURCE_SHA256
 rng=random.Random(20271027);cases=[]
 for n in range(700):
  c=[0]*24;c[0]=HEAP+rng.randrange(0,64);c[1]=rng.choice([127,639,895,1151,2175,3199]);c[2:6]=[rng.choice([0,2147483648,2139095040,4286578688,2143289344,2141192192,f32(rng.uniform(-100,100))]) for _ in range(4)];c[6]=rng.getrandbits(32);c[7:23]=[rng.getrandbits(32) for _ in range(16)];c[23]=rng.choice([0,0,0,1,2,3,4]);cases.append(c)
 lib=ctypes.CDLL(str(a.probe.resolve()));fn=lib.sadx_native_ninja_queue_lifecycle;U=ctypes.c_uint32;B=ctypes.c_uint8;fn.argtypes=[ctypes.POINTER(B),ctypes.c_size_t,ctypes.POINTER(U),ctypes.c_size_t,ctypes.POINTER(U),ctypes.c_size_t]
 tables=image.read(0x88ad9c,32)+image.read(0x7dcb10,4)+image.read(0x7dcb0c,4);source=Source(image);bad=[];flags=0
 for n,c in enumerate(cases):
  expected=source.run(c);out=(U*80)();assert fn((B*40).from_buffer_copy(tables),40,(U*24)(*c),24,out,80)==1
  diff=[i for i in range(80) if i!=52 and out[i]!=expected[i]]
  if diff:bad.append({'case':n,'fields':diff,'expected':expected,'actual':list(out)})
  flags+=out[52]!=expected[52]
 blob=b''.join(struct.pack('<24I',*c) for c in cases)
 report={'cases':len(cases),'state_mismatch_count':len(bad),'fp_flag_mismatches':flags,'stores_trace_boundaries_match':not bad,'case_sha256':hashlib.sha256(blob).hexdigest(),'seed':20271027,'mismatches':bad[:10],'source_sha256':image.sha256,'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest()};a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','state_mismatch_count','fp_flag_mismatches']}));return int(bool(bad))
if __name__=='__main__':raise SystemExit(main())
