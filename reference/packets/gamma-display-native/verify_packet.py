"""Finite packet commands. Run heavy stages only after Root releases its hold."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/gamma-display-native'
FLAGS = ['-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math',
         '-arch','arm64','-mmacosx-version-min=14.0','-Ibuild/gamma-display-native/include','-Isrc','-Itests/fixtures']
FRAMEWORKS = ['Foundation','AppKit','Metal','MetalKit','QuartzCore','CoreGraphics','ImageIO',
              'GameController','AudioToolbox','AVFoundation','CoreMedia','CoreVideo']
PLATFORMS = ['build/obj/platform/mac/'+name+'.o' for name in ['mac_clock','mac_audio','hid_switch_controller',
             'mac_movie','metal_device_frame','mac_audio_playback','mac_sound_bank','mac_sound_effects',
             'mac_save_files','mac_save_writer']]
EXE = '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe'


def run(label, argv, expected=(0,)):
    OUT.mkdir(parents=True,exist_ok=True)
    with (OUT/'commands.tsv').open('a') as f:f.write(label+'\t'+json.dumps(argv)+'\n')
    result=subprocess.run(argv,cwd=ROOT,capture_output=True,text=True)
    (OUT/(label+'.log')).write_text(result.stdout+result.stderr)
    (OUT/(label+'.exit')).write_text(str(result.returncode)+'\n')
    if result.returncode not in expected:
        raise RuntimeError(f'{label}: {result.returncode}: {(result.stdout+result.stderr)[-2000:]}')
    return result


def build():
    # Shared files stay untouched; the explicit override compiles the exact Root
    # callback fixture instead of its old shipping stub.
    makefile=OUT/'overlay.mk'
    makefile.write_text('build/obj/runtime/native_selection_preview.o: '
        'build/gamma-display-native/root-fixture/src/runtime/native_selection_preview.cpp\n'
        '\t$(CXX) $(CXXFLAGS) -MMD -MP $(INCLUDES) -c $< -o $@\n')
    sources=sorted(set((OUT/'root-fixture/native-sources.txt').read_text().split()+
        [str(p.relative_to(ROOT)) for p in (ROOT/'src/runtime').glob('native_*.cpp')]+
        [str(p.relative_to(ROOT)) for d in ['engine','runtime'] for p in (ROOT/'src'/d).glob('*_native.cpp')]))
    run('build-app',['make','-j2','-o','build/native-controller-deps/include/SDL3/SDL.h',
        '-f','Makefile','-f',str(makefile.relative_to(ROOT)),
        'INCLUDES=-Ibuild/gamma-display-native/include -Isrc','NATIVE_SOURCES='+' '.join(sources),
        'build/sadx-native','build/sadx-native-closure'])
    for slug,source,probe in [('gamma-display','src/player/gamma_display.cpp','tests/probes/native_gamma_display_probe.cpp'),
                              ('render-state-snapshot','src/engine/render_state_snapshot.cpp','tests/probes/native_render_state_snapshot_probe.cpp')]:
        run('build-probe-'+slug,['xcrun','clang++',*FLAGS,'-dynamiclib',probe,source,
            'build/libsadx-native.a','-o','build/native-'+slug+'-probe.dylib'])
    run('build-caller',['xcrun','clang++',*FLAGS,'-fobjc-arc','tests/unit/native_gamma_display.mm',
        *PLATFORMS,'build/libsadx-native.a',*[x for f in FRAMEWORKS for x in ['-framework',f]],
        '-o','build/native-gamma-display-test'])


def judge(holdout=False):
    for slug in ['gamma_display','render_state_snapshot']:
        extra=['--cases','4096','--seed','0x9e47fd50'] if holdout else []
        suffix='_holdout' if holdout else ''
        run(slug+suffix,['reference/python/bin/python','tools/oracles/verify_original_'+slug+'.py',EXE,
            'build/native-'+slug.replace('_','-')+'-probe.dylib',*extra,
            '--output','build/oracles/'+slug+suffix+'.json'])


def asset_overlay():
    assets=OUT/'assets';original=ROOT/'game/imported'
    for path in original.iterdir():
        destination=assets/path.name
        if path.name=='characters':
            destination.mkdir(exist_ok=True)
            for child in path.iterdir():
                target=destination/child.name
                if not target.exists():target.symlink_to(child.resolve())
        elif not destination.exists():destination.symlink_to(path.resolve(),target_is_directory=path.is_dir())
    return assets


def normal():
    assets=asset_overlay()
    argv=json.loads((OUT/'baseline-command.json').read_text())['argv']
    argv=[x.replace('baseline-save','display-save').replace('baseline.ppm','display.ppm') for x in argv]
    argv[argv.index('--assets')+1]=str(assets.relative_to(ROOT))
    run('normal-input',argv,(1,))


def main():
    p=argparse.ArgumentParser();p.add_argument('stage',choices=['build','development','holdout','caller','normal'])
    args=p.parse_args()
    if args.stage=='build':build()
    elif args.stage=='development':judge()
    elif args.stage=='holdout':judge(True)
    elif args.stage=='caller':run('caller',['build/native-gamma-display-test',str(asset_overlay().relative_to(ROOT))])
    else:normal()
if __name__=='__main__':main()
