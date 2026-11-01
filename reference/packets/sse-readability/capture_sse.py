"""Observe the unchanged scalar judge; freeze every input, output and classification."""
import argparse, gzip, hashlib, json, pathlib, struct, sys
sys.path.insert(0, 'tools/oracles')
import verify_original_sse_math as judge
p=argparse.ArgumentParser();p.add_argument('library');p.add_argument('directory');a=p.parse_args()
out=pathlib.Path(a.directory);out.mkdir(parents=True,exist_ok=False)
stream=bytearray(); identities={'value': [], 'status': [], 'classes': {}}
def capture(operation,x,y,mxcsr,expected,status,result,flags,reason):
    case=len(stream)//52
    stream.extend(struct.pack('<IQQIQIQII',operation,x,y,mxcsr,expected,status,result,flags,0))
    if expected!=result:
        identities['value'].append(case)
        identities['classes'].setdefault(reason,[]).append(case)
    if status!=flags:identities['status'].append(case)
source=pathlib.Path(judge.__file__).read_text()
anchor="                        if status!=flags.value:report['mxcsr_mismatches']+=1"
assert source.count(anchor)==1
source=source.replace(anchor,anchor+"\n                        capture(operation,x,y,mxcsr,expected,status,out.value,flags.value,reason if expected!=out.value else '')")
sys.argv=[judge.__file__,a.library,'--output',str(out/'report.json')]
exec(compile(source,judge.__file__,'exec'),{'__name__':'__main__','capture':capture})
with gzip.open(out/'full.bin.gz','wb') as f:f.write(stream)
identities.update(records=len(stream)//52,bytes=len(stream),sha256=hashlib.sha256(stream).hexdigest())
(out/'identities.json').write_text(json.dumps(identities,separators=(',',':'))+'\n')
print({k:v for k,v in identities.items() if k not in ('value','status','classes')})
