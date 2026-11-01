#!/usr/bin/env python3
"""Discriminate actual TaskBridge faults with current maintained dependency closure."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

BASE = '99bdaf4a9e06006aed7f9fcf2c963a6e68cf61d3'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--unicorn', type=Path, required=True)
    args = p.parse_args()
    out = Path('build/task-judges/bridge-faults')
    out.mkdir(parents=True, exist_ok=True)
    source = subprocess.check_output(['git', 'show', BASE + ':src/hle/hle_task_bridge.cpp']).decode()
    assert Path('src/hle/hle_task_bridge.cpp').read_text() == source
    faults = [
        ('eager-ready-before-shortcut', 'if(!ready())return 0;\n        import_graph();TaskResult result;',
         'if(!ready())return 0;\n        import_graph();TaskResult result;', 'tests/unit/task_bridge_contract.cpp'),
        ('missing-arena-is-success', 'if(!current){g.stop("task bridge: missing arena global at 40b1a5");return;}',
         'if(!current){g.set_eax(0);return;}', 'tests/unit/task_bridge_contract.cpp'),
        ('stale-graph-after-callback', 'if(!ready())return;import_graph();}\n            for(;;)',
         'if(!ready())return;}\n            for(;;)', 'tests/unit/guest_task_graph_sync.cpp'),
    ]
    rows = []
    for name, old, new, test in faults:
        if name == 'eager-ready-before-shortcut':
            old = 'entry=at;\n        try{\n        const auto level='
            new = 'entry=at;\n        try{\n        if(!ready())return 0;\n        const auto level='
        assert source.count(old) == 1
        copied = out / (name + '.cpp')
        copied.write_text(source.replace(old, new, 1))
        binary = out / name
        command = ['xcrun', 'clang++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                   '-ffp-contract=off', '-fno-fast-math', '-Isrc', '-I' + str(args.unicorn / 'include'),
                   test, str(copied), 'build/libsadx-guest.a', str(args.unicorn / 'lib/libunicorn.a'),
                   '-lpthread', 'build/libsadx-core.a', '-o', str(binary)]
        compiled = subprocess.run(command, capture_output=True, text=True)
        (out / (name + '.compile.log')).write_text(compiled.stdout + compiled.stderr)
        row = dict(name=name, old=old, new=new, command=command, compile_exit=compiled.returncode,
                   copied_sha256=hashlib.sha256(copied.read_bytes()).hexdigest(), caught=False)
        if compiled.returncode == 0:
            run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=60)
            (out / (name + '.run.log')).write_text(run.stdout + run.stderr)
            row.update(run_exit=run.returncode, caught=run.returncode != 0,
                       stdout=run.stdout, stderr=run.stderr)
        rows.append(row)
        (out / 'receipts.json').write_text(json.dumps(rows, indent=2) + '\n')
        print(name, row['compile_exit'], row['caught'], flush=True)
    assert Path('src/hle/hle_task_bridge.cpp').read_text() == source
    return int(not all(row['compile_exit'] == 0 and row['caught'] for row in rows))

if __name__ == '__main__':
    raise SystemExit(main())
