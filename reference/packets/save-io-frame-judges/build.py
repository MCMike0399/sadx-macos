"""Serial local probe build, with exact command/dependency hashes in a receipt.

No native archives, remote dependencies or Make discovery. Existing frozen
libraries must not be overwritten. Caller supplies a fresh output path.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
assert not a.output.exists(),'immutable output already exists'
a.output.parent.mkdir(parents=True,exist_ok=True)
probes=['save_creation','save_file_list','save_list_free','save_frame','save_data']
sources=[f'tests/probes/native_{x}_probe.cpp' for x in probes]+[
 'src/game/save_creation.cpp','src/game/save_data.cpp','src/game/save_file_list.cpp',
 'src/game/save_frame.cpp','src/game/save_write.cpp','src/game/save_completion.cpp',
 'src/game/file_select.cpp','src/engine/task.cpp','src/engine/task_work.cpp','src/game/game_bootstrap.cpp']
exports=[]
for file in sources[:len(probes)]:
 exports+=re.findall(r'extern "C" [\w:]+ (sadx_\w+)\(',Path(file).read_text())
export_file=a.output.with_suffix('.exports.txt');export_file.write_text(''.join('_'+x+'\n' for x in exports))
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
dependencies=set(sources)
raw=subprocess.check_output(flags+['-MM',*sources],text=True)
for line in raw.replace('\\\n',' ').splitlines():dependencies.update(line.split(':',1)[1].split())
sha=lambda file:hashlib.sha256(Path(file).read_bytes()).hexdigest()
command=flags+['-dynamiclib','-undefined','dynamic_lookup','-Wl,-dead_strip','-Wl,-exported_symbols_list,'+str(export_file),*sources,'-o',str(a.output)]
result=subprocess.run(command,capture_output=True,text=True)
a.output.with_suffix('.build.log').write_text(result.stdout+result.stderr)
receipt={'command':command,'exit':result.returncode,'dependencies':{file:sha(file) for file in sorted(dependencies)},'library_sha256':sha(a.output) if result.returncode==0 else None}
a.output.with_suffix('.build.json').write_text(json.dumps(receipt,indent=2)+'\n')
raise SystemExit(result.returncode)
