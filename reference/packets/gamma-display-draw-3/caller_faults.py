"""Suppress body and arm independently in the real constructor/display caller."""
import json, shlex, subprocess
from pathlib import Path
OUT=Path('build/draw-path-3/caller-faults');OUT.mkdir(parents=True,exist_ok=True)
plan=subprocess.run(['make','-n','-j2','-o','build/native-controller-deps/include/SDL3/SDL.h',
                     '-W','tests/unit/native_gamma_display.mm','build/native-gamma-display-test'],
                    capture_output=True,text=True,check=True)
line=next(line for line in reversed(plan.stdout.splitlines())
          if line.startswith('xcrun clang++ ') and 'tests/unit/native_gamma_display.mm' in line)
source=Path('src/runtime/native_gamma_display_binding.cpp').read_text()
rows=[]
for name,start,end in [('body','void draw_table_action(', 'void draw_generated_action('),
                       ('arm','void draw_arm_action(', 'void calculate_arm_matrix(')]:
 a,b=source.index(start),source.index(end)
 section=source[a:b]
 old='    submit(context, static_cast<ObjectId>(*action.object.id), &motion, frame);'
 assert section.count(old)==1
 changed=source[:a]+section.replace(old,'    (void)motion;\n    (void)frame;')+source[b:]
 cpp=OUT/(name+'.cpp');cpp.write_text(changed)
 argv=shlex.split(line);argv.insert(argv.index('tests/unit/native_gamma_display.mm')+1,str(cpp));argv[-1]=str(OUT/(name+'-test'))
 p=subprocess.run(argv,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==0,p.stderr
 run=[argv[-1],'build/cs-private-assets'];q=subprocess.run(run,capture_output=True,text=True)
 (OUT/(name+'-run.log')).write_text(q.stdout+q.stderr)
 row=dict(name=name,compile_command=argv,compile_exit=p.returncode,command=run,exit=q.returncode,
          caught=q.returncode!=0 and 'body_draws > 98 && body_vertices > 2862' in q.stderr,
          stdout=q.stdout,stderr=q.stderr)
 rows.append(row);assert row['caught'],row
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows,indent=2))
