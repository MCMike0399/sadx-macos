#!/usr/bin/env python3
"""Compile discriminating faults without editing the candidate or its judge."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
DIRECTORY=ROOT/'build/particle-mutants'
FAULTS=[
 ('unsigned-count','return value < 32768 ? value : std::int32_t(value) - kIndexModulus;',
  'return std::int32_t(value) - (value < 32768 ? 0 : kIndexModulus - kIndexModulus);',1),
 ('skip-update-display','    display_task(scheduler, task);','    /* fault: omitted display */',2),
 ('skip-swapped-slot','            --index;','            /* fault: skip swapped particle */',2),
 ('invert-custom-liveness','update(particle, work->parameters, frames) == 0','update(particle, work->parameters, frames) != 0',1),
 ('wrong-free-slot','write_index(arena, free, signed_count(last_index), value','write_index(arena, free, index, value',1),
 ('drop-vertical-acceleration','const auto falling = drag_y - work->parameters.vertical_acceleration;',
  'const auto falling = drag_y;',1),
 ('reverse-release-order','release_buffer(arena, free);','release_buffer(arena, work->particles); (void)free;',1),
 ('skip-loop-counter','const auto loops = particle.word(Field::LoopCount) + 1;',
  'const auto loops = particle.word(Field::LoopCount);',1),
]
def main():
 DIRECTORY.mkdir(parents=True,exist_ok=True)
 original=(ROOT/'src/engine/particle_pool_lifecycle_native.cpp').read_text()
 results=[]
 for name,before,after,expected in FAULTS:
  assert original.count(before)==expected,(name,'anchor changed')
  directory=DIRECTORY/name;directory.mkdir(exist_ok=True)
  source=directory/'particle_pool_lifecycle_native.cpp';source.write_text(original.replace(before,after))
  probe=directory/'probe.dylib'
  command=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-Isrc','-dynamiclib',
   'tests/probes/native_particle_lifecycle_probe.cpp',str(source),'src/engine/task.cpp','src/engine/task_work.cpp',
   'src/cpu/x87_math.cpp','src/cpu/crt_integer.cpp','build/softfloat/softfloat.a','-o',str(probe)]
  build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
  (directory/'build.log').write_text(build.stdout+build.stderr)
  if build.returncode:raise RuntimeError(f'{name}: compile failed, not a caught fault')
  report=directory/'original.json'
  command=[sys.executable,'tools/oracles/verify_original_particle_lifecycle.py','--exe','game/Sonic Adventure DX/sonic.exe','--probe',str(probe),'--output',str(report),'--count','64']
  run=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
  (directory/'run.log').write_text(run.stdout+run.stderr)
  data=json.loads(report.read_text())
  caught=run.returncode==1 and data['value_failures']>0
  results.append(dict(name=name,caught=caught,exit_status=run.returncode,cases=data['cases'],value_failures=data['value_failures'],
   source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(probe.read_bytes()).hexdigest(),report_sha256=hashlib.sha256(report.read_bytes()).hexdigest()))
  print(name,'caught' if caught else 'SURVIVED',flush=True)
  if not caught:raise RuntimeError(name)
 (ROOT/'reference/packets/particle-lifecycle-native/mutants.json').write_text(json.dumps(dict(mutants=results),indent=2)+'\n')
if __name__=='__main__':main()
