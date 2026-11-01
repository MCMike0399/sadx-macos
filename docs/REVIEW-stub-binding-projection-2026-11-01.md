# Independent bounded STUB binding projection review

Candidate: `aaf759f` + `bd8130c` on `packet/stub-binding-receipts`, base `e5cf19d`.
Reviewed in the tool-provided isolated checkout on
`packet/review-stub-binding-projection`; candidate commits were cherry-picked as
`e186cd0` and `8299627`. No candidate, Source or Main checkout was modified.
Disposition: changes requested for two bounded validation defects.

## P2 — Require evidence before awarding binding review status

Location: `tools/inventory/refactor_inventory.py:112–132`.

The required-field check tests only truthiness. Recursive validation checks
fingerprints only when a dictionary contains `path`. Removing `evidence` from
`canonical_owner`, `native_binding` or `reference_binding` leaves a nonempty
dictionary and succeeds. Replacing `accepted_coverage` with a list containing an
unlinked string also succeeds. The result still reports
`bindings_reviewed_behavior_unrecovered` and projects a reviewed owner.

This is a concrete hole in the promised source revalidation: removing both native
binding evidence lists and then changing the native implementation's bytes still
allows both reviews through the full renderer. The declaration-header hash does
not cover the native implementation. The current authored receipts do include the
evidence; this finding concerns omissions the new validator accepts.

Require nonempty fingerprinted evidence for each owner/binding section and actual
fingerprinted coverage records before projection. Keep recursive validation for
nested evidence. Validate nonblank limits too: `limits=['   ']` currently passes.
Add regressions for missing/empty evidence and nonempty unlinked coverage, rather
than only changed or missing paths in an existing evidence object.

## P2 — Reject duplicate IDs inside a single fragment

Location: `tools/inventory/refactor_inventory.py:390–393`.

`json.loads` discards duplicate object keys before the identity loop runs. The
duplicate guard catches the same ID across fragments, but cannot catch two entries
in one fragment. Inserting a first entry for `src/game/save_completion.hpp:24`
with `behavior_status='implemented'` before the existing valid entry succeeds and
still reports two reviewed bindings. Reversing the order changes the result.
Conflicting review entries are silently resolved by order despite README's promise
that duplicate entries fail reconciliation.

Use duplicate-aware parsing for binding fragments and test duplicates within one
fragment as well as across fragments. This does not require expanding inventory
architecture or changing existing behavior judges.

## Read-only reproducers

Run from the candidate root. These use in-memory copies and mocks only; they write
no source, fixture, receipt or generated artifact.

```python
import copy
import importlib.util
import json
from unittest.mock import patch

s = importlib.util.spec_from_file_location(
    'inventory', 'tools/inventory/refactor_inventory.py')
m = importlib.util.module_from_spec(s)
s.loader.exec_module(m)
name = 'docs/refactor-inventory/reviews/save-warning-stub-bindings.json'
fragment = json.loads(m.read(name))
identity, review = next(iter(fragment['stubs'].items()))
paths = set(filter(None, m.git('ls-files', '-z').split('\0')))
stub = dict(path='src/game/save_completion.hpp',
            kind='annotation_declaration_candidate',
            annotation=review['annotation'],
            following_source=review['declaration'])

for field in ('canonical_owner', 'native_binding', 'reference_binding',
              'accepted_coverage'):
    bad = copy.deepcopy(review)
    if field == 'accepted_coverage':
        bad[field] = ['unlinked coverage']
    else:
        del bad[field]['evidence']
    print(field, m.reviewed_stub_binding(stub, bad, paths)['status'])
# All four incorrectly print bindings_reviewed_behavior_unrecovered.

raw = json.dumps(fragment).replace(
    '"stubs": {',
    '"stubs": {' + json.dumps(identity) +
    ':{"behavior_status":"implemented"},', 1)
original_read = m.read
with patch.object(m, 'read', lambda p: raw if p == name else original_read(p)):
    print(json.loads(m.render()['summary.json'])['reviewed_stub_bindings'])
# Incorrectly prints 2 instead of rejecting the duplicate ID.
```

## Confirmed scope and verification

- Read AGENTS, complete-refactor brief, relevant shared memory and STYLE §12.
  Reviewed source owner, both loud native failures, ordered reference hooks,
  callback unit and qualification/holdout receipt limits. The current authored
  evidence hashes match at the supplied base; no invented warning implementation
  or live guest binding was found.
- All 15 focused Python tests pass, including the five additions. They cover exact
  annotation/declaration/header identity, changed/missing nested evidence paths,
  explicit open behavior and stale IDs. They miss both defects above.
- Additional read-only probes reject moved-line and renamed-path IDs, duplicate
  IDs across fragments, changed header bytes, changed owner/native/reference
  evidence bytes and changed coverage receipt bytes. They demonstrate evidence
  omissions, blank limits and within-fragment duplicates being accepted.
- `python3 tools/inventory/refactor_inventory.py --check` exits 0 before this
  review document is added. Summary is byte-equivalent in meaning to the base
  except for `reviewed_stub_bindings: 2`. All 84 annotations and 11 other mentions
  retain their IDs, annotation text, following source and kinds; all 95 retain
  `unrecovered_open_debt`. Scope denominator remains 3,829 paths / 1,638 maintained
  entries / 2,191 boundaries; file and module dispositions remain unchanged.
- The protected manifest retains 936 paths; only the focused Python test hash
  changes. Existing checker implementations are unchanged. Local
  `python3 tools/gate/check_judge.py` exits 1: 883 present inputs and 53 missing
  ignored corpus binaries, no other reported violations. No storage restoration
  was attempted. The supplied full 936-input pass was not independently reproduced
  here. An initial invocation with unsupported `--check` exited 2 before the
  corrected invocation.

No builds, providers, oracles, semantic mutants, gates, network, storage operations
or delegates were run. Adversarial probes changed only in-memory inputs. This is
tool validation review, not module/system/campaign conformance or warning recovery.
Root's separate regular-resource changes to `native_session.hpp` and
`native_title_tasks.cpp` intentionally require fresh Source revalidation on
integration; the current fingerprints should not be exempted or blindly refreshed.
Only this review document is committed as review output; no implementation fixes,
baseline refresh, inventory regeneration or root-owned HANDOFF/STATUS edits.
