# Collision component: independent bounded review

Reviewed frozen `packet/collision-component-core` commit `78c2c592a93afdc8bd2b24eafa0abb5eba1defcc` against `54e159a`. Reviewer branch: `packet/review-collision-component`, based on `5125fab`. This is the first review round, combining standards and spec in one fresh reviewer under the explicit capacity constraint. No delegation.

**Verdict: bounded approval; no source blockers found in the recovered component.** Approval covers the five complete component bodies and `njScalor`, the typed TaskWork payload, and the type-only Context alias. It does not approve runtime integration or certify N3b.

## Standards

No blocking finding against AGENTS, GOAL or STYLE §12. The component uses semantic types, structured control flow, named masks/storage offsets, literal arithmetic constants and annotated source declarations. The arena offsets describe the existing allocation/cleanup interface; there is no guest-address dispatch, numeric state mirror, PE dependency, invented service layer or fixture-optional game state. Native boundary stops explicitly identify unsupported storage/corruption rather than fabricate behavior. Introduced helpers have NOTSA explanations.

`TaskWork::collision_state` is the sole typed component payload attached to the actual allocation. The primary reference selects that allocation; status is explicitly borrowed from the canonical primary owner. The weak task backlink avoids a task/component ownership cycle. Only ownership byte 5 and info reference 12 remain in arena storage for the existing scheduler cleanup (`task.cpp`, collision destruction stage). Future ownership-transfer code must maintain that metadata, as PACKET explicitly requires; this scope never transfers ownership. Released handles must not be reused as live component state.

ExactFloat is justified here by the absent complete call-site census, the original signed sqrt's m64 zero comparison, retained register exponent/precision, and explicit directed-rounding oracle inputs. Game expressions name the arithmetic rather than emulate instructions. The existing `signed_square_root` implementation is unchanged. Reversing every Context alias substitution restores `fidelity_float.hpp` byte-for-byte to base (SHA-256 `f8ab14de7dc01f0d054ce4098e61af34b7e5ba39f2281fc506be9d1864869dcb`). The entire readability baseline is byte-unchanged, including emulator allowance **37**. The lexical decrease is not debt removal. Existing judges, fixtures, tolerances and harnesses have no changed paths in this diff.

## Spec and source correspondence

Header declarations confirm CCL_Init `41caf0`, FreeColliWork `41c4e0`, Collision_Free `41caa0`, CCL_ClearInfo `418b60`, CCL_CalcRange `41bac0`, and scalar **njScalor `787cc0`**, not the tentative `43ef00`. CCL_INFO/colliwk field meanings match the mod-loader structures. Info decoding/copy preserves all 48 source bytes, including d and angles.

The complete init assembly preserves old free before allocation, first-allocation failure, the second-allocation failure's orphaned 168-byte work, full copy before ownership/id/count, and primary publication at `41cb98` before task backlink at `41cb9b`, clear and range. No EntryColliList call was added. Explicit partial-layout/allocator-wrap stops delimit unsupported corruption; they are not claimed as exact memory-corruption emulation.

Clear masks, hit slots, status, damage bits and sentinels agree with the full clear assembly; unrelated normal/id/range state is retained. Both free entries preserve conditional owned-info release, work release, then primary-reference clear. The existing scheduler consumes the same cleanup metadata.

Range matches the actual jump table: 0 sphere, 1/2 omit c, 6 capsule, all remaining forms use the default. Scalar multiplication order is x, y, z; addition is (z²+y²)+x² with the binary32 spill. Default extent is sqrt((c²+b²)+a²)+length, capsule adds b then a, sphere adds a. The strict greater maximum comparison, maximum*1.01 store, and maximum<0.1 override to 25 match the assembly, including unordered behavior. This is not a minimum-radius clamp. The reused signed sqrt preserves negative input sign.

## Receipt and identity audit

`audit.json` records hashes for all 28 changed files. Independently read and hashed: all 12 proof source identities; five full origin dossiers and five saved assembly files; the pinned EXE; all ten PE ranges and four constant encodings; both full retained reports and their common probe library. All match the packet receipts. The decoded seven-entry PE jump table agrees with the native form switch. Full dev/holdout report fields agree with the committed summaries. All seven retained mutant reports agree with their recorded mismatch counts: 444, 366, 678, 3232, 45, 1074 and 14.

Recorded qualification only: native C++17 unit exit 0; development 7964 cases / 0 value mismatches / 6462 FP partials, exit 2; holdout 8864 / 0 / 6834, exit 2. Each includes 888 scalar/sqrt cases with 834 FP partials. Both retain 146 lifecycle cases with zero value mismatches. FP flags are compared separately and neither partial is called pass. The profile receipt covers ten ranges with zero UNHANDLED or undetermined overlaps; checker receipts report provenance/owners/layers/readability success. The checker programs were read, not executed.

**Tests NOT RERUN.** No builds, tests, oracles, apps, Ghidra, providers or workers were launched. Only source/receipt reads and hash/byte inspection were performed; candidate files and Root/shared files were untouched. No mutable Main build was inspected. Receipt audit cannot independently prove the historical execution order or holdout secrecy.

Runtime binding, actual canonical status/Info consumption, Make/native-source/judge registrations, complete consumer rebuild, Root's full gate, GPU/actor output, N3b and later journey qualification remain Root-owned. Main's reported frontier remains native_gamma_CCL_Init at frame 1382, diagnostic exit 1; this review claims no visible progress. No push.
