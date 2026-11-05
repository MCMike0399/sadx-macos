"""Isolated compiled source faults; original cases/tolerances remain unchanged."""
import json, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'build/draw-path-3/faults'
OUT.mkdir(parents=True,exist_ok=True)
flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off',
       '-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc','-dynamiclib']
faults=[
 ('suppressed-arm','src/player/gamma_display_tail.cpp','    draw_arm_action(context);','    (void)context;'),
 ('matrix-before-arm','src/player/gamma_display_tail.cpp',
  '    draw_arm_action(context);\n    calculate_arm_matrix(context);',
  '    calculate_arm_matrix(context);\n    draw_arm_action(context);'),
 ('missing-control-restore','src/player/gamma_display_tail.cpp','    restore_control(context);','    (void)context;'),
 ('missing-callback-clear','src/player/gamma_display_action.cpp','    clear_object_callback(context);','    (void)context;')]
rows=[]
for name,path,old,new in faults:
 source=Path(path).read_text();assert source.count(old)==1
 mutant=OUT/(name+'.cpp');mutant.write_text(source.replace(old,new))
 ordinary=name=='missing-callback-clear'
 probe='tests/probes/native_gamma_object_callback_probe.cpp' if ordinary else 'tests/probes/native_gamma_tail_probe.cpp'
 library=OUT/(name+'.dylib')
 command=flags+[probe,str(mutant),'build/libsadx-native.a','-o',str(library)]
 p=subprocess.run(command,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==0,p.stderr
 judge='gamma_ordinary_body' if ordinary else 'gamma_tail'
 report=OUT/(name+'.json')
 argv=['reference/python/bin/python','tools/oracles/verify_original_'+judge+'.py',
       'game/Sonic Adventure DX/sonic.exe',str(library),'--output',str(report)]
 import os
 env=dict(os.environ,PYTHONPATH='tools/lib:tools/oracles')
 q=subprocess.run(argv,capture_output=True,text=True,env=env)
 (OUT/(name+'-run.log')).write_text(q.stdout+q.stderr)
 result=json.loads(report.read_text())
 row=dict(name=name,compile_command=command,compile_exit=p.returncode,command=argv,exit=q.returncode,
          value_failures=result['value_failures'],cases=result['cases'],caught=q.returncode==1 and result['value_failures']>0)
 rows.append(row);assert row['caught'],row
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(rows,indent=2))
