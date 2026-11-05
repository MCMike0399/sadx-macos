"""Run unchanged prior faults into new artifact directories."""
import importlib.util, sys
from pathlib import Path
for name,script,output in [('animated','verify_faults.py','animated-faults'),
                           ('callback','callback_faults.py','callback-faults')]:
 path=Path('reference/packets/gamma-display-draw-2')/script
 spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 module.OUT=Path('build/draw-path-3')/output
 sys.argv=[str(path),'game/Sonic Adventure DX/sonic.exe']
 result=module.main()
 if result:raise SystemExit(result)
