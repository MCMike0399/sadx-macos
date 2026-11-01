"""Full observations from unchanged DreamPad instruction judge."""
import gzip, hashlib, json, pathlib, struct, sys
sys.path.insert(0,'tools/oracles')
import verify_original_input_mod_pad as judge
library,directory=sys.argv[1:];out=pathlib.Path(directory);out.mkdir(parents=True,exist_ok=False)
stream=bytearray();identity={'value':[],'status':[]}
def capture(n,c,ret,got,e):
    stream.extend(struct.pack('<I48qi24I24I',n,*c,ret,*got,*e))
    if any(got[i]!=e[i] for i in range(24) if i!=19):identity['value'].append(n)
    if got[19]!=e[19]:identity['status'].append(n)
source=pathlib.Path(judge.__file__).read_text()
anchor="        if got[19]!=e[19]:mxcsr_bad+=1"
assert source.count(anchor)==1
# The inherited probe invocation is asserted successful by the unchanged judge.
source=source.replace(anchor,anchor+'\n        capture(n,c,1,got,e)')
sys.argv=[judge.__file__, '/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll',library,'--output',str(out/'report.json')]
scope={'__name__':'capture','__file__':judge.__file__,'capture':capture}
exec(compile(source,judge.__file__,'exec'),scope)
assert scope['main']()==0
with gzip.open(out/'full.bin.gz','wb') as f:f.write(stream)
identity.update(bytes=len(stream),sha256=hashlib.sha256(stream).hexdigest())
(out/'identities.json').write_text(json.dumps(identity,indent=2)+'\n')
