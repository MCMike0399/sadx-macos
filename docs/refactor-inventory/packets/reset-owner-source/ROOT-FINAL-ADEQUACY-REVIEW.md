# Independent final collector adequacy review

Changes required. Reviewed `git diff e898411...b41def6` and `git log
e898411..b41def6 --oneline`; exact head
`b41def60e81b1beed6debdc574de5db8250ea35d`, base
`e8984111fc218cfcf2d57bc0471c1db6ab75a1dc`.

Standards: no additional actionable STYLE §10/12 or ARCHITECTURE violation found.
Spec: four findings against the requested fail-closed qualification and
ROOT-RESERVED-PROTOCOL observation contract (PROMPT-complete-refactor §3.8).
Paths below are repository-relative.

- **P1 — Unsealed collector startup.**
  `reference/packets/reset-owner-final/collector.py:432` dispatches without
  requiring the pinned running interpreter or `-I -S -B`. Lines 158–161 only
  compare two frozen paths; the executable check at 57 belongs to preparation.
  An environment/site-influenced parent can reach model execution. A disposable
  fake with unsealed flags/interpreter reached mocked `run`. Require sealed
  startup before dispatch and test refusal before output/model calls.
- **P1 — Incomplete interpreter closure.**
  `reference/packets/reset-owner-final/collector.py:83` pins the 34,640-byte
  Python launcher but omits its Homebrew `Python.framework/Versions/3.14/Python`
  runtime, established by static `otool -L`; standard-library execution dependencies
  are also unpinned. Altering these leaves all 1,435 hashes valid. Seal the actual
  interpreter execution/import closure and test dependency alterations.
- **P2 — Dropped reached buffer facts accepted.**
  `reference/packets/reset-owner-final/collector.py:271` checks snapshot length
  but generally ignores buffer facts at indices 2/3. Replacing both first-call
  facts with `[]` in retained development observations passes validation and
  complete comparison. Validate every presence/value pair and expected reached
  buffer state; add symmetric omission tests.
- **P2 — Extra media ownership survives diagnostic output.**
  `tests/fixtures/reset_reference_final.cpp:179` retains the stopped player until
  the loop iteration ends, including output at 194–203. The protocol explicitly
  forbids retaining extra player ownership through diagnostic output. Limit that
  retention to safe callback completion and verify the lifetime boundary.

Validation: all 1,435 pins match freeze SHA256
`56eb59c06c38d539ce6cdec697cb52ea5b56d70cf6984428c4a7f0226c1861e5`;
17/17 permitted fake tests pass. Additional disposable fakes used development
records only; one incomplete flags mock failed before dispatch, then was corrected.
No build, model/library load, original execution, reserved execution or admission.

Body dispositions: `development.py`, `original.py`, Sonic fixture, Session fixture,
parent guard/fake, Makefile, Session friend and both changed unit bodies inspected
completely; bounded interfaces/PC/STOP, 128-word padding, fourteen-page/200-word
comparison, forced include and actual imported Session alias are explicit.
`collector.py`, `test_collector.py`, reference fixture require corrections above.
Eight frozen definitions remain unchanged. Historical Session failure/supersession,
53 missing corpora and existing fidelity/lifetime limits remain; no Source fidelity,
Root admission, integration or campaign-completion approval.
