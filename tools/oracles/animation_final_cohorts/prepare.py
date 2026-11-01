"""Materialize immutable Git blobs and build only paired test probes. Never runs cases."""
import hashlib, json, pathlib, shutil, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parents[3]
OUT = ROOT / 'build/animation-final-cohorts'
HERE = pathlib.Path(__file__).resolve().parent
OLD = '53da4de17a1c1f21745d8403cd5bf6068afeb9f5'
CURRENT = '38e1c69ecae029e38bf1d51c404e6a6358a75025'
FOUNDATION = pathlib.Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-7dfc86b6/build/animation-judges')
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    receipts = []
    for label, commit in [('old', OLD), ('current', CURRENT)]:
        tree = OUT / label
        # Snapshot was extracted from git archive, never a Git checkout.
        files = subprocess.check_output(['git','ls-tree','-r','--name-only',commit,'src','tests','tools','third_party','reference/sonic-animation-operations.json'],cwd=ROOT,text=True).splitlines()
        for name in files:
            assert (tree/name).read_bytes() == subprocess.check_output(['git','show',commit+':'+name],cwd=ROOT), name
        dep = tree/'build/animation-judges/softfloat/softfloat.a'
        dep.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(FOUNDATION/'softfloat/softfloat.a',dep)
        # Immutable fixture support shared verbatim by both interfaces.
        unit = (OUT/'current/tests/unit/animation_characterization.cpp').read_text()
        fixture = unit[unit.index('using namespace sadx;'):unit.index('unsigned cases = 0;')]
        (tree/'fixture.hpp').write_text('#include "player/player_animation.hpp"\n#include <algorithm>\n#include <cstring>\n#include <stdexcept>\n#include <string>\n'+fixture+'\n}\n')
        flags = ['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc','-I.','-I'+str(HERE),'-dynamiclib']
        common=['src/player/player_animation.cpp','src/player/player_animation_blend.cpp','src/engine/task_work.cpp','src/cpu/x87_math.cpp','src/cpu/crt_integer.cpp']
        if label=='current': common+=['src/guest/player_animation_reference.cpp']
        groups={'interface':([str(HERE/'interface.cpp')]+common+[str(dep)]), 'table':([str(HERE/'table.cpp'),'src/player/sonic_animation_table.cpp']+(['src/guest/sonic_animation_table_reference.cpp'] if label=='current' else []))}
        for slug,sources in groups.items():
            output=tree/(slug+'.dylib')
            command=flags+sources+['-o',str(output)]
            result=subprocess.run(command,cwd=tree,capture_output=True,text=True)
            (tree/(slug+'.log')).write_text(result.stdout+result.stderr)
            receipt=dict(label=label,commit=commit,slug=slug,command=command,exit=result.returncode,dependency_sha256=sha(dep))
            if result.returncode: receipts.append(receipt); (OUT/'builds.json').write_text(json.dumps(receipts,indent=2)); raise RuntimeError(receipt)
            dependencies=set()
            for source in sources:
                if not source.endswith('.cpp'): continue
                cmd=flags[:-1]+['-MM',source]
                text=subprocess.check_output(cmd,cwd=tree,text=True)
                for path in text.replace('\\\n',' ').split(':',1)[1].split(): dependencies.add(str((tree/path).resolve()))
            receipt.update(library_sha256=sha(output),dependencies={p:sha(pathlib.Path(p)) for p in sorted(dependencies)})
            receipts.append(receipt)
    (OUT/'builds.json').write_text(json.dumps(receipts,indent=2)+'\n')
if __name__=='__main__':main()
