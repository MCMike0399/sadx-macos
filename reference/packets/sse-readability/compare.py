"""Compare every byte and case identity; a stable aggregate verdict is insufficient."""
import gzip,hashlib,json,pathlib
base=pathlib.Path('build/sse-readability/baseline');current=pathlib.Path('build/sse-readability/current');results={}
for name,before,after in [('sse','sse/full.bin.gz','sse/full.bin.gz'),('axes','axes/full-cases.jsonl.gz','axes/full-cases.jsonl.gz'),('pad','pad-final/full.bin.gz','pad/full.bin.gz'),('native-sse','sse-contract.bin.gz','sse-contract.bin.gz'),('native-axes','axes-contract.bin.gz','axes-contract.bin.gz')]:
    digest=hashlib.sha256();size=0
    with gzip.open(base/before,'rb') as old,gzip.open(current/after,'rb') as new:
        while True:
            a,b=old.read(1024*1024),new.read(1024*1024)
            assert a==b,(name,size)
            if not a:break
            digest.update(a);size+=len(a)
    results[name]=dict(bytes=size,sha256=digest.hexdigest(),every_byte_equal=True)
for name,oldpath,newpath in [('sse','sse/identities.json','sse/identities.json'),('axes','axes/identity.json','axes/identity.json'),('pad','pad-final/identities.json','pad/identities.json')]:
    old=json.loads((base/oldpath).read_text());new=json.loads((current/newpath).read_text())
    if name=='axes':
        for p in (old,new):
            for key in ['library_sha256','original_report_sha256']:p.pop(key)
    assert old==new,name
    results[name]['every_case_identity_equal']=True
pathlib.Path('reference/packets/sse-readability/development-comparison.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
