"""Private source closure and test-only store-prefix instrumentation.

No runtime observer or test hook enters product source. Each snapshot is inserted
immediately after its corresponding assignment in an otherwise unchanged copy.
The original uninstrumented probe is checked separately by the same oracle.
"""
import argparse
import re
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source', default=str(ROOT / 'src/engine/sprite_registry.cpp'))
parser.add_argument('--output', default=str(ROOT / 'build/native-sprite-registry-prefix-probe.dylib'))
args = parser.parse_args()
source = Path(args.source).read_text()
header = '''
namespace sadx::sprite_registry {
void record_prefix(unsigned, const State &, const NodeHandle &);
}
'''
source = source.replace('#include "engine/sprite_registry.hpp"',
                        '#include "engine/sprite_registry.hpp"\n' + header)
for target, phase in [('node->next', 0), ('state.head', 1), ('state.count', 2),
                      ('node->definition', 4)]:
    pattern = r'    ' + re.escape(target) + r' = [^;]+;'
    assert len(re.findall(pattern, source)) == 1
    def observe(match):
        before = '    record_prefix(3, state, node);\n' if phase == 4 else ''
        return before + match[0] + f'\n    record_prefix({phase}, state, node);'
    source = re.sub(pattern, observe, source)
path = ROOT / 'build/sprite_registry_prefix.cpp'
path.write_text(source)
subprocess.run(['c++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wmissing-field-initializers',
                '-shared', '-fPIC', '-DSADX_SPRITE_TRACE', '-Isrc',
                'tests/probes/native_sprite_registry_probe.cpp', str(path),
                'src/engine/task_work.cpp', '-o', args.output],
               cwd=ROOT, check=True)
