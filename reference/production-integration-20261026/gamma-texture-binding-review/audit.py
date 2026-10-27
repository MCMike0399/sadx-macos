"""Read retained files and Git objects only; never execute candidate programs."""
import hashlib
import json
import subprocess
from pathlib import Path

C = Path('/Users/burbujamc/.t3/worktrees/sadx-macos/t3-b75da23e')
P = C / 'reference/production-integration-20261026/gamma-texture-binding'
Q = C / 'build/gamma-texture-qualification'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def git(*args):
    return subprocess.check_output(['git', '-C', str(C), *args])

def obj(ref, path):
    return git('show', ref + ':' + path)

proof = json.loads((P / 'proof.json').read_text())
result = {'candidate': git('rev-parse', 'd7e912a').decode().strip(),
          'base': git('rev-parse', '6e9a1ec').decode().strip(),
          'candidate_worktree_status': git('status', '--short').decode(),
          'tests': 'NOT RERUN', 'checks': []}

def record(label, ok, **details):
    result['checks'].append(dict(check=label, matches=bool(ok), **details))

for group in ['source_sha256', 'approved_8052fac_imports_byte_identical',
              'gamma_algorithm_harness_unchanged', 'downstream_regular_sources_harness_unchanged']:
    for path, expected in proof[group].items():
        data = (C / path).read_bytes()
        record(group + ':' + path, sha(data) == expected, sha256=sha(data))
        if group == 'approved_8052fac_imports_byte_identical':
            record('approved Git object:' + path, data == obj('8052fac', path))
        if group.endswith('harness_unchanged'):
            record('base Git object:' + path, data == obj('6e9a1ec', path))

for path in ['reference/judge-manifest.json', 'reference/readability-baseline.json',
             'src/cpu/x87.hpp', 'Makefile', 'tests/unit/manifest.tsv']:
    # CPU Context alias came from the prior reviewed collision packet, not this diff.
    if (C / path).exists():
        record('unchanged baseline:' + path, obj('6e9a1ec', path) == obj('d7e912a', path))

for name, entry in proof['wrapper_original'].items():
    path = Q / (name + '.json')
    data = json.loads(path.read_text())
    record('raw report:' + name, sha(path.read_bytes()) == entry['report_sha256'],
           cases=data['cases'], mismatches=data['value_mismatches'], strict=data['strict'])
for name, entry in proof['gamma_original'].items():
    path = Q / (name + '.json')
    data = json.loads(path.read_text())
    record('raw report:' + name, sha(path.read_bytes()) == entry['report_sha256'],
           cases=data['case_count'], mismatches=data['value_failures'], partials=data['fp_flag_partials'])
for name, key in [('startup', 'startup_original'), ('regular', 'downstream_regular_original')]:
    path = Q / (name + '.json')
    record('raw report:' + name, sha(path.read_bytes()) == proof[key]['report_sha256'])

for item in proof['composition_mutations']['mutants']:
    folder = Q / 'composition-mutants' / item['name']
    source = folder / ('binding.cpp' if (folder / 'binding.cpp').exists()
                       else 'runtime/character_texture_assets.hpp')
    log = (folder / 'test.log').read_text().strip()
    record('composition mutant:' + item['name'],
           sha(source.read_bytes()) == item['mutated_source_sha256'] and log == item['failure'],
           recorded_build_exit=item['build_exit'], recorded_test_exit=item['test_exit'],
           failure=log, source_sha256=sha(source.read_bytes()))
for item in proof['wrapper_mutations']['mutants']:
    source = Q / 'wrapper-mutants' / (item['name'] + '.cpp')
    report = json.loads((Q / 'wrapper-mutants' / (item['name'] + '.json')).read_text())
    record('wrapper mutant:' + item['name'],
           sha(source.read_bytes()) == item['source_sha256']
           and report['value_mismatches'] > 0
           and report['failures'][0]['case'] == item['first_mismatch_case'],
           value_mismatches=report['value_mismatches'], first_case=report['failures'][0]['case'])

for path, expected in [
    ('build/native-closure.json', proof['native_build']['closure_report_sha256']),
    ('build/gamma-texture-binding-test.log', proof['native_unit']['log_sha256']),
    ('build/gamma-texture-frontier/application.log', proof['native_app']['log_sha256']),
    ('build/gamma-texture-frontier/frontier.ppm', proof['native_app']['screenshot_sha256'])]:
    record('raw receipt:' + path, sha((C / path).read_bytes()) == expected)

frontier = C / 'build/gamma-texture-frontier'
capture = json.loads((frontier / 'captures/1382.json').read_text())
lines = (frontier / 'application.log').read_text().splitlines()
terminal = [json.loads(line) for line in lines if line.startswith('{')][-1]
record('canonical counter transition',
       capture['character_texture_loaded'] == proof['native_app']['character_texture_loaded_pre_controller']
       and terminal['character_texture_loaded'] == proof['native_app']['character_texture_loaded_terminal'],
       pre=capture['character_texture_loaded'], terminal=terminal['character_texture_loaded'])
record('same resident slots', capture['texture_slots_in_use'] == terminal['texture_slots_in_use'] == 762)
record('real terminal', terminal['frames'] == 1382 and lines[-1] == 'native session stopped: native_gamma_weld_binding'
       and json.loads((frontier / 'exit.json').read_text())['exit_status'] == 1)
record('command receipt', json.loads((frontier / 'command.json').read_text()) == proof['native_app']['command'])
record('prior image equality', (frontier / 'frontier.ppm').read_bytes()
       == (C / 'build/gamma-collision-frontier/frontier.ppm').read_bytes())
record('prior collision branch preserved', git('rev-parse', 'packet/gamma-collision-binding') == git('rev-parse', '6e9a1ec'))
result['all_matches'] = all(row['matches'] for row in result['checks'])
result['check_count'] = len(result['checks'])
print(json.dumps(result, indent=2))
