from prepare import *
def main():
    receipts=[]
    for label in ('old','current'):
        tree=OUT/label
        dep=tree/"build/animation-judges/softfloat/softfloat.a";dep.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(FOUNDATION/"softfloat/softfloat.a",dep)
        sources=[str(HERE/'gamma.mm'),'src/engine/character_pack.cpp','src/player/player_startup.cpp','src/player/player_state.cpp','src/player/player_task.cpp','src/guest/player_initialization_reference.cpp','src/engine/task.cpp','src/engine/task_work.cpp','src/cpu/x87_math.cpp']
        flags=['xcrun','clang++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-ffp-contract=off','-fno-fast-math','-arch','arm64','-mmacosx-version-min=14.0','-Isrc','-fobjc-arc']+(['-DCURRENT_CONTRACT'] if label=='current' else [])
        cmd=flags+['-dynamiclib']+sources+[str(tree/'build/animation-judges/softfloat/softfloat.a'),'-framework','Foundation','-o',str(tree/'gamma.dylib')]
        r=subprocess.run(cmd,cwd=tree,capture_output=True,text=True);(tree/'gamma.log').write_text(r.stdout+r.stderr);rec=dict(label=label,command=cmd,exit=r.returncode);receipts.append(rec)
        (OUT/'gamma-builds.json').write_text(json.dumps(receipts,indent=2)+'\n')
        if r.returncode:raise RuntimeError(rec)
        deps=set()
        for source in sources:
            s=subprocess.check_output(flags+['-MM',source],cwd=tree,text=True);deps.update(str((tree/p).resolve()) for p in s.replace('\\\n',' ').split(':',1)[1].split())
        rec.update(library_sha256=sha(tree/'gamma.dylib'),dependencies={p:sha(pathlib.Path(p)) for p in sorted(deps)})
    (OUT/'gamma-builds.json').write_text(json.dumps(receipts,indent=2)+'\n')
if __name__=='__main__':main()
