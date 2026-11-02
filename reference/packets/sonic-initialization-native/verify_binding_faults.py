"""Compile faults in canonical native transports against the actual callback."""
import json
from pathlib import Path
import shlex
import subprocess
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'build/sonic-initialization-binding-faults';OUT.mkdir(exist_ok=True)
binding_path = ROOT / 'src/runtime/native_sonic_binding.cpp'
weld_path = ROOT / 'src/player/player_vertex_weld.cpp'
faults={
 'captured_allocation_reloaded':('binding.arena().write(allocation,','binding.arena().write(player.record->allocation,'),
 'captured_physics_reloaded':('animation::View view{binding.physics(player), allocation};','animation::View view{*player.record->physics, allocation};'),
 'active_table_consumed_before_matrix':('if (mode == 1) {', 'if (mode == 9) {'),
}
base=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc']
dry=subprocess.run(['make','-n','-W','tests/unit/sonic_initialization_native.mm','-o','build/native-controller-deps/include/SDL3/SDL.h','build/sonic-initialization-native-test'],cwd=ROOT,capture_output=True,text=True,check=True)
link=[line for line in dry.stdout.splitlines() if line.startswith('xcrun clang++') and 'tests/unit/sonic_initialization_native.mm' in line];assert len(link)==1
receipts=[]
for name,(old,new) in faults.items():
 source_path = weld_path if name == 'active_table_consumed_before_matrix' else binding_path
 source = source_path.read_text()
 assert source.count(old)==1,(name,source.count(old))
 changed=OUT/(name+'.cpp');changed.write_text(source.replace(old,new));obj=OUT/(name+'.o')
 command=base+['-c',str(changed),'-o',str(obj)]
 result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True);assert result.returncode==0,result.stderr
 command=shlex.split(link[0]);command.insert(command.index('build/sonic-initialization-callback.o'),str(obj));command[-1]=str(OUT/name)
 result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True);assert result.returncode==0,result.stderr
 result=subprocess.run([str(OUT/name),'game/imported'],cwd=ROOT,capture_output=True,text=True)
 assert result.returncode!=0 and 'Assertion failed' in result.stderr,(name,result.stdout,result.stderr)
 receipts.append(dict(name=name,exit=result.returncode,stdout=result.stdout,stderr=result.stderr,link=command))
(OUT/'receipts.json').write_text(json.dumps(receipts,indent=2)+'\n');print(len(receipts),'compiled native binding faults caught')
