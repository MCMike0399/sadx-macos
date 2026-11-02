# Root-owned integration hunks

`ROOT-WIRING.patch` isolates the minimal shared-owner/build wiring from source
commit `ed00b1e3`. These hunks are implemented only in this supplied packet so the
actual app caller can be verified. Root owns reconciliation against newer Main.

1. `native_player_system.hpp`: include `gamma_motion.hpp` and add exactly one
   `unique_ptr<GammaMotionAssets>` beside `gamma_assets`. Retain every Sonic,
   effects, material, extra-work and future particle field. No Session header or
   app field, callback signature or other actor binding changes are needed.
2. `Session::load_player_assets`, in `native_gamma_assets_native.cpp`: construct
   GammaMotionAssets from the existing combined `*character_pack_` and imported
   GammaActorAssets, then publish that immutable owner beside the existing assets.
   The preexisting SceneState remains authoritative. No asset import or namespace
   renumbering is needed.
3. `native-sources.txt`: append the shared `animation_resources.cpp` adapter and
   `gamma_motion.cpp`. Preserve all existing membership and newer particle entries.
4. The separate verification commit appends one explicit Make rule and one unit
   manifest row. The four new protected files and these two existing-file hashes
   are the complete finite judge proposal. No inherited path is removed.

The owned implementation in `native_gamma_binding.cpp::set_motion` borrows these
owners and calls `update_gamma_motion`; failure retains phase, kind and unavailable
resource reason in its diagnostic. Successful update returns to the unchanged
common tail and its PJoinVertexes stop. The old missing-owner diagnostic remains
`native_gamma_set_motion` for callers without imported composition.

No particle-pool file, task/reset owner, other actor callback, Session header or
app file was changed. Root's existing Sonic owner and callback binding survive
unchanged. Animation's new runtime object pointer defaults absent for those old
call sites; Gamma supplies its real SceneState explicitly.
