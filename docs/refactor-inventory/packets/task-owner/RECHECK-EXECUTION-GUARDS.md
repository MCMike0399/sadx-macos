# Independent execution-guard correction recheck

**Verdict: changes required before freeze/admission.** Complete-file review covers
`7a584a5edda823605e89c0970f570d1b87555258` at metadata HEAD `5d046aa`, plus the
requested immutable follow-up **`2f2711f436c77fd29bf5bf9fa336f7714fd8641d`**,
`fix(judge): establish actual Xcode task runner inputs`. Both complete reports,
`REVIEW-FINAL-RUNNER.md` (prior review `e6d057f`) and
`FINAL-RUNNER-CORRECTIONS.json`, and the follow-up's complete
`TOOLCHAIN-PREFLIGHT.json` were read. Source review `0e92356` grants no tool approval.

## Remaining and introduced findings

**P2 — Follow-up compiler symlink pin rejects its own unchanged freeze.**
In follow-up `reserved_run.py:128–132`, the dictionary key is
`str(path.resolve())`, while `digest(path)` hashes symlink text. Thus
`clang++ -> clang` stores SHA256(`link:clang`) under the resolved `clang` filename.
`verify_freeze:171–173` reads that filename and hashes binary bytes instead.
The installed Xcode `clang++ -> clang` identity was confirmed with read-only
`ls -l`; no compiler ran during this review.

Executed fake-only reproducer against the follow-up's exact Git source: create
temporary `clang` bytes, a `clang++ -> clang` symlink, SDK/resource/C++ trees,
Unicorn cache, linker/LTO/PE placeholders; patch only tool discovery, locations
and committed-freeze retrieval. Run real `root_inputs()`, serialize its returned
pins, then `verify_freeze()`. Unchanged inputs raise
`Frozen dependency changed: …/clang`. Preserve the driver's `clang++` spelling,
but record link identity and resolved target bytes under separate, correctly
typed identities. Add an unchanged complete-freeze round-trip regression;
the new driver-spelling test never crosses this boundary.

**P2 — Selected toolchain metadata remains independent of checked input pins.**
Follow-up `reserved_run.py:156–157` discovers tools separately for `root_inputs()`
and `toolchain()`. `verify_freeze:164–176` checks the former's map but never
compares `frozen['toolchain']` with discovery, nor proves every selected path is
represented. The build later consumes the unchecked metadata.

Executed independent fake reproducer after removing the temporary compiler
symlink: retain the real `root_inputs()` output for fake toolchain A, place an
existing different compiler B only in the serialized `toolchain.compiler`, and
mock Git retrieval to those same serialized bytes. Real `verify_freeze()` accepts
B although B is absent from `inputs`. No build function was called by this
reproducer. This is an internally inconsistent committed-freeze counterexample,
not evidence that Root has created such a freeze. Double discovery permits it
if selection changes between calls. Discover once, bind the complete selection
to its hashes, and require selection equality during verification. Cover compiler,
linker/LTO and search-root substitutions with refusal tests.

**P2 — Ordinary import origins and Python startup remain outside the seal.**
The stale-cache correction works for directly loaded Root helpers, recorder and
validator. However, follow-up `reserved_run.py:311–317` still imports original
modules normally; `reserved_observer.py:430` does likewise. Search precedence
does not override `sys.modules`. Neither verifies the original/transitive helper
origins or the actual imported Unicorn package against its pinned directory.
`-I` permits site startup; `.pth`, `sitecustomize` and interpreter environment
configuration are not included in `root_inputs()`.

Executed real tiny-Python reproducer: make a temporary venv without pip; add a
temporary site `.pth` containing `import sys,types; m=types.ModuleType('verify_original_task_work'); m.__file__='/outside-freeze/startup.py'; sys.modules[m.__name__]=m`.
Its `python -I -B -c` importing that name exits zero and reports the outside
origin. A separate fake archived module on the first search path does not replace
the preloaded module. Neither experiment imports or executes a real original.
Suppress unsealed startup and bind the allowed import closure, including actual
module origins; add a subprocess regression. Static inspection also finds
Unicorn's native-loader fallback outside its package; pinning package contents
alone does not check which library was loaded. No fallback library was loaded here.

