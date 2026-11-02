"""Repeatability and failure-prefix checks for the additive Sonic metadata import."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/import'))
from sonic_animation_resources import import_sonic_animation_resources


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(dll, exe, pack):
    output = ROOT / 'build/sonic-animation-native/importer'
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output) as temporary:
        target = Path(temporary) / pack.name
        shutil.copyfile(pack, target)
        before = sha(target)
        first = import_sonic_animation_resources(dll, exe, target)
        metadata = target.parent / 'sonic-animation.json'
        proof = target.parent / 'sonic-animation.provenance.json'
        files = {path.name: sha(path) for path in (target, metadata, proof)}
        second = import_sonic_animation_resources(dll, exe, target)
        assert first == second and files == {path.name: sha(path) for path in (target, metadata, proof)}
        assert sha(target) == before
        failures = []
        for name in ('wrong-dll', 'wrong-exe', 'changed-pack'):
            if name == 'changed-pack':
                payload = bytearray(target.read_bytes())
                payload[-1] ^= 1
                target.write_bytes(payload)
            try:
                import_sonic_animation_resources(exe if name == 'wrong-dll' else dll,
                    dll if name == 'wrong-exe' else exe, target)
            except ValueError as error:
                failures.append(dict(name=name, rejected=True, diagnostic=str(error)))
            else:
                raise AssertionError(name + ' accepted')
            assert sha(metadata) == files[metadata.name] and sha(proof) == files[proof.name]
        return dict(pack_unchanged=True, deterministic_metadata=True, deterministic_provenance=True,
                    failure_prefix_metadata_unchanged=True, failures=failures)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('dll', type=Path)
    parser.add_argument('exe', type=Path)
    parser.add_argument('pack', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = verify(args.dll, args.exe, args.pack)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
