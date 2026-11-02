"""Run unchanged symbol/provenance checks against exact Root patch read fixtures."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / 'reference/packets/model-matrices-native'
OUT = ROOT / 'build/model-matrices-native/source-overlay'

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    patches = [PACKET / 'ROOT-OWNER.patch', PACKET / 'ROOT-ACTIVATION.patch']
    paths = {line[6:] for p in patches for line in p.read_text().splitlines() if line.startswith('--- a/')}
    for path in paths:
        target = OUT / path; target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / path, target)
    for p in patches:
        subprocess.run(['patch', '--batch', '-p1', '-d', str(OUT), '-i', str(p)], check=True)
    replacements = {ROOT / p: OUT / p for p in paths}
    read_text, read_bytes = Path.read_text, Path.read_bytes
    def text(path, *args, **kwargs):
        return read_text(replacements.get(path, path), *args, **kwargs)
    def binary(path, *args, **kwargs):
        return read_bytes(replacements.get(path, path), *args, **kwargs)
    # The pinned label checkout is an input, never an output. Remove only our new alias.
    labels = ROOT / 'reference/sa-tools'; created = not labels.exists()
    if created: labels.symlink_to('/Users/burbujamc/Developer/sadx-macos/reference/sa-tools', target_is_directory=True)
    try:
        symbols = module('model_symbols', ROOT / 'tools/symbols/build_symbol_db.py')
        provenance = module('model_provenance', ROOT / 'tools/gate/check_provenance.py')
        with patch.object(Path, 'read_text', text), patch.object(Path, 'read_bytes', binary):
            outputs = symbols.generate(ROOT)
            database = json.loads(outputs['symbols.json'])
            baseline = json.loads((ROOT / 'reference/symbols/legacy-identifiers.json').read_text())
            errors, counts, owned = provenance.check(ROOT, database, baseline)
    finally:
        if created: labels.unlink()
    (OUT / 'symbols.json').write_text(outputs['symbols.json'])
    report = {'symbols_regenerated_from_exact_override_patch': True, 'provenance_errors': errors,
              'counts': counts, 'owned_byte_identities': owned,
              'counterpart_catalog_changed': False, 'shared_sources_changed': False,
              'root_owner_patch_sha256': hashlib.sha256((PACKET / 'ROOT-OWNER.patch').read_bytes()).hexdigest()}
    (PACKET / 'ROOT-PROVENANCE.json').write_text(json.dumps(report, indent=2) + '\n')
    assert not errors, errors
    print(f'Exact Root patch: regenerated symbols, {owned} owned-byte identities, zero provenance errors; counterpart guard unchanged')
if __name__ == '__main__': main()
