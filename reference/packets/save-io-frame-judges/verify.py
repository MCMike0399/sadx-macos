"""Check local packet artifacts and optional immutable before-source closure.

Missing inherited corpora are reported, never dropped or resealed. A packet
integrity pass does not mean the repository judge manifest passes.
"""
import argparse
import hashlib
import json
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--source-freeze',action='store_true');a=p.parse_args()
packet=Path(__file__).parent
sha=lambda file:hashlib.sha256(Path(file).read_bytes()).hexdigest()
qualification=json.loads((packet/'qualification.json').read_text())
errors=[]
for path,digest in qualification['local_artifacts'].items():
 if not Path(path).is_file() or sha(path)!=digest:errors.append(path)
delta=json.loads((packet/'proposed-manifest-delta.json').read_text())
for path,digest in delta['added'].items():
 if not Path(path).is_file() or sha(path)!=digest:errors.append(path)
for path,pins in delta['append_only_changes'].items():
 if not Path(path).is_file() or sha(path)!=pins['current']:errors.append(path)
freeze=json.loads((packet/'source-freeze.json').read_text())
if a.source_freeze:
 for path,digest in freeze['files'].items():
  if path.startswith('src/') and (not Path(path).is_file() or sha(path)!=digest):errors.append(path)
missing=[path for path in freeze['inherited_manifest'] if not Path(path).is_file()]
print(json.dumps({'artifact_count':len(qualification['local_artifacts']),'integrity_errors':errors,'inherited_missing_count':len(missing),'inherited_manifest_refreshed':False,'source_freeze_checked':a.source_freeze},indent=2))
raise SystemExit(bool(errors))
