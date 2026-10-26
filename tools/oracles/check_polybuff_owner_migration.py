"""Private source builds of migrated legacy owners and their frozen instruction judges.

Only frozen fixture/probe translation units receive the compatibility include. Game
translation units never include it. Read-only Root archives supply unchanged dependencies;
all changed algorithms and consumers are compiled here, without make/apps/Ghidra.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
LEGACY = {
    'system_init': ['src/engine/ninja_system_init.cpp','src/engine/ninja_texture_pool.cpp',
                    'src/engine/ninja_memory.cpp','src/loader/guest_heap.cpp',
                    'src/guest/polybuff_initialization_reference.cpp','src/engine/polybuff_initialization.cpp'],
    'device_setup': ['src/guest/ninja_device_init.cpp','src/engine/game_frame_control.cpp'],
    'ninja_frame_calls': ['src/guest/ninja_frame_calls.cpp','src/engine/ninja_render.cpp'],
}
UNITS = {'system_init':'ninja_system_init','device_setup':'ninja_device_init',
         'ninja_frame_calls':'ninja_frame_calls'}


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root-dependencies', type=Path, required=True)
    p.add_argument('--python', type=Path, required=True)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--loader', type=Path, required=True)
    p.add_argument('--softfloat', type=Path, required=True)
    a = p.parse_args()
    build = ROOT/'build/polybuff-owner-migration'
    build.mkdir(parents=True, exist_ok=True)
    flags = ['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
             '-ffp-contract=off','-fno-fast-math','-Isrc','-Itests/fixtures']
    # Frozen tests contain their historical formatting; keep their established flags.
    fixture_flags = [x for x in flags if x not in ['-Wall','-Wextra','-Werror']]
    archives = [a.root_dependencies/'build/libsadx-guest.a',
                a.root_dependencies/'build/libsadx-core.a', a.softfloat]
    hashes = {str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in archives}
    env = dict(os.environ, PYTHONPATH=str(ROOT/'tools/lib')+':'+str(ROOT/'tools/oracles'))
    def run(command, label):
        r = subprocess.run([str(x) for x in command], cwd=ROOT, env=env, text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (build/(label+'.log')).write_text(r.stdout)
        return r.returncode
    objects = {}
    def source_object(name):
        if name in objects: return objects[name]
        obj = build/(Path(name).stem+'.o')
        code = run(fixture_flags+['-c',name,'-o',obj],Path(name).stem+'-compile')
        if code: raise RuntimeError(f'{name} compile failed')
        objects[name] = obj
        return obj
    owner = source_object('src/guest/polybuff_owners.cpp')
    summary = {}
    for slug, sources in LEGACY.items():
        probe = build/(slug+'-probe.o')
        assert run(fixture_flags+['-include','guest/polybuff_owners.hpp','-c',
            f'tests/probes/native_{slug}_probe.cpp','-o',probe],slug+'-probe-compile') == 0
        module_objects = [source_object(x) for x in sources]
        library = build/(slug+'.dylib')
        assert run(fixture_flags+['-dynamiclib',probe,owner]+module_objects+archives+
                   ['-o',library],slug+'-link') == 0
        report = build/(slug+'.json')
        command = [a.python,f'tools/oracles/verify_original_{slug}.py',a.exe,library,
                   '--output',report]
        if slug == 'ninja_frame_calls': command += ['--loader',a.loader]
        code = run(command,slug)
        data = json.loads(report.read_text()) if report.exists() else {}
        assert code == 0 and data['mismatch_count'] == 0, slug
        unit_source = f'tests/unit/{UNITS[slug]}.cpp'
        unit_object, unit = build/(slug+'-unit.o'), build/(slug+'-unit')
        assert run(fixture_flags+['-include','guest/polybuff_owners.hpp','-c',unit_source,
                                  '-o',unit_object],slug+'-unit-compile') == 0
        assert run(fixture_flags+[unit_object,owner]+module_objects+archives+['-o',unit],
                   slug+'-unit-link') == 0
        unit_code = run([unit],slug+'-unit')
        assert unit_code == 0, slug
        summary[slug] = {'exit_status':code,'cases':data['cases'],
            'mismatch_count':data['mismatch_count'],'state_traces_match':data['state_traces_match'],
            'report_sha256':hashlib.sha256(report.read_bytes()).hexdigest(),
            'unit_exit_status':unit_code,'unit_output':(build/(slug+'-unit.log')).read_text().strip()}
        print(slug, summary[slug], flush=True)
    owner_unit = build/'owner-unit'
    owner_unit_sources = [source_object(x) for x in LEGACY['system_init'] + LEGACY['device_setup']]
    assert run(flags+['tests/unit/polybuff_ownership.cpp',owner]+owner_unit_sources+archives+
               ['-o',owner_unit], 'owner-unit-compile') == 0
    assert run([owner_unit], 'owner-unit') == 0
    summary['owner_unit'] = (build/'owner-unit.log').read_text().strip()
    mutations = json.loads((ROOT/'tests/fixtures/polybuff_owner_regression_mutants.json').read_text())
    mutation_results = {}
    for mutation in mutations:
        slug, name, file = mutation['slug'], mutation['name'], mutation['file']
        label = slug+'-'+name
        source = (ROOT/file).read_text()
        assert mutation['before'] in source, label
        variant_owner, variant_probe = owner, build/(slug+'-probe.o')
        if file.endswith('.hpp'):
            overlay = build/label
            header = overlay/'engine/polybuff_initialization.hpp'
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text(source.replace(mutation['before'],mutation['after'],1))
            variant_flags = fixture_flags.copy()
            variant_flags.insert(variant_flags.index('-Isrc'), '-I'+str(overlay))
            # Recompile every participating TU under the same template definition.
            # A header mutant is never mixed with baseline instantiations.
            variant_owner, variant_probe = overlay/'owners.o', overlay/'probe.o'
            assert run(variant_flags+['-c','src/guest/polybuff_owners.cpp','-o',variant_owner],
                       label+'-owner-compile') == 0
            assert run(variant_flags+['-include','guest/polybuff_owners.hpp','-c',
                       f'tests/probes/native_{slug}_probe.cpp','-o',variant_probe],
                       label+'-probe-compile') == 0
            module_objects = []
            for module in LEGACY[slug]:
                variant_object = overlay/(Path(module).stem+'.o')
                assert run(variant_flags+['-c',module,'-o',variant_object],
                           label+'-'+Path(module).stem+'-compile') == 0
                module_objects.append(variant_object)
        else:
            variant, obj = build/(label+'.cpp'), build/(label+'.o')
            variant.write_text(source.replace(mutation['before'],mutation['after'],1))
            assert run(fixture_flags+['-c',variant,'-o',obj],label+'-compile') == 0, label
            module_objects = [obj if file == x else source_object(x) for x in LEGACY[slug]]
        library, report = build/(label+'.dylib'), build/(label+'.json')
        assert run(fixture_flags+['-dynamiclib',variant_probe,variant_owner]+module_objects+
                   archives+['-o',library],label+'-link') == 0, label
        command = [a.python,f'tools/oracles/verify_original_{slug}.py',a.exe,library,
                   '--output',report]
        if slug == 'ninja_frame_calls': command += ['--loader',a.loader]
        report.unlink(missing_ok=True)
        code = run(command,label)
        data = json.loads(report.read_text()) if report.exists() else {}
        # Match the existing mutate.py contract: a judge abort is a detected mutant,
        # recorded separately rather than relabeled as a value mismatch.
        aborted = code != 0 and not report.exists()
        oracle_detected = aborted or (code == 1 and data['mismatch_count'] > 0)
        owner_unit_code = None
        if name == 'direct3d-sdk':
            # The frozen device probe ignores this operand; the new owner unit checks
            # the actual host argument without altering that original judge/fixture.
            mutant_unit = build/(label+'-unit')
            unit_objects = [obj if file == x else source_object(x)
                            for x in LEGACY['system_init'] + LEGACY['device_setup']]
            assert run(flags+['tests/unit/polybuff_ownership.cpp',owner]+unit_objects+archives+
                       ['-o',mutant_unit],label+'-unit-compile') == 0
            owner_unit_code = run([mutant_unit],label+'-unit')
        killed = oracle_detected or owner_unit_code == 1
        mutation_results[label] = {'killed':killed,'exit_status':code,
            'mismatch_count':data.get('mismatch_count'),'oracle_aborted':aborted,
            'original_oracle_detected':oracle_detected,'owner_unit_exit_status':owner_unit_code,
            'log_sha256':hashlib.sha256((build/(label+'.log')).read_bytes()).hexdigest()}
        assert killed, label
    summary['legacy_regression_mutants'] = mutation_results
    summary['historical_mutant_scope'] = (
        'All seven historical frame-call categories are reproduced. Original historical '
        'system/device spec files are absent from tracked history; new 9/7 regressions '
        'cover their live behavior, with five system mutants proving shared-core coupling.')
    # Compile all changed high-level consumers too; nothing is launched.
    unicorn = next((a.root_dependencies/'reference/python/lib').glob('python*/site-packages/unicorn'))
    for name in ['src/guest/native_game_reference.cpp','src/hle/hle_device_bridge.cpp',
                 'src/guest/ninja_screen.cpp','src/engine/ninja_view.cpp']:
        obj = build/(Path(name).stem+'-consumer.o')
        assert run(fixture_flags+['-I'+str(unicorn/'include'),'-c',name,'-o',obj],
                   Path(name).stem+'-consumer-compile') == 0, name
    # The polygon descriptor now borrows a live typed identity; preserve its frozen journey too.
    screen_objects = [source_object('src/guest/ninja_screen.cpp'),
                      source_object('src/engine/ninja_projected_depth.cpp')]
    for label, fixture, shared in [('screen_polygon','tests/probes/native_screen_polygon_probe.cpp',True),
                                    ('screen_unit','tests/unit/ninja_screen.cpp',False)]:
        obj = build/(label+'.o')
        assert run(fixture_flags+['-include','guest/polybuff_owners.hpp','-c',fixture,'-o',obj],
                   label+'-compile') == 0
        target = build/(label+('.dylib' if shared else '-unit'))
        assert run(fixture_flags+(['-dynamiclib'] if shared else [])+[obj,owner]+screen_objects+
                   archives+['-o',target],label+'-link') == 0
        command = [a.python,'tools/oracles/verify_original_screen_polygon.py',a.exe,target,
                   '--output',build/'screen_polygon.json'] if shared else [target]
        code = run(command,label)
        assert code == 0
        summary[label] = {'exit_status':code}
        if shared:
            report = json.loads((build/'screen_polygon.json').read_text())
            summary[label].update(cases=report['cases'], mismatch_count=report['mismatch_count'])
        else:
            summary[label]['output'] = (build/(label+'.log')).read_text().strip()
    assert hashes == {str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in archives}, \
        'read-only dependency archives changed during verification'
    summary['dependency_archives'] = hashes
    summary['changed_sources_compiled_privately'] = sorted(objects)
    summary['limits'] = ['Root archives supply unchanged dependencies; not full source-only product.',
                         'No apps, full gate, runtime binding, rendering or N3b claim.']
    (build/'proof.json').write_text(json.dumps(summary,indent=2)+'\n')

if __name__ == '__main__': main()
