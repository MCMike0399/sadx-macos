"""Compile isolated model suppression; the real constructor must reject zero submitted vertices."""
from pathlib import Path
import subprocess,json,shlex
source=Path('src/runtime/model_draw.cpp').read_text()
old='''void draw_model(Context &context, ModelId id) {
    basic_model::Context model{context.display, context.environment};
    basic_model::draw_model(id, context.environment.pack, context.environment.scene.materials,
                            model.owners(), model);
}'''
assert source.count(old)==1
mutant=Path('build/draw-path-2/caller-suppressed-model.cpp')
mutant.write_text(source.replace(old,'''void draw_model(Context &context, ModelId id) {
    (void)context;
    (void)id;
}'''))
planned=subprocess.run(['make','-n','-j2','-o','build/native-controller-deps/include/SDL3/SDL.h',
                        '-W','tests/unit/native_gamma_display.mm','build/native-gamma-display-test'],
                       capture_output=True,text=True,check=True)
line=next(line for line in reversed(planned.stdout.splitlines())
          if line.startswith('xcrun clang++ ') and 'tests/unit/native_gamma_display.mm' in line)
argv=shlex.split(line);argv.insert(argv.index('tests/unit/native_gamma_display.mm')+1,str(mutant));argv[-1]='build/draw-path-2/native-gamma-display-suppressed-test'
p=subprocess.run(argv,capture_output=True,text=True);Path('build/draw-path-2/caller-suppressed-build.log').write_text(p.stdout+p.stderr)
assert p.returncode==0,p.stderr
run=[argv[-1],'build/gamma-display-native/assets'];q=subprocess.run(run,capture_output=True,text=True)
Path('build/draw-path-2/caller-suppressed.log').write_text(q.stdout+q.stderr)
row={'compile_command':argv,'compile_exit':p.returncode,'command':run,'exit':q.returncode,'stdout':q.stdout,'stderr':q.stderr,'caught':q.returncode!=0 and 'body_draws > 0 && body_vertices > 0' in q.stderr}
Path('build/draw-path-2/caller-suppressed.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row,indent=2));assert row['caught']
