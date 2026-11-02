"""Check exact Root hunks against the header/source list used by this private build."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from verify_packet import ROOT, OUT, run

def main():
    patch=ROOT/'reference/packets/gamma-fan-native/ROOT-WIRING.patch'
    shared=['src/runtime/native_display.hpp','native-sources.txt']
    fixture=OUT/'root-fixture'
    hashes={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in shared}
    for name in shared:
        destination=fixture/name
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(ROOT/name,destination)
    run('apply-root-fixture',['patch','--batch','-p1','-d',str(fixture),'-i',str(patch)])
    assert (fixture/'src/runtime/native_display.hpp').read_bytes()==(OUT/'include/runtime/native_display.hpp').read_bytes()
    label,serialized=next(line.split('\t') for line in (OUT/'commands.tsv').read_text().splitlines() if line.startswith('final-build-bound-app\t'))
    argv=json.loads(serialized)
    compiled_sources=set(next(a for a in argv if a.startswith('NATIVE_SOURCES=')).split('=',1)[1].split())
    fixture_sources=set((fixture/'native-sources.txt').read_text().splitlines())
    fixture_sources.update(str(p.relative_to(ROOT)) for pattern in ['src/engine/*_native.cpp','src/runtime/native_*.cpp'] for p in ROOT.glob(pattern))
    fixture_sources={p for p in fixture_sources if p and not p.startswith('#')}
    assert compiled_sources==fixture_sources
    assert hashes=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in shared}
    report={'exact_root_header_compiled':True,'exact_root_source_membership':True,
            'compiled_source_count':len(compiled_sources),'shared_files_unchanged':hashes,
            'patch_sha256':hashlib.sha256(patch.read_bytes()).hexdigest()}
    (ROOT/'reference/packets/gamma-fan-native/ROOT-FIXTURE.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
