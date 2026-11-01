Independent spec/fidelity recheck of `a833f7584320eb47b8e1ff48009fe111e11c8fb7`;
fixed point `53da4de`, correction comparison `3bdbcf1...a833f75`.
No new concrete fidelity finding. Prior P2 is closed within this bounded contract.

Criterion: `docs/PROMPT-complete-refactor.md:94`, “Retain all already implemented
behavior,” and §3.3/§3.6: preserve lazy reads, partial stores, exceptional values
and ordering. `src/guest/player_animation_reference.cpp:20` returns the supplied
binary32 values without arithmetic. All eight reached uses in
`src/player/animation/animation_engine.hpp` (98, 200, 207, 221, 365, 470, 479, 678)
now honor zero/one/unsigned correction. `src/cpu/fidelity_float.hpp:73` and :111
retain memory-operand arithmetic/comparison; getters introduce neither an x87
load nor eager evaluation of a later constant. This preserves arbitrary float-bit,
NaN and exception semantics by inspection, beyond the finite captured matrix.
`animation_native_access.hpp:23` retains literal defaults without a constant pool;
both bindings share the engine. Mode comparisons preserve signed-wide cleanup,
value 14 and store order (`animation_engine.hpp:505`); arena-helper deduplication
preserves failure translation.

Independently recomputed the stored 252-row comparison and 96-row prior-candidate mismatch
set, including every return/output/flag/diagnostic word. Both receipts have identical
old observations and the old-library hash frozen at `53da4de` (`31fbdfad…82957`),
distinct from both candidate libraries: this is not self-comparison. Seven blocks
and twelve masked FP controls do not exhaust arbitrary bits/unmasked exceptions.

Read-only validation matched 660 current Source hashes, exactly three intended
changes from historical Source, 172 retained artifacts, 279 recorded dependency
entries, 971 protected inputs, all 948 parent paths and 53 raw inputs. Historical
metadata, expectations/masks and old cohorts remain unchanged. All six full stock
streams match frozen hashes (103/16440/7944/28800/2004/1422 cases); 85 identity parts
match, preserving every FP/failure identity. Three native libraries have no guest
TUs; their stored 103/16440/7944-case streams match. Inspected eight-unit and
thirteen compiled/caught-fault receipts; the extra publication fault remains prior
evidence. Canonical tail/live references, partial headers, Sonic publication,
Gamma status/import distinctions and loud PSetMotion/story STUBs remain bounded.

No builds, library executions, original oracles, mutations, holdouts, journeys or
gate ran. Old libraries outside this checkout were not reopened; provenance relies
on frozen hashes/receipts. Five FP cohorts remain partial; Gamma retains exit 2.
Root retirement/denominator, Main protection union, inventory, product/profile
closure, fresh holdouts, journeys and coherent gate remain pending. No whole-player
or campaign acceptance follows.
