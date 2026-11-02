"""Run this packet's finite commands; argv is always field two of commands.tsv."""
import argparse
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/gamma-fan-native'
FLAGS = ['-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math',
         '-arch','arm64','-mmacosx-version-min=14.0','-Ibuild/gamma-fan-native/include','-Isrc','-Itests/fixtures']
FRAMEWORKS = ['Foundation','AppKit','Metal','MetalKit','QuartzCore','CoreGraphics','ImageIO',
              'GameController','AudioToolbox','AVFoundation','CoreMedia','CoreVideo']
PLATFORMS = ['build/obj/platform/mac/'+name+'.o' for name in ['mac_clock','mac_audio','hid_switch_controller',
             'mac_movie','metal_device_frame','mac_audio_playback','mac_sound_bank','mac_sound_effects',
             'mac_save_files','mac_save_writer']]

def run(label, argv, expected=(0,)):
    OUT.mkdir(parents=True,exist_ok=True)
    tsv = OUT / 'commands.tsv'
    with tsv.open('a') as f: f.write(label+'\t'+json.dumps(argv)+'\n')
    # Execute the serialized argv, so the retained command is the actual input.
    saved_label, saved = tsv.read_text().splitlines()[-1].split('\t')
    assert saved_label == label
    result=subprocess.run(json.loads(saved),cwd=ROOT,capture_output=True,text=True)
    (OUT/(label+'.log')).write_text(result.stdout+result.stderr)
    (OUT/(label+'.exit')).write_text(str(result.returncode)+'\n')
    if result.returncode not in expected:
        raise RuntimeError(f'{label}: {result.returncode}: {result.stdout[-1500:]} {result.stderr[-1500:]}')
    return result.returncode

def unit_command(source, binary, extra=()):
    return ['xcrun','clang++',*FLAGS,'-fobjc-arc',source,*extra,*PLATFORMS,
            'build/libsadx-native.a',*[x for f in FRAMEWORKS for x in ['-framework',f]],'-o',binary]

def main():
    p=argparse.ArgumentParser();p.add_argument('stage',choices=['caller','holdout','faults','normal'])
    args=p.parse_args()
    if args.stage=='caller':
        run('build-caller',unit_command('tests/unit/native_gamma_fan.mm','build/native-gamma-fan-test'))
        run('caller',['build/native-gamma-fan-test','game/imported'])
    elif args.stage=='holdout':
        run('holdout',['reference/python/bin/python','tools/oracles/verify_original_gamma_fan.py',
            '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe','build/native-gamma-fan-probe.dylib',
            '--cases','4096','--seed','0x9e492c10','--output','build/oracles/gamma_fan_holdout.json'],(0,2))
    elif args.stage=='normal':
        argv=json.loads((OUT/'commands.tsv').read_text().splitlines()[0].split('\t')[1])
        argv=[s.replace('baseline-save','final-save').replace('baseline.ppm','final.ppm') for s in argv]
        run('final-normal-input',argv,(1,))
        log=(OUT/'final-normal-input.log').read_text()
        assert 'native session stopped: native_gamma_display' in log
        states=[json.loads(line) for line in log.splitlines() if line.startswith('{') and '"execution":"native"' in line]
        assert len(states)==1 and states[0]['frames']==1332 and states[0]['completed_frames']==1330
        assert (OUT/'final.ppm').read_bytes()==(OUT/'baseline.ppm').read_bytes()
        (OUT/'final-state.json').write_text(json.dumps(states[0],indent=2)+'\n')
    else:
        source=(ROOT/'src/player/gamma_fan.cpp').read_text()
        faults=[('ignore-lateral','access.lateral_speed()','0.0f'),
                ('wrong-lower-clamp','speed - 1.0f','speed - 0.5f'),
                ('wrong-rate','speed * 12.5f','speed * 10.0f'),
                ('wrong-phase-direction','2048U - static_cast','2048U + static_cast'),
                ('stale-forward-reload','fidelity::ExactFloat(arithmetic, access.forward_speed()) * 10.0f','speed * 10.0f'),
                ('wrong-tilt-limit','-4551, 4551','-4000, 4000'),
                ('phase-halfword','access.store_phase(phase);','access.store_phase(phase & 65535U);')]
        results=[]
        for label,old,new in faults:
            assert source.count(old)==1,(label,source.count(old))
            path=OUT/(label+'.cpp');path.write_text(source.replace(old,new))
            dylib=str((OUT/(label+'.dylib')).relative_to(ROOT))
            run('build-fault-'+label,['xcrun','clang++',*FLAGS,'-dynamiclib','tests/probes/native_gamma_fan_probe.cpp',
                                    str(path.relative_to(ROOT)),'build/libsadx-native.a','-o',dylib])
            report='build/oracles/gamma_fan_fault_'+label+'.json'
            rc=run('fault-'+label,['reference/python/bin/python','tools/oracles/verify_original_gamma_fan.py',
                '/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe',dylib,'--output',report],(1,))
            r=json.loads((ROOT/report).read_text());assert r['value_failures']>0
            results.append({'fault':label,'value_failures':r['value_failures'],'exit_status':rc})
        (ROOT/'reference/packets/gamma-fan-native/FAULTS.json').write_text(json.dumps(results,indent=2)+'\n')
if __name__=='__main__':main()
