# Independent bounded recheck

Candidate `011369364633c0ccd8a9b4487fe3e8455a93f1de`, pinned in the assigned
checkout on `packet/animation-observer-recheck`. Reviewed the prior `fe12f2d`
report imported by `4d20651`, revised observer documentation, corrections,
freeze, seal, code and focused regressions. **Bounded acceptance: prior P1 and
P2 are closed; no newly actionable finding in these changes.**

P1: independently reconstructed all 372 `tools/**/*.py` paths and SHA-256 values
from immutable `38e1c69`; the seal and extracted current tree match exactly.
Its source commit matches the original freeze and its tree identifies `tools`.
Membership rejects omitted/additional helpers; byte checks reject drift.
Continuation requires the seal in committed authorization definitions, verifies
all four original frozen inputs, helper closure, runner definitions and libraries
before importing the runner or calling `run.modules()`. The inspected transitive
local imports, including task-work helpers, fall within that closure. Gamma's
three consumed content paths match the checked frozen inputs. All nine frozen
runner definitions, eight custom libraries, ten numeric libraries and eight revised
definition hashes match. The original freeze and retained captures were not
changed by this correction; the revised freeze seals the changed bodies.

P2: return now requires zero boundary and actual EIP equal to STOP. An established
stop requires actual EIP equal to the reported nonzero boundary in the inherited
stop set. Count exhaustion and unclassified stops become `unexpected_error`,
which continuation rejects. Exact fault signature, prefix, written-null
publication, complete native result/return equality, separate FP partials and
ordered independent-component completeness remain enforced. Nine pure focused
tests pass, exit 0.

Separately, Main `48df570` (under `44ee9b2`) adequately closes the bounded
inventory-generator protection gap: explicit `FILES` inclusion uses existing
hash, missing-file and ancestor/file symlink rejection. Its added pure regression
passes directly from Git blobs, without switching Main. Independently verified
1,269 manifest entries, 1,266 unchanged inherited hashes, two evidenced checker/test
updates and one generator addition; evidence digest matches. Generator semantics
and Source are unchanged. The retained 17-test receipt was inspected; only its
new regression was rerun here. Full gate remains Root-owned and pending.

Original exit 1, raw inputs and prior attempts remain retained; case 0 is completed
and IDs 1–1023 remain UNRUN. No emulator, continuation execution, reserved case,
holdout, mutation, build, gate, journey, provider, storage operation, Source edit
or push occurred. Fresh committed Root authorization, protection union, registered
builds and coherent gate remain required before activation. PSetMotion, playback,
destruction and N3b remain open. Accepted positive composition and four compiled
fault receipts were not rerun or expanded into gameplay claims.
