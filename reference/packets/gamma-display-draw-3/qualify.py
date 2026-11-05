"""Focused packet qualification, including strict partial exit codes; no full gate."""
import hashlib, json, os, subprocess
from pathlib import Path
OUT=Path('build/draw-path-3/qualified');OUT.mkdir(parents=True,exist_ok=True)
rows=[]
def run(name,command):
 env=dict(os.environ,PYTHONPATH='tools/lib:tools/oracles')
 p=subprocess.run(command,capture_output=True,text=True,env=env)
 log=OUT/(name+'.log');log.write_text(p.stdout+p.stderr)
 row=dict(name=name,command=command,exit=p.returncode,log=str(log));rows.append(row)
 (OUT/'commands.json').write_text(json.dumps(rows,indent=2)+'\n')
 return row
exe='game/Sonic Adventure DX/sonic.exe';python='reference/python/bin/python'
for name,args in [
 ('caller',['./build/native-gamma-display-test','build/cs-private-assets']),
 ('animated-unit',['./build/animated-model-test']),
 ('startup-pack',['./build/character-pack-test','game/imported/characters/sonic-startup.actions']),
 ('selection-pack',['./build/selection-character-pack-test','game/imported/characters/sonic-selection.actions','game/imported/characters/title.actions']),
 ('gamma-assets',['./build/animation-gamma-assets-test','build/cs-private-assets']),
 ('python',['python3','-m','unittest','discover','-s','tests/python','-p','test_private_native_assets.py'])]:run(name,args)
for slug,library,extra in [
 ('gamma_display','gamma-display',['--body']),
 ('gamma_ordinary_body','gamma-ordinary-body',[]),
 ('gamma_object_callback','gamma-ordinary-body',[]),
 ('animated_model','animated-model',[]),
 ('animated_model','animated-model',['--mdata3']),
 ('gamma_tail','gamma-tail',[])]:
 name=slug+('-mdata3' if extra==['--mdata3'] else '')
 base=[python,'tools/oracles/verify_original_'+slug+'.py',exe,'build/native-'+library+'-probe.dylib']+extra
 run(name+'-dev',base+['--output',str(OUT/(name+'-dev.json'))])
 if slug!='gamma_display':
  seed={'gamma_ordinary_body':'0x9e4800b4','gamma_object_callback':'0x9e47f990','animated_model':'0x9e4053d1','gamma_tail':'0x9e480135'}[slug]
  count='4096' if slug=='animated_model' else '2048'
  run(name+'-holdout',base+['--cases',count,'--seed',seed,'--output',str(OUT/(name+'-holdout.json'))])
run('new-faults',[python,'reference/packets/gamma-display-draw-3/verify_faults.py'])
run('inherited-faults',[python,'reference/packets/gamma-display-draw-3/rerun_inherited_faults.py'])
run('caller-faults',[python,'reference/packets/gamma-display-draw-3/caller_faults.py'])
run('normal-input',['python3','tools/testing/check_native_character_select_display.py',
    '--assets','build/cs-private-assets','--save-source','reference/updated-pc-reference/game/savedata',
    '--output',str(OUT/'normal-input')])
run('check',['make','-j2','check'])
print(json.dumps(rows,indent=2))
