"""Fresh verified saves and the unchanged drawn-mode checker; never forces a present."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
source=Path('reference/updated-pc-reference/game/savedata')
receipt=json.loads(Path('reference/production-integration-20261102/sonic-initialization-fan-source-only.json').read_text())['save_input']
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
before={name:sha(source/name) for name in receipt}
assert before==receipt
command=['python3','tools/testing/check_native_character_select_display.py','--assets','build/cs-private-assets',
         '--save-source',str(source),'--output',str(a.output),'--frame','1550']
p=subprocess.run(command,capture_output=True,text=True)
a.output.with_suffix('.log').write_text(p.stdout+p.stderr)
after={name:sha(source/name) for name in receipt}
assert before==after
report=dict(command=command,exit=p.returncode,source_saves_before=before,source_saves_after=after,
            app_sha256=sha(Path('build/sadx-native')),source_saves_unchanged=True)
if (a.output/'result.json').exists():report['result']=json.loads((a.output/'result.json').read_text())
a.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='result'},indent=2))
raise SystemExit(p.returncode)
