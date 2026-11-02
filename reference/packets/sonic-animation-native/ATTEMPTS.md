# Retained attempts and coordination

- Started on exact Main 08820f79, created only packet/sonic-animation-native in
  the supplied worktree. No shell-created checkout or child agent was used.
- Checkout lacked game assets and build outputs. Copied existing imported
  character assets locally and copied public controller dependencies with
  symlinks dereferenced. No writable build path points to Root.
- Discovery commands for nonexistent native_animation_resources.hpp,
  character_action_pack.hpp, native_asset_values.hpp and an animation-native
  packet found no files. Used the actual canonical modules instead.
- The first profile checker ran with its empty default build/oracles directory:
  zero addressed ranges, so it is not evidence of source/profile coverage.
- First provenance checker exited 1 before checking because local symbols.json
  was absent. Rerun with Root's existing read-only database exited 0.
- No implementation or original table comparison mismatch occurred. Deliberate
  invalid-input and compiled-fault rejections are retained separately in the
  native test, importer.json and faults.json; they are expected negative cases.
- Root requested exclusive build slot for its combined gate. Finished the
  current compile, acknowledged the clear slot to parent thread
  d2fc033a-49f8-41c8-b9c3-4a0d87ca2ae4, and held all further heavy execution.
  Documentation/source work continues. No full gate is launched here.

- Root explicitly released the slot after its daa443e4 combined gate exited 0.
  Resumed only focused sequential compiles/checks, with no full gate here.
- Source review found that an unfilled typed declaration array defaulted to
  known nulls in candidate a77d7110. The added seventh negative exposes that
  behavior in a separately compiled old-header witness (exit 1). Correction
  02a6d9a4 defaults entries to unavailable; final optimized/UBSan tests pass.
  All six previous typed negatives and twelve JSON negatives are retained.
- Final 20 command receipts all exit 0. The earlier empty-report profile check
  is superseded by one explicit full producer range, zero UNHANDLED overlaps.
- Final whitespace review caught a space-before-tab in the documentation-only
  nested Make snippet. Replaced it with an explicit <TAB> placeholder and reran
  diff checking; no compiled source or verification input changed.
