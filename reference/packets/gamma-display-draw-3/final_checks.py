"""Final post-review build, caller, asset guards and fresh normal-input capture."""
import json, os, subprocess
from pathlib import Path
out=Path('build/draw-path-3/final');out.mkdir(parents=True,exist_ok=True)
commands=[
 ('build',['make','-j2','-o','build/native-controller-deps/include/SDL3/SDL.h',
           'build/sadx-native','build/native-gamma-display-test','build/cs-private-assets/.private-native-assets.json']),
 ('caller',['./build/native-gamma-display-test','build/cs-private-assets']),
 ('startup',['./build/character-pack-test','game/imported/characters/sonic-startup.actions']),
 ('selection',['./build/selection-character-pack-test','game/imported/characters/sonic-selection.actions','game/imported/characters/title.actions']),
 ('gamma-assets',['./build/animation-gamma-assets-test','build/cs-private-assets']),
 ('animated-unit',['./build/animated-model-test']),
 ('python',['python3','-m','unittest','discover','-s','tests/python','-p','test_private_native_assets.py']),
 ('caller-faults',['reference/python/bin/python','reference/packets/gamma-display-draw-3/caller_faults.py']),
 ('normal-input',['python3','tools/testing/check_native_character_select_display.py','--assets','build/cs-private-assets',
                  '--save-source','reference/updated-pc-reference/game/savedata','--output',str(out/'normal-input')]),
 ('check',['make','-j2','check'])]
rows=[]
for name,command in commands:
 p=subprocess.run(command,capture_output=True,text=True,env=dict(os.environ,PYTHONPATH='tools/lib:tools/oracles'))
 log=out/(name+'.log');log.write_text(p.stdout+p.stderr)
 rows.append(dict(name=name,command=command,exit=p.returncode,log=str(log)))
 (out/'commands.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(rows,indent=2))
