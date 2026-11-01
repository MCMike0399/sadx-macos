"""Run complete default IO/list/frame save closure; no case or tolerance overrides."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('input_dll');p.add_argument('library');p.add_argument('output',type=Path);a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=True)
rows=[]
for slug in ['save_file_list','save_list_free','load_selected_save','save_write','save_frame','save_creation','save_data','save_prepare','save_request','save_completion']:
 command=[sys.executable,'reference/packets/save-io-frame-judges/observe.py',f'tools/oracles/verify_original_{slug}.py',a.exe,str(Path(a.library).resolve()),str(a.output)]
 if slug in ('save_data','save_prepare','load_selected_save'):command+=['--input-dll',a.input_dll]
 with (a.output/(slug+'.log')).open('w') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'})
 rows.append({'slug':slug,'command':command,'exit':result.returncode});print(slug,result.returncode,flush=True)
 (a.output/'commands.json').write_text(json.dumps(rows,indent=2)+'\n')
raise SystemExit(any(r['exit'] for r in rows))
