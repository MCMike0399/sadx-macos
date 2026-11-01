# Bounded module inventory judge review

Reviewed `00bbbbcdf54d1b4f0ed66b62109ac98d8f5323f9` against
`628e36e6abdfdf862edfbe7152c13c2913e78e9f` on 2026-11-01.
Scope: module policy validation/projection, focused tests and MODULE-REVIEW.md.
Verdict: changes requested for the three gaps below. No campaign verdict.

Standards: no separate documented coding-style finding in this bounded diff.
Spec: two P2 validation gaps and one P3 evidence-projection gap.

1. **P2 — Missing nested evidence can certify module conformity.**
   `tools/inventory/refactor_inventory.py:312` checks only the top-level evidence
   list. Owner/consumer/judge records can name nonexistent evidence paths and
   still produce `reviewed_conforming` and `reviewed_with_recorded_limits`.
   Reproduced through actual `render()` with the currently reviewed
   `src/platform/mac/mac_save_writer` members and exact current hashes; only the
   in-memory module policy changed. A real top-level document does not validate
   missing evidence supporting the accepted judge or resolved owner/consumer.
   Validate evidence references in the authored semantic records, including
   obsolescence proof, and add a render-level rejection regression. This does
   not ask the checker to prove the reasoning true or expand caller scope.

2. **P2 — Missing classification limits still close a module.**
   `tools/inventory/refactor_inventory.py:62–67,79` omits classification_limits
   from validation and defaults it to an empty list. Removing that field from
   an otherwise accepted review still yields `reviewed_conforming`, `limits: []`.
   MODULE-REVIEW.md explicitly requires recorded limits for conforming modules;
   silently accepting their omission loses the bounds on accepted consumer and
   judge scope. Require an explicit nonempty limits record and test missing and
   empty cases. An explicit statement of no additional limitations is reviewable.

3. **P3 — Obsolescence proof disappears from the generated module record.**
   `tools/inventory/refactor_inventory.py:68–79` requires obsolescence_proof but
   does not return it. Matching obsolete members and a supplied detailed proof
   yield an obsolete module whose semantic_review omits the removal rationale's
   proof entirely. The authored policy retains it, but the generated projection
   loses the distinct evidence the guard required. Preserve the proof and add a
   round-trip assertion; keep obsolete status separate from conformity.

The seven existing tests pass but do not exercise these cases or render-level
evidence validation. The following reproducer changes no files and uses actual
rendering for findings 1 and 2; finding 3 uses synthetic obsolete member rows.

```sh
python3 - <<'PY'
import importlib.util, json
from pathlib import Path
from unittest.mock import patch

s = importlib.util.spec_from_file_location('inv', 'tools/inventory/refactor_inventory.py')
i = importlib.util.module_from_spec(s)
s.loader.exec_module(i)
original_read = Path.read_text
policy = json.loads((i.OUT / 'review.json').read_text())
outputs = i.render()
records = [json.loads(row) for key, value in outputs.items()
           if key.startswith('files/') for row in value.splitlines()]
name = 'src/platform/mac/mac_save_writer'
members = [r for r in records if r['module'] == name]
assert len(members) == 2
r = dict(disposition='already_conforming', reason='Whole module inspected.',
         reviewed_members={m['path']: m['sha256'] for m in members},
         evidence=[{'path': 'docs/refactor-inventory/MODULE-REVIEW.md'}],
         semantic_justification='Interface and implementation inspected.',
         classification_limits=['Bounded authored review.'])
for field in ('resolved_owners', 'resolved_consumers', 'accepted_judges'):
    r[field] = [dict(resolution='Inspected', evidence=[
        {'path': 'docs/DOES-NOT-EXIST-' + field + '.md'}])]

def projected(review):
    replacement = dict(policy, modules={name: review})
    def read(path, *a, **kw):
        if path == i.OUT / 'review.json':
            return json.dumps(replacement)
        return original_read(path, *a, **kw)
    with patch.object(Path, 'read_text', read):
        out = i.render()
    return next(json.loads(row)['semantic_review']
                for row in out['modules.jsonl'].splitlines()
                if json.loads(row)['module'] == name)

assert projected(r)['status'] == 'reviewed_conforming'  # Missing evidence accepted.
without_limits = dict(r)
del without_limits['classification_limits']
assert projected(without_limits)['status'] == 'reviewed_conforming'
assert projected(without_limits)['limits'] == []
obsolete = dict(r, disposition='obsolete', obsolescence_proof={
    'reason': 'No maintained consumer or behavior remains.',
    'evidence': [{'path': 'docs/refactor-inventory/MODULE-REVIEW.md'}]})
result = i.reviewed_module(obsolete, [dict(m, disposition='obsolete') for m in members])
assert 'obsolescence_proof' not in result
print('All three review reproducers confirmed.')
PY
```

Validation: `python3 tests/python/test_refactor_inventory_review.py -v` passes
7 tests; `python3 tools/inventory/refactor_inventory.py --check` exits 0;
`python3 tools/gate/check_judge.py` reports 934 files, 0 violations. No frozen
input restoration needed. Inventory retains 3,811 tracked scope entries, 1,634
maintained/script entries, and 1,039 open modules; no authored module closures.
Additional in-memory controls reject unknown module IDs, a non-map modules
policy, moved/deleted/changed members, an unreviewed member and missing top-level
evidence. Existing tests also reject added/renamed members and purpose mismatch.
The diff preserves the existing file guard, denominator/removal checks and
ratchets; no game source changes or widened directory exceptions were found.

Only this report is authored by the reviewer. No inventory, HANDOFF, STATUS,
source, protected inputs or Main edits. No builds, oracles, gates, providers,
network, storage operations or workers. Runtime/system/campaign acceptance,
semantic truth of future authored reviews, and automatic invalidation on new
unrelated callers remain outside this review.
