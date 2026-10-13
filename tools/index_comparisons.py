#!/usr/bin/env python3
"""Write index.json summarising every comparison report in a directory (scalar fields + digest)."""
import argparse,hashlib,json,pathlib
p=argparse.ArgumentParser();p.add_argument('directory',type=pathlib.Path);p.add_argument('--gate',required=True);p.add_argument('--note',default='');a=p.parse_args()
index={'gate':a.gate,'note':a.note,'reports':{}}
for f in sorted(a.directory.glob('*.json')):
    if f.name=='index.json':continue
    d=json.loads(f.read_text())
    index['reports'][f.name]={'sha256':hashlib.sha256(f.read_bytes()).hexdigest(),'summary':{k:v for k,v in d.items() if isinstance(v,(int,float,bool,str)) and k!='limits'}}
(a.directory/'index.json').write_text(json.dumps(index,indent=1)+'\n');print(f"{len(index['reports'])} reports indexed")
