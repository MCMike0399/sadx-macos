"""Build-only exact callback hunk; shared Root source remains untouched."""
from pathlib import Path
import difflib
ROOT = Path(__file__).resolve().parents[3]
source = ROOT / 'src/runtime/native_selection_preview.cpp'
before = source.read_text()
needle = '    context.model = assets->model();\n'
extra = """    context.act = &scene.session_.act_;
    context.scene = &scene.session_.character_scene_;
    CharacterTextureViews texture_views(scene.session_.character_resources_.assets(),
                                        scene.session_.character_resources_.lists());
    auto texture_services = scene.session_.regular_texture_services();
    context.character_textures = texture_views.catalog(scene.session_.character_resources_.loaded());
    context.texture_state = scene.session_.regular_resources_.state();
    context.texture_services = &texture_services;
"""
assert before.count(needle) == 1
assert before.count(extra) in (0, 1)
after = before if extra in before else before.replace(needle, needle + extra)
out = ROOT / 'build/sonic-initialization-selection-preview.cpp'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(after)
(ROOT / 'build/sonic-initialization-root-wiring.patch').write_text(''.join(
    difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                        fromfile='a/src/runtime/native_selection_preview.cpp',
                        tofile='b/src/runtime/native_selection_preview.cpp')))
