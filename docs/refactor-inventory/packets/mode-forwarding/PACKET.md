# Mode forwarding and complete diagnostic contract

Base `353d528`; baseline commit `d860512`. This is an evidenced judge packet.
No `src/` file changes. Main's running gate and all other packets are untouched.
The complete backend/services qualification gaps come from the independent
`frame-mode-foundations` review. No new gameplay or callee implementation is added.

## Before editing a protected judge

Freshly compiled the existing native unit and reference probe directly from their
maintained translation units. The unchanged original mode judge passes all
24,000 cases / 252 paths. The complete original native ABI is frozen locally as
`build/mode-forwarding/baseline/native-abi.bin.gz`: each 1,220-byte record contains
return code, all 48 input words and all 256 output words. `baseline.json` pins the
raw stream, input identity and every native failure ID. Original comparison only
claims the fields and owner substitutions that its existing script compares.

## Added observations

`game_mode_forwarding.cpp` exercises all 27 finite `NativeServices` operations:
exact single invocation, unequal signed/unsigned operands, return bits, live
callback mutation, propagated exceptions and named missing-service stops.
Nine complete native dispatcher scenarios check ordered callback identities and
57 injected failure prefixes. Every prefix observes all 16 state words before its
callback and retains exactly the reached stores. Positive cases independently pin
transition outcomes, seven destructor kinds, short-circuit readiness, callback
mutation of the title load phase and mode, the wrapping increment and the
SuperSonic store between vsync and sound loading. Borrowed signed sequence reads,
null sequence failure and nested native-world routing are covered separately.
Three semantic module requests leave state unchanged and invoke no service.

`game_mode_diagnostics.cpp` checks 26 direct reference scenarios. It observes every
result field: failure, outer/nested boundaries, completion, pending site/callee,
three argument availability bits and values. It covers all six movie-cleanup
owner-prefix stops for failed/cancelled/unavailable receipts, absent invocation,
required return missing, known-zero versus absent arguments, high-bit payloads,
missing mode/movie facts, a callback clearing a later fact, and nested host-error
unwinding restoring the outer reference binding. These are adapter contracts,
not invented original-machine outputs or downstream service implementations.

The existing probe preserves its C ABI and all old outputs. A companion export
appends eight diagnostic words. Both exports use the same fixture implementation;
no game algorithm is copied into a probe. `check_game_mode_diagnostics.py` compares
every old record against the immutable full stream and checks each appended
pending payload against the existing fixture's trace/argument contract. All
24,000 old ABI records remain exact; complete companion records are retained in
`build/mode-forwarding/current/diagnostics.bin.gz`, with all outcome IDs and stream
hashes in its report. The unchanged original oracle is independently rerun on the
current library: 24,000 cases, 252 paths, zero mismatches. Its canonical tracked
report is refreshed with current bindings; cases and comparisons are unchanged.

## Direct module-request evidence

`check_game_mode_module_request.py` reads pinned executable bytes through PEImage
and decodes the complete 21-byte `4221e0` body: it pushes the input and literal
format address, calls `51f680`, restores the stack, clears EAX and returns.
The printer is a one-byte RET. Twenty-five actual original executions retain
caller arguments and stack discipline and return zero, including invalid-looking
argument words that are not dereferenced. The bound pinned profile census has no
active overlapping patch. The separate profile-bound receipt preserves this
source evidence; no live mod execution or general diagnostic behavior is claimed.
This fills the earlier missing direct-body evidence, without changing the no-op.

## Discrimination and honest development failures

Ten final copied faults compile and are caught: swapped frame operands, lost
movie identity, wrong cleanup owner/destructor kind, swallowed callback failure,
reordered destructors, early SuperSonic store, mode cached before callback, lost
nested boundary and lost pending argument payload. Source comes from immutable
base Git blobs. The tool freezes unit bytes once, compiles copies, fingerprints
all sources/tests and retains every compile/run result.

`mutants-initial.json` retains one invalid compile (unused movie parameter), which
is not counted. `mutants-corrected.json` preserves the next attempt. The final
`mutants-final.json` binds the actual current frozen units and all ten valid
faults. Initial diagnostic test setup used nonexistent `unported` rather than
actual `unavailable`; it was corrected after reading the declaration. The first
instruction extraction requested 19 bytes instead of the complete 21 and was
refused; it produced no success receipt. None changed an original expectation.

## Reproduction and remaining acceptance

From this checkout, with pinned Python/Unicorn/Capstone already local:

```sh
make -j2 build/game-mode-forwarding-test build/game-mode-diagnostics-test
./build/game-mode-forwarding-test
./build/game-mode-diagnostics-test
PYTHONPATH=tools/lib:tools/oracles <pinned-python> tools/oracles/check_game_mode_diagnostics.py <current-probe.dylib> build/mode-forwarding/baseline/native-abi.bin.gz <new-output-directory>
PYTHONPATH=tools/lib:tools/oracles <pinned-python> tools/oracles/check_game_mode_module_request.py <sonic.exe> <new-evidence.json>
python3 tools/oracles/mutate_game_mode_contracts.py <new-mutation-directory>
```

The new tests have manifest rows and explicit direct-source/header Make rules.
`QUALIFICATION.json` records hashes, results and limits. Original raw libraries,
full ABI streams and copied faults remain locally pinned; do not archive/release
these current proofs for headroom.

Root still needs fresh independent review, additive integration/protected manifest
union and a coherent gate. Current product/Session consumer closure and journey
receipts require explicit source fingerprint binding before module disposition.
Test services do not certify actual downstream algorithms. No final holdout,
whole game-mode/Session/campaign conformance, native actor, stage FPS, physical
controller, new route, full gate or push is claimed by this packet.
