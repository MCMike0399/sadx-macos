# Gamma setup native packet

Base Main ca640c5; branch packet/gamma-setup-native. This packet carries the
reviewed da73181 constructor as db3aa61 and its isolated Task enum compatibility
fix 9e3bd1b. Review1938ef3 applies to the constructor only. Root owns fresh review,
shared integration and acceptance. No push or full gate.

Implements the complete GammaLoadSetupResource body through gamma_setup::load_resource:
capture the live definition, register with zero extra bytes, publish even NULL.
The real imported definition includes eight Ninja animation frames, blend modes,
count and all particle parameter bytes. It borrows the canonical E102EFFECT list.
It adds no material copy, dummy definition, actor substitute or skipped tail.

Owned additions: engine sprite_definition.hpp and gamma_setup_allocation_native.cpp;
player gamma_setup_resource.hpp/cpp; dedicated tools/import/gamma_setup_resource.py;
three Gamma unit programs, probe, original oracle, mutants and importer tests.
The inherited constructor header changes only its formerly opaque-layout comment.
All shared owner declarations, app composition, manifests, Makefile, native source
list, frozen evidence and global inventory/protection files remain untouched.
Source/context details: reference/packets/gamma-setup-native/SOURCE.md.
Checks: 704 development and 2,048 holdout cases pass; six new mutants caught;
three native programs and five Python tests pass. The unchanged Gamma caller
retains 1,422 cases and 498 FP partials. Exact identities/hashes are adjacent.
Focused reproduction: sh reference/packets/gamma-setup-native/verify.sh.
Exact manifest rows and compiler constraints are in the adjacent REGISTRATION.md.

Root observation correction is recorded in
reference/packets/gamma-setup-native/review-correction.json. It preserves all
original inputs and adds 144 two-definition/reentrant cases to each cohort:
848 development and 2,192 retained holdouts pass. Six original faults and a new
wrong-definition fault are caught. The old nonnull projection hid all 32 failing
original-input cases of that new fault. Source and original receipts are unchanged;
fresh correction review remains pending.

Root integration requirements (all required before the application can use it):

1. Register the inherited two symbol proposals and this packet's four proposals.
   Private-symbol provenance validation is conditional, not canonical registration.
   Add sprite_registry.cpp and gamma_setup_resource.cpp to native-sources.txt, plus
   gamma_setup_allocation_native.cpp if not already picked up by the native wildcard.
   There must be exactly one allocate_zeroed definition in the product link.
2. Import gamma-setup.bin with the dedicated CLI and retain provenance outside the
   source install. No CharacterPack or global native_assets.py schema edit is needed.
   Read these bytes in app asset composition; pass them into a Root-owned attach/load
   boundary. Validate character_resources_.assets()[6][2].archive == "E102EFFECT"
   and resource_name == "gamma_effects" (the imported 91c570 identity) before
   borrowing lists()[6][2]. The
   decoder checks twelve list entries but cannot infer a list's source identity.
3. Add one sprite_registry::State owner to Session, shared by every future registry
   caller; append a borrowed sprite_registry::State& to gamma_selection::Context.
   Add gamma_setup::State and unique_ptr<const gamma_setup::Asset> to PlayerSystem.
   Decode into that stable Asset using character_resources_.lists()[6][2]; initialize
   players_.gamma_setup.definition = &players_.gamma_setup_asset->definition once.
   Keep Asset, texture list and arena alive through all registry users. Do not
   reinitialize the definition pointer on every call: it is a live source owner.
4. Replace native_gamma_binding.cpp's load_setup_resource stop with
   gamma_setup::load_resource(context.players.gamma_setup, context.sprites, arena(context)).
   Retain the existing single source-entry annotation on gamma_selection's seam;
   it may become FUNCTION when binding is integrated. The helper deliberately has
   no duplicate entry annotation. Preserve the following materials/effects/tail.
5. Register unit rows sprite-registry, gamma-setup-resource, gamma-setup-allocation
   and gamma-setup-caller. Exact source/compiler inputs are in verify.sh and the
   inherited constructor verify.sh. gamma-setup-caller needs the imported pack path
   argument; gamma-setup-resource runs both synthetic and imported-byte inverses.
   Register native-sprite-registry-probe.dylib and native-gamma-setup-resource-probe.dylib
   using those exact recipes; add oracle-gamma-setup-resource calling the new oracle.
   New tests/oracles and the separate enum fix need normal judge reconciliation.
6. Arrange fresh source/STYLE and observer adequacy review, then source-only closure,
   actual normal-input native run with saved image/state/exit, and coherent gate.
   No worker result here establishes N3b. Reproduce the input schedule from the Root
   frame 1332 receipt; the older frame 1382 schedule is different.

Next actual call after setup is shared material consumption, currently
native_gamma_shared_material_binding; bind the existing five imported identities.
After materials and the ID-dependent time HUD, initialize_effects reaches 49fb10,
then PSetMotion, PJoinVertexes and the complete common tail/display. Existing Gamma
metadata swaps the descriptive labels for setup/effects; source entry addresses
and the actual caller are authoritative. No adjacent material/effects rewrite ran.

Limits: CAlloc is intercepted in the original comparisons; the production binding
and real exhaustion have separate native tests. Host bad_alloc and arbitrary fault
delivery remain unqualified. Full sprite update/display/cleanup, native destruction,
GPU/actor output and normal-input app integration remain Root work. Raw-header
consumers must not be bound to the typed node as if raw pointer bytes were coherent.
Cleanup must preserve the stale Gamma result identity and explicitly release its
arena allocation, rather than silently clearing it or relying on shared_ptr release.
Existing Gamma FP partials remain partial, with each identity retained. No reset
reserved inputs, stopped schedules or storage jobs were used.
