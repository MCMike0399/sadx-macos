"""Build the same bounded consumer fixture against immutable old/current trees."""
from prepare import *
def main():
    receipts=[]
    for label in ('old','current'):
        tree=OUT/label
        dep=tree/"build/animation-judges/softfloat/softfloat.a";dep.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(FOUNDATION/"softfloat/softfloat.a",dep)
        sys.path.insert(0,str(tree/'tools/oracles/animation_judges'))
        # Read exact source list as data from each immutable build helper; import
        # executes definitions only and does not build, mutate or execute a case.
        import importlib.util
        path=tree/'tools/oracles/animation_judges/build.py'
        spec=importlib.util.spec_from_file_location('snapshot_build_'+label,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        bochs=tree/'build/animation-judges/bochs/libbochs_fpatan.a';bochs.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(pathlib.Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-9b97f112/build/animation-judges/bochs/libbochs_fpatan.a'),bochs)
        includes=['-Isrc','-Ithird_party/bochs_fpatan','-Ithird_party/bochs_fpatan/source','-Ithird_party/bochs_fpatan/source/softfloat3e/include']
        sources=[str(HERE/'consumer.cpp')]+module.SONIC
        command=module.FLAGS+includes+['-dynamiclib']+sources+[str(tree/'build/animation-judges/softfloat/softfloat.a'),str(bochs),'-o',str(tree/'consumer.dylib')]
        r=subprocess.run(command,cwd=tree,capture_output=True,text=True);(tree/'consumer.log').write_text(r.stdout+r.stderr)
        receipt=dict(label=label,command=command,exit=r.returncode,bochs_sha256=sha(bochs));receipts.append(receipt)
        (OUT/'consumer-builds.json').write_text(json.dumps(receipts,indent=2)+'\n')
        if r.returncode:raise RuntimeError(receipt)
        dependencies=set()
        for source in sources:
            s=subprocess.check_output(module.FLAGS+includes+['-MM',source],cwd=tree,text=True)
            dependencies.update(str((tree/p).resolve()) for p in s.replace('\\\n',' ').split(':',1)[1].split())
        receipt.update(library_sha256=sha(tree/'consumer.dylib'),dependencies={p:sha(pathlib.Path(p)) for p in sorted(dependencies)})
    (OUT/'consumer-builds.json').write_text(json.dumps(receipts,indent=2)+'\n')
if __name__=='__main__':main()
