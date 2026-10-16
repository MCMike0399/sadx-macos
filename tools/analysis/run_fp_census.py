#!/usr/bin/env python3
"""Observe reference entry FP controls, retaining zero coverage and trace parity limits."""

import argparse
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/lib'))
from extract import SOURCE_SHA256


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(root=ROOT):
    subprocess.run(['make', '-s', 'build/libsadx-guest.a', 'build/libsadx-core.a'], cwd=root, check=True)
    unicorn = sorted((root / 'reference/python/lib').glob('python*/site-packages/unicorn'))
    if len(unicorn) != 1:
        raise ValueError('expected one reference Unicorn installation')
    source = root / 'tools/analysis/fp_control_census.cpp'
    inputs = [source, root / 'build/libsadx-guest.a', root / 'build/libsadx-core.a',
              unicorn[0] / 'lib/libunicorn.a']
    binary = root / 'build/fp-control-census'
    command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
               '-ffp-contract=off', '-fno-fast-math', '-arch', 'arm64', '-mmacosx-version-min=14.0',
               '-I' + str(root / 'src'), '-I' + str(unicorn[0] / 'include'),
               *map(str, inputs), '-lpthread', '-o', str(binary)]
    identity = dict(inputs={str(p.relative_to(root)): sha(p) for p in inputs}, command=command)
    metadata = root / 'build/fp-control-census-build.json'
    if not binary.exists() or not metadata.exists() or json.loads(metadata.read_text()) != identity:
        subprocess.run(command, cwd=root, check=True)
        metadata.write_text(json.dumps(identity, indent=2) + '\n')
    return binary


def player_targets(root=ROOT):
    """Conservative union of original entry/range keys in existing player oracle reports."""
    inventory = {f['address'] for f in json.loads((root / 'reference/inventory/functions.json').read_text())['functions']}
    symbols = {s['address']: s['name'] for s in json.loads((root / 'build/symbols/symbols.json').read_text())['symbols']
               if s['module'] == 'SONIC'}
    targets = {}

    def add(value, report):
        if not isinstance(value, str):
            return
        try:
            at = hex(int(value.split('-')[0], 16))
        except ValueError:
            return
        if at in inventory:
            row = targets.setdefault(at, dict(name=symbols.get(at), evidence_paths=[]))
            if report not in row['evidence_paths']:
                row['evidence_paths'].append(report)

    def collect(node, report):
        if isinstance(node, dict):
            for key, value in node.items():
                if key in {'source_function', 'source_body', 'source_range', 'function'} and isinstance(value, dict):
                    add(value.get('address'), report)
                if key == 'source_function_va':
                    add(value, report)
                if key in {'source_functions', 'source_ranges', 'original_ranges_sha256', 'original_range_sha256'}:
                    if isinstance(value, dict):
                        for at in value:
                            add(at, report)
                    elif isinstance(value, list):
                        for item in value:
                            if isinstance(item, dict):
                                add(item.get('address'), report)
                collect(value, report)
        elif isinstance(node, list):
            for value in node:
                collect(value, report)

    slugs = {'air_acceleration', 'ground_acceleration', 'alternate_acceleration', 'floor_angles',
             'axis_projection', 'collision_shapes', 'scene_input_producer', 'scene_mesh_bounds',
             'sonic_player', 'sonic_actions'}
    for path in sorted((root / 'reference/oracle-reports').glob('*.json')):
        if path.stem.startswith('player_') or path.stem in slugs:
            collect(json.loads(path.read_text()), path.relative_to(root).as_posix())
            script = root / f'tools/oracles/verify_original_{path.stem}.py'
            if script.exists():
                # Legacy summaries omit some entry addresses. Include every literal
                # original procedure referenced by their verifier, retaining provenance.
                # This deliberately over-includes controlled callees/dependencies.
                for node in ast.walk(ast.parse(script.read_text())):
                    if isinstance(node, ast.Constant) and type(node.value) is int:
                        add(hex(node.value), script.relative_to(root).as_posix())
    return targets


def summarize(site):
    samples = site['samples']
    calls = sum(row[3] for row in samples)
    controls = sorted({row[0] for row in samples})
    return dict(address=hex(site['address']), name=site['name'], bound_override=site['bound'],
                calls=calls, fpcw=[hex(v) for v in controls],
                mxcsr=[hex(v) for v in sorted({row[1] for row in samples})],
                mxcsr_control=[hex(v) for v in sorted({row[1] & ~0x3f for row in samples})],
                samples=[dict(fpcw=hex(cw), mxcsr=hex(mxcsr), caller_return_address=hex(caller), calls=count)
                         for cw, mxcsr, caller, count in samples],
                missing_caller_return=site['missing_return'],
                observed_cw_precondition=(controls == [0x7f]) if calls else None)


