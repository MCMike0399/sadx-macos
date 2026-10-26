"""Link/run reference initialization with Root's actual native Context/resource/device code.

Root source and archives are read-only. Its existing model_device unit is copied privately;
renaming main requires making C++ main's implicit return 0 explicit. Assertions and all
other body bytes remain unchanged. No native startup is faked: unbound test-only startup
hooks throw, while the reference initializer and real native device unit both execute.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--native-root',type=Path,required=True)
    p.add_argument('--system',type=Path,required=True)
    p.add_argument('--bound-native-startup', action='store_true',
                   help='use actual native startup hooks already defined in the native archive')
    a = p.parse_args()
    b = ROOT/'build/polybuff-context-coexistence'
    b.mkdir(parents=True,exist_ok=True)
    archive = a.native_root/'build/libsadx-core.a'
    archive_hash = hashlib.sha256(archive.read_bytes()).hexdigest()
    native_source = a.native_root/'tests/unit/model_device.cpp'
    source = native_source.read_text()
    assert source.count('int main(') == 1
    end = source.rfind('}')
    private_source = b/'native_device_unit.cpp'
    private_source.write_text(source[:end].replace('int main(', 'int polybuff_native_device_unit(',1)+
                              '    return 0;\n'+source[end:])
    overlay = b/'include/engine'
    overlay.mkdir(parents=True,exist_ok=True)
    overlay_header = overlay/'polybuff_initialization.hpp'
    overlay_header.write_bytes((ROOT/'src/engine/polybuff_initialization.hpp').read_bytes())
    for name in ['normal_uv_strip.hpp','normal_uv_strip.cpp']:
        assert (a.native_root/'src/engine'/name).read_bytes() == (ROOT/'src/engine'/name).read_bytes()
    flags = ['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror',
             '-ffp-contract=off','-fno-fast-math']
    def run(cmd,label):
        r = subprocess.run([str(x) for x in cmd],cwd=ROOT,text=True,
                           stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (b/(label+'.log')).write_text(r.stdout)
        if r.returncode: raise RuntimeError(label+': '+r.stdout[-2500:])
        return r
    objects = []
    for name in ['src/guest/polybuff_owners.cpp','src/guest/polybuff_initialization_reference.cpp',
                 'src/engine/polybuff_initialization.cpp','src/engine/polybuff_initialization_binding.cpp',
                 'tests/probes/polybuff_coexistence_main.cpp'] + (
                     [] if a.bound_native_startup else ['tests/probes/polybuff_coexistence_hooks.cpp']):
        obj = b/(Path(name).stem+'.o')
        run(flags+['-Isrc','-c',name,'-o',obj],Path(name).stem+'-compile')
        objects.append(obj)
    for name in [private_source,ROOT/'tests/probes/polybuff_native_context_link_probe.cpp']:
        obj = b/(name.stem+'.o')
        run(flags+['-I'+str(b/'include'),'-I'+str(a.native_root/'src'),'-Isrc','-c',name,'-o',obj],
            name.stem+'-compile')
        objects.append(obj)
    binary = b/'coexistence'
    run(flags+objects+[archive,'-o',binary],'link')
    run([binary,a.system],'unit')
    raw = run(['xcrun','nm','-gU',b/'polybuff_initialization_reference.o'],'reference-symbols').stdout
    symbols = subprocess.check_output(['xcrun','c++filt'],input=raw,text=True)
    for name in ['configured_count','buffer_description','allocate_vertex_scratch',
                 'observe_initialization_store','initialize_record']:
        assert 'sadx::polybuff::'+name+'(' not in symbols,name
        assert 'sadx::polybuff_reference::'+name+'(' in symbols,name
    assert archive_hash == hashlib.sha256(archive.read_bytes()).hexdigest()
    result = {'passed':True,'exit_status':0,'native_context':'Root actual Context { Display& }',
              'reference_context':'sadx::polybuff_reference::Context',
              'native_startup':('Actual Display startup executes through the native device unit.'
                                if a.bound_native_startup else
                                'Unbound hooks throw; native full startup not executed or claimed.'),
              'output':(b/'unit.log').read_text().strip(),
              'native_archive':str(archive),'native_archive_sha256':archive_hash,
              'native_unit_source_sha256':hashlib.sha256(native_source.read_bytes()).hexdigest(),
              'native_context_header_sha256':hashlib.sha256((a.native_root/'src/runtime/model_device.hpp').read_bytes()).hexdigest(),
              'reference_defined_symbols':symbols,'undefined_dynamic_lookup_used':False,
              'limits':['Actual Root unit exercises its real resource types/device functions in the same executable.',
                        ('Native Display constructor executes its actual configured startup; no GPU proof.'
                         if a.bound_native_startup else
                         'Native initializer specialization is linked, not executed: Root still binds actual counts/storage.')]}
    (b/'proof.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result['output'])
if __name__ == '__main__': main()
