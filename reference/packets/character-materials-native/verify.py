"""Focused native consumer faults and unchanged original renderer/Gamma judges.

Run after the documented -j2 build. Outputs stay under this worktree's build/.
No source, judge, fixture, archive or Root cache is modified.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path[:0] = [str(ROOT / 'tools/oracles'), str(ROOT / 'tools/lib')]
from check_basic_model_packet import MUTANTS
from verify_original_gamma_selection_core import fixtures
OUT = ROOT / 'build/character-materials/qualification'
FLAGS = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures']
PYTHON = ROOT / 'reference/python/bin/python'
EXE = ROOT / 'game/Sonic Adventure DX/sonic.exe'

def run(command, name, expected=0):
    with (OUT / (name + '.log')).open('w') as stream:
        result = subprocess.run(list(map(str, command)), cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode != expected:
        raise RuntimeError(f'{name}: expected {expected}, got {result.returncode}; see log')
    return result.returncode

def oracle(kind, library, name, expected=2, holdout=False):
    output = OUT / (name + '.json')
    command = [PYTHON, f'tools/oracles/verify_original_{kind}.py', EXE, library, '--output', output]
    if holdout:
        command += ['--cases', '1800', '--seed', '7914850'] if kind == 'basic_model' else ['--holdout']
    run(command, name, expected)
    return json.loads(output.read_text())

def compile_source(path, text, name):
    cpp, obj = OUT / (name + '.cpp'), OUT / (name + '.o')
    cpp.write_text(text)
    run(FLAGS + ['-fPIC', '-c', cpp, '-o', obj], name + '-compile')
    return obj

def probe(kind, core, probe_obj, name):
    library = OUT / (name + '.dylib')
    run(FLAGS + ['-dynamiclib', core, probe_obj, 'build/libsadx-core.a', '-o', library], name + '-link')
    return library

def patch(source, before, after):
    assert source.count(before) == 1, before
    return source.replace(before, after)

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    result = {'native_programs': {}, 'mutants': []}
    for name, args in [('native-gamma-binding', ['game/imported']),
                       ('native-gamma-weld-cursors', ['game/imported']),
                       ('animation-gamma-shared-binding', ['game/imported']),
                       ('animation-gamma-assets', ['game/imported']),
                       ('gamma-setup-caller', ['game/imported/gamma-setup.bin']), ('basic-model', []),
                       ('model-binding', ['game/imported/characters/sonic-selection.actions', 'game/imported/system'])]:
        result['native_programs'][name] = run([f'build/{name}-test', *args], name)
    # These two faults compile and link against the ACTUAL native application bindings.
    faults = [
        ('immutable-renderer', 'src/runtime/model_draw.cpp',
         '    basic_model::draw_model(id, context.environment.pack, context.environment.scene.materials,',
         '    const character_pack::MaterialState immutable(context.environment.pack);\n'
         '    basic_model::draw_model(id, context.environment.pack, immutable,'),
        ('detached-gamma', 'src/runtime/native_gamma_binding.cpp',
         '        return context.scene.materials.material(row.materials, row.record_index);',
         '        static SceneMaterial detached;\n'
         '        detached = context.scene.materials.material(row.materials, row.record_index);\n'
         '        return detached;')]
    for name, path, before, after in faults:
        obj = compile_source(path, patch((ROOT / path).read_text(), before, after), name)
        binary = OUT / (name + '-test')
        run(FLAGS + ['-fobjc-arc', 'tests/unit/native_gamma_binding.mm', obj,
                     'build/libsadx-native.a', '-framework', 'Foundation', '-o', binary], name + '-test-link')
        rc = run([binary, 'game/imported'], name + '-test', 1)
        text = (OUT / (name + '-test.log')).read_text()
        expected = ('actual recorded draw must observe' if name == 'immutable-renderer' else
                    'Gamma resolves the shared material identity')
        assert expected in text, text
        result['mutants'].append(dict(name=name, compiled=True, exit_status=rc, observed=text.strip()))
    # Old comparisons run unmodified, against adapted probes and real canonical material storage.
    for kind in ['basic_model', 'gamma_selection_core']:
        probe_source = f'tests/probes/native_{kind}_probe.cpp'
        obj = OUT / (kind + '-probe.o')
        run(FLAGS + ['-fPIC', '-c', probe_source, '-o', obj], kind + '-probe-compile')
        library = ROOT / ('build/native-' + kind.replace('_', '-') + '-probe.dylib')
        holdout = oracle(kind, library, kind + '-holdout', holdout=True)
        result[kind + '_holdout'] = {k:v for k,v in holdout.items() if k in (
            'cases','case_count','mismatch_count','value_failures','fp_status_mismatches','fp_flag_partials','case_sha256')}
        source = (ROOT / ('src/engine/basic_model.cpp' if kind == 'basic_model' else 'src/player/gamma_selection_core.cpp')).read_text()
        mutations = MUTANTS if kind == 'basic_model' else [
            ('skip_id2', '    prepare_collision(context, task, primary_work, player);',
             '    prepare_collision(context, task, primary_work, player);\n    if (task.id == 2) return;'),
            ('republish_player', '    bind_animation_table(context, player);',
             '    current(context).player = player_allocation(context, player);\n    bind_animation_table(context, player);'),
            ('wrong_texture_character', 'load_character_textures(context, kGamma);', 'load_character_textures(context, 0);'),
            ('wrong_material_mask', 'kMaterialRetainedFlags = 0x03efffff', 'kMaterialRetainedFlags = 0x07efffff'),
            ('stale_extra_after_display', 'const auto ground_extra = extra_work(context, player);', 'const auto ground_extra = local_extra;'),
            ('omit_join', '    join_vertices(context, task, motion, player);', '    // fault: omitted join'),
            ('spill_shadow_return', 'const auto ground = shadow(context, task, 1.5f);',
             'const auto ground = exact(context, shadow(context, task, 1.5f).rounded());')]
        for name, before, after in mutations:
            name = kind + '-' + name
            core = compile_source('', patch(source, before, after), name)
            mutant = probe(kind, core, obj, name)
            verdict = oracle(kind, mutant, name, expected=1)
            failures = verdict.get('mismatch_count', verdict.get('value_failures'))
            assert failures > 0
            result['mutants'].append(dict(name=name, compiled=True, exit_status=1, value_failures=failures))
    # Compare complete native records, including each FP-status identity, to frozen baseline.
    old = ctypes.CDLL(str(ROOT / 'build/character-materials/baseline/native-gamma-selection-core-probe.dylib'))
    new = ctypes.CDLL(str(ROOT / 'build/native-gamma-selection-core-probe.dylib'))
    U = ctypes.c_uint32
    funcs = [lib.sadx_native_gamma_selection for lib in (old,new)]
    for fn in funcs:
        fn.argtypes = [ctypes.POINTER(U),ctypes.c_size_t,ctypes.c_uint16,ctypes.POINTER(U),ctypes.c_size_t,ctypes.POINTER(U)]
    digest = hashlib.sha256()
    count = 0
    for holdout in (False, True):
        for cw in [0x7f,0x27f,0x37f,0x47f,0x87f,0xc7f]:
            for case in fixtures(holdout):
                rows = []
                for fn in funcs:
                    out, meta = (U*8192)(), (U*4)()
                    rc = fn((U*32)(*case),32,cw,out,8192,meta)
                    rows.append((rc,list(meta),list(out[:meta[0]])))
                assert rows[0] == rows[1], (holdout,cw,count)
                digest.update(json.dumps(rows[0],separators=(',',':')).encode())
                count += 1
    result['Gamma_complete_native_baseline_equality'] = dict(cases=count, sha256=digest.hexdigest(), includes_each_FP_status=True)
    result['basic_complete_receipts_equal'] = (ROOT/'build/character-materials/basic.receipts.jsonl').read_bytes() == (ROOT/'build/character-materials/baseline/basic.receipts.jsonl').read_bytes()
    assert result['basic_complete_receipts_equal']
    oracle('basic_model', ROOT / 'build/character-materials/baseline/native-basic-model-probe.dylib',
           'basic-baseline-holdout', holdout=True)
    result['basic_holdout_complete_receipts_equal'] = (OUT / 'basic_model-holdout.receipts.jsonl').read_bytes() == (OUT / 'basic-baseline-holdout.receipts.jsonl').read_bytes()
    assert result['basic_holdout_complete_receipts_equal']
    (OUT / 'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__ == '__main__':
    main()
