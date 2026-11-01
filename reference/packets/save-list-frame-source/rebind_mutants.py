"""Rebind the fourteen immutable foundation faults to the migrated Source API.

No fault intent, cases or expected streams change. Header faults compile a copied
include overlay so both native and reference consumers see the same policy.
"""
import ast
import hashlib
import json
from pathlib import Path
import subprocess

spec = Path('reference/packets/save-io-frame-judges/mutate.py')
tree = ast.parse(spec.read_text())
mutants = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
               and any(isinstance(t, ast.Name) and t.id == 'mutants' for t in n.targets))
root = Path('build/save-source/mutants')
root.mkdir(parents=True, exist_ok=True)
flags = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
         '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-Itests/fixtures', '-Wl,-dead_strip']
io = ['src/game/save_write.cpp', 'src/game/save_file_list.cpp', 'src/game/save_data.cpp', 'src/game/file_select.cpp']
frame = ['src/game/save_frame.cpp', 'src/guest/game_loop.cpp']
rows = []
for name, original_path, *edits in mutants:
    path = 'src/game/save_frame.hpp' if original_path == 'src/game/save_frame.cpp' else original_path
    if name == 'keep-retained-file':
        edits = [x.replace('previous->next->file.reset()', 'previous->next->clear_file()') for x in edits]
    if name == 'frame-eager-phase':
        # auto must capture the value, not the reference-only lazy proxy.
        edits = [x.replace('const auto phase = state.now_saving;', 'const std::int32_t phase = state.now_saving;') for x in edits]
    if name == 'reference-eager-exec':
        edits = ['    bool save_after_completion = false;',
                 '    bool save_after_completion = false;\n    std::optional<std::uint8_t> eager_exec;',
                 '        call(GameLoopOperation::work_420840,0x40cc13,0x420840);',
                 '        eager_exec = st.pending_at_3abdf76;\n        call(GameLoopOperation::work_420840,0x40cc13,0x420840);',
                 'operator T() const { return body.need(owner, read_site); }',
                 'operator T() const { if (read_site == 0x40cc1d) return static_cast<T>(body.need(body.eager_exec, read_site)); return body.need(owner, read_site); }']
    source = Path(path).read_text()
    for old, new in zip(edits[::2], edits[1::2]):
        assert old in source, (name, old)
        source = source.replace(old, new, 1)
    directory = root / name
    directory.mkdir(exist_ok=True)
    copied = directory / ('game/save_frame.hpp' if path.endswith('.hpp') else 'fault.cpp')
    copied.parent.mkdir(parents=True, exist_ok=True)
    copied.write_text(source)
    sources = frame if path.endswith('save_frame.hpp') or path in frame else io
    test = 'tests/unit/save_frame_characterization.cpp' if sources is frame else 'tests/unit/save_io_characterization.cpp'
    exe = directory / 'test'
    command = flags[:1] + flags[1:]
    if path.endswith('.hpp'):
        command.insert(command.index('-Isrc'), '-I' + str(directory))
    command += [test] + [str(copied) if p == path else p for p in sources] + ['-o', str(exe)]
    built = subprocess.run(command, capture_output=True, text=True)
    (directory / 'compile.log').write_text(built.stdout + built.stderr)
    row = dict(name=name, original_path=original_path, rebound_path=path,
               original_spec_sha256=hashlib.sha256(spec.read_bytes()).hexdigest(),
               source_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest(),
               fault_sha256=hashlib.sha256(copied.read_bytes()).hexdigest(),
               compile_command=command, compile_exit=built.returncode, run_exit=None, detected=False)
    if built.returncode == 0:
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        (directory / 'run.log').write_text(result.stdout + result.stderr)
        row.update(run_exit=result.returncode, detected=result.returncode != 0)
    rows.append(row)
    (root / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
    print(name, row['compile_exit'], row['run_exit'], flush=True)
assert len(rows) == 14 and all(r['compile_exit'] == 0 and r['detected'] for r in rows)