## Four correction dispositions

1. **Stale Root bytecode: closed; complete origin integrity: partial.** The real
   timestamp-valid stale-cache test executes fresh source. Recorder/validator use
   source bytes; fake full input inspection confirms Unicorn `.pyc` inclusion.
   Verified archives reject extra caches. The startup/origin finding remains.
2. **Toolchain closure: partial.** Whole-tree content, symlink and target hashing,
   sanitized build environment, explicit sysroot/resource/C++/linker arguments,
   disabled configs/modules and declared search-root/LTO pins are meaningful.
   The follow-up fixes the observed absent C++ directory, deprecated linker option
   and C++ driver identity. Its preflight is retained evidence, not independently
   repeated here. Both toolchain findings above remain.
3. **Terminated-parent/stage continuation: closed within the stated process
   contract.** Terminal RUN refusal precedes work; direct parent PID, stage, token
   and pipe device/inode/type are checked. Only the parent retains the writer;
   children receive the reader. Exclusive stage markers prevent retry. The real
   tiny Python child exits **125** on EOF. No original/native call was exercised;
   this is not empirical proof of termination inside every foreign call.
4. **Constructor/setup attribution: closed.** First/later constructor failures
   create their own event before construction, with `pc=None` when unavailable;
   hook installation failures remain setup-only. Previous case evidence survives.
   `source_attempted` changes at helper/direct entry. Additional fake normal/error
   checks confirm constructor, call, emu method and installed hook restoration.

## Complete-file Standards / Spec dispositions

Standards: STYLE reference-tier allowance, semantic names, structured control flow,
explicit unknowns and bounded instrumentation. Spec: the request, prior findings
and ROOT-RESERVED-PROTOCOL. Both axes were assessed independently here; no child
agents were started, as requested.

| Complete file at follow-up | Standards | Purpose-specific Spec |
|---|---|---|
| `reserved_run.py` (389 lines) | Accept reference orchestration structure. | Changes required: three findings above. |
| `reserved_observer.py` (466 lines) | Accept explicit setup/error evidence and restoration. | Setup/direct corrections accepted; ordinary import depends on runner origin closure. |
| `test_task_reserved_run.py` (261 lines) | Accept readable isolated fakes and bounded subprocess tests. | Useful refusals/EOF coverage; misses symlink freeze round trip, selected-tool mismatch and startup origins. |
| `test_task_reserved_observer.py` (459 lines) | Accept maintained fake integration and explicit fault evidence. | Adequate for bounded constructor/direct corrections; real recorder rejects bypassed calls, wrong inputs and missing/shrunken outputs. |

## Verification and limits

Original snapshot: **60 tests, exit 0**. Exact follow-up Git blobs were copied to a
temporary tools/tests tree, unchanged, and run with bytecode writes disabled:
`python3 -B -m unittest discover -s <snapshot>/tests/python -p 'test_task_*' -v`:
**62 tests, exit 0**. All four original correction metadata hashes match; the
follow-up changes only the runner/test pair among these four files.

No prepare, _generate or _observe command, admitted run, actual SDK hashing,
compiler preflight, compilation/linking, original/native/emulator/reserved execution,
provider/network action, protected edit, refresh, gate, child agent or push was
performed. Observer integration used Python fakes. Prior normal/direct-fault
development evidence remains separate and was not replayed. All four reserved
seeds remain UNGENERATED.

Only this report is committed. Prior Root pure-helper four-file/two-test-module
dispositions remain intact; the tool-directory whole-module disposition stays
OPEN. Root retains exact freeze, distinct admission, actual qualification and
integration. No Task, Source, campaign or final-execution approval follows.
