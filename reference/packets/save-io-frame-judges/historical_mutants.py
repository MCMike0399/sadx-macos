"""Replay all five historical completion faults from the immutable packet script.

The original specification is parsed as data; its obsolete archive path and build
commands are never executed. Only copied fault variants are compiled locally.
"""
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess

spec=Path('reference/packets/save-completion/mutants.py')
tree=ast.parse(spec.read_text())
mutants=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='mutants' for t in n.targets))
source=Path('src/game/save_completion.cpp').read_text()
folder=Path('build/save-io-judges/historical-mutants');folder.mkdir(parents=True,exist_ok=True)
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures','-Wl,-dead_strip']
exports=folder/'exports.txt';exports.write_text('_sadx_native_save_completion\n')
rows=[]
for name,old,new,judge in mutants:
 assert old in source
 variant=folder/(name+'.cpp');variant.write_text(source.replace(old,new))
 if judge=='oracle':
  binary=folder/(name+'.dylib')
  command=flags+['-dynamiclib','-undefined','dynamic_lookup','-Wl,-exported_symbols_list,'+str(exports),'tests/probes/native_save_creation_probe.cpp',str(variant),'src/engine/task.cpp','src/engine/task_work.cpp','-o',str(binary)]
  run=['/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python','tools/oracles/verify_original_save_completion.py','build/save-io-judges/pinned/sonic.exe',str(binary),'--output',str(folder/(name+'.json'))]
 else:
  binary=folder/name
  command=flags+['tests/unit/save_completion_callbacks.cpp',str(variant),'src/engine/task.cpp','src/engine/task_work.cpp','-o',str(binary)]
  run=[str(binary)]
 compiled=subprocess.run(command,capture_output=True,text=True)
 (folder/(name+'.build.log')).write_text(compiled.stdout+compiled.stderr)
 row={'name':name,'specification_sha256':hashlib.sha256(spec.read_bytes()).hexdigest(),'fault_sha256':hashlib.sha256(variant.read_bytes()).hexdigest(),'compile_command':command,'run_command':run,'compile_exit':compiled.returncode,'run_exit':None,'detected':False}
 if not compiled.returncode:
  result=subprocess.run(run,capture_output=True,text=True,env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','PYTHONPATH':'tools/lib:tools/oracles'})
  (folder/(name+'.run.log')).write_text(result.stdout+result.stderr)
  row.update(run_exit=result.returncode,detected=result.returncode!=0)
  if judge=='oracle':row['report']=json.loads((folder/(name+'.json')).read_text())
 rows.append(row);(folder/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(name,row['compile_exit'],row['run_exit'],flush=True)