def parity(baseline, traced):
    common = min(len(baseline['digests']), len(traced['digests']))
    first = next((i + 1 for i in range(common) if baseline['digests'][i] != traced['digests'][i]), None)
    equal = (first is None and baseline['digests'] == traced['digests'] and
             baseline['clocks'] == traced['clocks'] and baseline['modes'] == traced['modes'] and
             baseline['stop'] == traced['stop'] == 'census frames done')
    return dict(passed=equal, first_digest_mismatch=first, baseline_frames=baseline['frames'],
                traced_frames=traced['frames'], clocks_equal=baseline['clocks'] == traced['clocks'],
                modes_equal=baseline['modes'] == traced['modes'], stop=traced['stop'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, default=ROOT / 'reference/updated-pc-reference/game')
    parser.add_argument('--system', type=Path, default=ROOT / 'reference/windows-system')
    parser.add_argument('--output', type=Path, default=ROOT / 'reference/fp-control-census.json')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--smoke', action='store_true', help='120-frame development run, not the full census')
    args = parser.parse_args()
    binary = build()
    if args.self_test:
        return subprocess.run([str(binary), '--self-test'], cwd=ROOT).returncode
    if sha(args.game / 'sonic.exe') != SOURCE_SHA256:
        raise ValueError('unexpected sonic.exe identity')
    targets = player_targets()
    if '0x44c270' not in targets:
        raise ValueError('ground-acceleration entry missing from player target inventory')
    target_file = ROOT / 'build/fp-control-targets.tsv'
    target_file.write_text(''.join(f'{at}\t{row["name"] or "unnamed"}\n' for at, row in sorted(targets.items())))
    runs = {}
    for profile, frames in [('intro', 120 if args.smoke else 7228), ('controller', 120 if args.smoke else 900)]:
        for variant in ['plain', 'original', 'native']:
            with tempfile.TemporaryDirectory(prefix='fp-census-', dir=ROOT / 'build') as temporary:
                output = ROOT / f'build/fp-census-{profile}-{variant}.json'
                with output.open('w') as handle:
                    result = subprocess.run([str(binary), str(args.game), str(args.system), str(target_file),
                                             profile, variant, str(frames), str(Path(temporary) / 'overlay')],
                                            cwd=ROOT, stdout=handle)
                if result.returncode:
                    raise ValueError(f'{profile}/{variant} collector failed; inspect {output}')
                runs[(profile, variant)] = json.loads(output.read_text())
    report = dict(schema=1, smoke=args.smoke, source_sha256=SOURCE_SHA256,
                  collector_sha256=sha(ROOT / 'tools/analysis/fp_control_census.cpp'),
                  runner_sha256=sha(Path(__file__)), build=json.loads((ROOT / 'build/fp-control-census-build.json').read_text()),
                  profile_patches_sha256=sha(ROOT / 'tools/gate/profile_patches.json'),
                  fpu_sites_sha256=sha(ROOT / 'reference/inventory/fpu-sites.json'), player_targets=targets,
                  stage_replay=dict(available=False, reason='No stage replay is registered in tests/unit/manifest.tsv; ROADMAP M3 stage replays remain open.'),
                  runs=[], comparisons=[], limits=[
                      'Reference Unicorn observations under the pinned profile; not Windows hardware control-word capture.',
                      'Entry observations do not authorize plain-float arithmetic alone: STYLE 7 also requires assembly, oracle and mutant proofs.',
                      'Zero calls mean unobserved; no player-physics control-word inference from intro/title coverage.',
                      'Original variant observes original guest entry instructions; native variant observes bridge entry plus unbound player entries.',
                  ])
    for (profile, variant), run in runs.items():
        encoded = json.dumps(run['digests'], separators=(',', ':')).encode()
        report['runs'].append(dict(profile=profile, variant=variant, frames=run['frames'], stop=run['stop'],
                                  modes=run['modes'], digest_sha256=hashlib.sha256(encoded).hexdigest(),
                                  sites=[summarize(site) for site in run['sites']]))
        if variant != 'plain':
            report['comparisons'].append(dict(profile=profile, variant=variant,
                                               **parity(runs[(profile, 'plain')], run)))
    report['all_comparisons_passed'] = all(c['passed'] for c in report['comparisons'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{args.output}: {len(targets)} player/dependency targets; parity={report["all_comparisons_passed"]}')
    return 0 if report['all_comparisons_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
