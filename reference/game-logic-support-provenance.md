# Native game-loop leaves

`game_logic_support` implements actual40ec80 and413920 local bodies using caller
owned optional source facts. The raw256×3 byte fields remain separate from HID,
virtual-key and player-input namespaces. Only lane0 is consumed. No Win32 input,
clock value or frame-rate default is inferred.

40ec80 mode0 copies256 bytes and the special byte; mode1 ORs into aggregation;
mode2 ORs aggregation back into the raw fields. Other raw32-bit mode arguments
execute the observed no-op return. Unrolled prefetch-before-current-store order
is preserved so unavailable facts retain the exact committed prefix.

413920 preserves signed division truncation, explicit zero/INT_MIN÷−1 #DE
boundaries, modulo32 INC and low32 IMUL. Both uncapped outputs are written before
the selector read that may apply the original six-tick clamp. Negative/wrapped
results have no invented cap. A missing selector retains prior uncapped stores.
The disabled branch consumes no divisor, delta or multiplier facts.

The final actual-instruction comparison covers1737 edge profiles and4117 timing
profiles with zero state/presence/boundary/trace discrepancies. All288 expected
source #DE cases remain in the corpus and match native tagged failures; prior
outputs survive them. Missing-fact profiles stop immediately before the actual
source read, rather than pretending source memory contains C++ optional values.
Optimized and UBSan unit checks pass. The oracle restores clean caller CPU state
per experiment, preventing prior divide traps from contaminating later calls.

The first edge comparison exposed three miscopied missing-read instruction
addresses. Exact LLVM corrected them to40ecd6/40ece4/40ecf2; no arithmetic changed.
The final report and ledger preserve this superseded finding and genuine Ghidra
Evidence IDs. Original exception dispatch, live Win32 clock/input observations,
updated-mod patches and physical-key mappings remain outside this leaf scope.
The parent413dd0 orchestration borrows this same timing owner for composition.
