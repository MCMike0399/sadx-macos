"""Build-only application of the exact Root callback hunk; no source owner is edited."""
from pathlib import Path
import difflib

ROOT = Path(__file__).resolve().parents[3]
source = ROOT / 'src/runtime/native_selection_preview.cpp'
before = source.read_text()
old = '    animation::Runtime animation{scheduler.work_arena().get(), assets->resources()};\n'
new = ('    context.model = assets->model();\n'
       '    animation::Runtime animation{scheduler.work_arena().get(), assets->resources(),\n'
       '                                 &scene.session_.character_scene_.objects};\n')
assert before.count(old) == 1 or before.count(new) == 1
after = before.replace(old, new)
output = ROOT / 'build/sonic-model-selection-preview.cpp'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(after)
patch = ''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                  fromfile='a/src/runtime/native_selection_preview.cpp',
                                  tofile='b/src/runtime/native_selection_preview.cpp'))
# The exact recipe is emitted under build; the source packet separately retains its hunk.
(ROOT / 'build/sonic-model-root-wiring.patch').write_text(patch)
