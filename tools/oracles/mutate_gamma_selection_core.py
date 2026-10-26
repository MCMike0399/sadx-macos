"""Seven value/ordering mutants of the bounded Gamma core; never edits a judge."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
MUTANTS={
    'skip_id2':('    if (static_cast<Mode>(task.mode) == Mode::Initialize)\n',
                '    if (task.id == 2) return;\n    if (static_cast<Mode>(task.mode) == Mode::Initialize)\n'),
    'republish_player':('    bind_animation_table(context, player);',
                       '    current(context).player = player_allocation(context, player);\n    bind_animation_table(context, player);'),
    'wrong_texture_character':('    load_character_textures(context, kGamma);',
                               '    load_character_textures(context, 3);'),
    'wrong_material_mask':('kMaterialRetainedFlags = 0x03efffff',
                           'kMaterialRetainedFlags = 0x07efffff'),
    'stale_extra_after_display':('update_target_effects(context, task, player, extra_work(context, player));',
                                'update_target_effects(context, task, player, local_extra);'),
    'omit_join':('    join_vertices(context, task, motion, player);',
                 '    // Deliberate mutation: omit mandatory join.'),
    'spill_shadow_return':('    const auto ground = shadow(context, task, 1.5f);',
                          '    const auto ground = exact(context, shadow(context, task, 1.5f).rounded());'),
}
def main():
    ap=argparse.ArgumentParser();ap.add_argument('exe',type=Path)
    ap.add_argument('--output-dir',type=Path,required=True);a=ap.parse_args()
    a.output_dir.mkdir(parents=True,exist_ok=True)
    source=(ROOT/'src/player/gamma_selection_core.cpp').read_text()
    def run(item):
        name,(old,new)=item
        if source.count(old)!=1:raise ValueError(f'ambiguous mutant {name}')
        path=a.output_dir/f'{name}.cpp';path.write_text(source.replace(old,new))
        lib=a.output_dir/f'{name}.dylib';report=a.output_dir/f'{name}.json'
        command=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
                 '-ffp-contract=off','-fno-fast-math','-dynamiclib','-Isrc',
                 'tests/probes/native_gamma_selection_core_probe.cpp',str(path),
                 'build/libsadx-core.a','-o',str(lib)]
        built=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        (a.output_dir/f'{name}.build.log').write_text(built.stdout+built.stderr)
        if built.returncode:return {'name':name,'build_exit_status':built.returncode,'caught':False}
        tested=subprocess.run([sys.executable,str(ROOT/'tools/oracles/verify_original_gamma_selection_core.py'),
                               str(a.exe),str(lib),'--output',str(report)],cwd=ROOT,capture_output=True,text=True)
        data=json.loads(report.read_text())
        return {'name':name,'build_exit_status':0,'judge_exit_status':tested.returncode,
                'value_failures':data['value_failures'],'caught':data['value_failures']>0,
                'library_sha256':hashlib.sha256(lib.read_bytes()).hexdigest()}
    with ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(run,MUTANTS.items()))
    result={'mutants':results,'caught':sum(r['caught'] for r in results),'total':len(results),
            'source_sha256':hashlib.sha256(source.encode()).hexdigest()}
    (a.output_dir/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result));return 0 if all(r['caught'] for r in results) else 1
if __name__=='__main__':sys.exit(main())
