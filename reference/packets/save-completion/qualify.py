"""Observe unchanged dedicated judges without changing their inputs or comparisons."""
import hashlib
import json
from pathlib import Path
import runpy
import sys

script, library, output = sys.argv[1:]
output = Path(output)
exe = '/Users/burbujamc/Developer/sadx-macos/reference/updated-pc-reference/game/sonic.exe'
rows = []
partials = []
source = Path(script).read_text().splitlines()
observation = next(n for n, line in enumerate(source, 1)
                   if line.strip().startswith(('if observed!=expected:', 'if diff:')))
def trace(frame, event, arg):
    if frame.f_code.co_filename == script and event == 'line' and frame.f_lineno == observation:
        v = frame.f_locals
        expected = list(v['expected'])
        observed = list(v['out'][:len(expected)])
        case = v.get('case', v.get('n'))
        row = {'case': case, 'input': v['v'], 'expected': expected, 'observed': observed}
        rows.append(row)
        diff = [[i, x, y] for i, (x, y) in enumerate(zip(expected, observed)) if x != y]
        if diff:
            partials.append({'case': case, 'input': v['v'], 'diff': diff})
    return trace
sys.path[:0] = ['tools/oracles', 'tools/lib']
sys.argv = [script, exe, library, '--output', str(output)]
sys.settrace(trace)
try:
    runpy.run_path(script, run_name='__main__')
except SystemExit as error:
    status = int(error.code or 0)
finally:
    sys.settrace(None)
raw = output.with_suffix('.outputs.json')
raw.write_text(json.dumps(rows, separators=(',', ':')) + '\n')
summary = {'terminal_status': status, 'output_cases': len(rows),
           'full_outputs_sha256': hashlib.sha256(raw.read_bytes()).hexdigest(),
           'partial_or_mismatch_identities': partials,
           'judge_sha256': hashlib.sha256(Path(script).read_bytes()).hexdigest(),
           'probe_library_sha256': hashlib.sha256(Path(library).read_bytes()).hexdigest()}
output.with_suffix('.qualification.json').write_text(json.dumps(summary, indent=2) + '\n')
raise SystemExit(status)
