"""Finite compiled faults for this model-matrix cluster; no judge is rewritten."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/model-matrices-native/faults'
CPP = ['src/engine/model_matrices.cpp', 'src/engine/ninja_matrix_composition.cpp',
       'src/player/model_motion_matrices.cpp']
FAULTS = [
    ('getter-equal-index', CPP[0], 'index >= state.count', 'index > state.count'),
    ('getter-destroys-rejected-output', CPP[0], 'return false;', '{ destination[15] = 0; return false; }'),
    ('limit-without-clamp', CPP[2], 'std::min(requested_node + 1U, std::uint32_t{128})', 'requested_node + 1U'),
    ('wrapped-limit', CPP[2], 'requested_node + 1U', 'requested_node'),
    ('root-sibling-followed', CPP[2], '(void)scanner.visit(motion.root());',
     'auto root = motion.root(); while (root) root = scanner.visit(root);'),
    ('mdata2-position-flag', CPP[2], '(channels == 2 && channel == Channel::Position)', 'false'),
    ('numeric-translate-order', CPP[2], 'translate_ninja_matrix(matrix, sampled[0], sampled[1], sampled[2], fp);',
     'coordinates.translate_matrix(matrix, {sampled[0], sampled[1], sampled[2]}, fp);'),
    ('matrix-product-order', CPP[1], 'auto sum = column == 3 ? term(3) : term(1);', 'auto sum = term(0);'),
    ('xyz-axis-order', CPP[1], 'coordinates.rotate_z(matrix, angles[2], fp);', 'coordinates.rotate_x(matrix, angles[2], fp);'),
]

def main():
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--exe', required=True)
    p.add_argument('--native-only', action='store_true')
    a = p.parse_args(); OUT.mkdir(parents=True, exist_ok=True)
    rows = []
    for name, path, before, after in ([] if a.native_only else FAULTS):
        directory = OUT / name; directory.mkdir(exist_ok=True)
        text = (ROOT / path).read_text(); assert text.count(before) == 1, (name, text.count(before))
        modified = directory / Path(path).name; modified.write_text(text.replace(before, after))
        sources = [str(modified) if f == path else f for f in CPP]
        library = directory / 'probe.dylib'
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
                   '-Isrc', '-dynamiclib', 'tests/probes/native_model_matrices_probe.cpp', *sources,
                   'build/libsadx-core.a', '-o', str(library)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'build.log').write_text(build.stdout + build.stderr)
        assert build.returncode == 0, name
        report = directory / 'report.json'
        run = subprocess.run([sys.executable, 'tools/oracles/verify_original_model_matrices.py', a.exe,
            str(library), '--cases', '0', '--output', str(report)], cwd=ROOT, capture_output=True, text=True)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        result = json.loads(report.read_text()) if report.exists() else {}
        caught = run.returncode == 1 and result.get('value_mismatches', 0) > 0
        assert caught, (name, run.returncode, run.stderr)
        rows.append({'name': name, 'compiled': True, 'caught_by_value_difference': caught,
                     'value_mismatches': result['value_mismatches'], 'report': str(report.relative_to(ROOT))})
    native_path = 'src/player/model_motion_source.cpp'
    native_faults = [
        ('imported-key-header-as-data', 'keys.bytes->data() + keys.offset + offset', 'keys.bytes->data() + offset'),
        ('generated-node-stride', '(*motion.type & 4U) ? 24 : 16', '(*motion.type & 4U) ? 16 : 24'),
        ('scene-root-instead-of-live-node', 'objects_.object(id)', 'objects_.object(id ? root() : id)'),
    ]
    for name, before, after in native_faults:
        directory = OUT / name; directory.mkdir(exist_ok=True)
        source = (ROOT / native_path).read_text(); assert source.count(before) == 1
        modified = directory / 'model_motion_source.cpp'; modified.write_text(source.replace(before, after))
        binary, observations = directory / 'test', directory / 'observations.tsv'
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
                   '-fobjc-arc', '-Isrc', 'tests/unit/model_motion_matrices.mm', str(modified),
                   'build/libsadx-native.a', '-framework', 'Foundation', '-o', str(binary)]
        build = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (directory / 'build.log').write_text(build.stdout + build.stderr)
        assert build.returncode == 0, name
        run = subprocess.run([str(binary), 'game/imported', str(observations)], cwd=ROOT, capture_output=True, text=True)
        (directory / 'run.log').write_text(run.stdout + run.stderr)
        caught = run.returncode != 0
        if not caught:
            compare = subprocess.run([sys.executable, 'reference/packets/model-matrices-native/verify_imported.py',
                a.exe, 'game/Sonic Adventure DX/system/CHRMODELS_orig.dll', str(observations),
                str(directory / 'original.json')], cwd=ROOT, capture_output=True, text=True)
            caught = compare.returncode == 1
        assert caught, name
        rows.append({'name': name, 'compiled': True, 'caught_by_native_owner_or_original_comparison': True,
                     'native_exit_status': run.returncode})
    output = ROOT / 'reference/packets/model-matrices-native/FAULTS.json'
    if a.native_only:
        prior = json.loads(output.read_text())['faults']
        rows = [r for r in prior if r['name'] not in {r['name'] for r in rows}] + rows
    output.write_text(json.dumps({'faults': rows, 'passed': True}, indent=2) + '\n')
    print(f'{len(rows)} compiled matrix faults caught by original value comparisons')
if __name__ == '__main__': main()
