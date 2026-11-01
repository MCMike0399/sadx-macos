# Independent menu texture binding review

Reviewed 2026-11-01. Target: `packet/menu-texture-bindings` at `09492b7`,
implementation `4ad9175`, registration `38cff6f`, baseline `ddac8f7`.
Source inspected in `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-11689037`.
Later Main metadata commit `628e36e` is not the comparison base.

## Finding

**P3 — Preserve the Japanese-load missing-declaration diagnostic.**
`src/runtime/native_session.cpp:342` changes the failure identifier from
`native_japanese_title_texture_declaration` to
`native_menu_textures_.japanese()declaration`. This is an accidental replacement
inside a string, unrelated to moving the owner. `UnrecoveredGameFlowCall` passes
that string straight to `std::runtime_error`, so callers observing `what()` now
receive a changed identifier containing implementation syntax.

Reproducer, established by source inspection: enter
`Session::load_japanese_title_textures()` before menu attachment, or after a
successful attachment whose `japanese_title.archive` is absent. Both versions
throw before creating services or loading an archive, but their `what()` values
differ. The second case can reach this boundary through Japanese title
initialization, whose language-zero branch calls the loader. Restore the old
literal. The frozen differential covers attachment/lookup, not this Session
loader, and therefore cannot detect this regression. No execution was performed
for this review. This affects diagnostics, not the guard or texture behavior.

## Ownership and design verdict

Bounded approval of the extracted owner and its consumer wiring, with the P3
correction above. No blocking ownership, aliasing, ordering, or design defect
found in the reviewed diff. This is not an unqualified packet completion verdict.

The owner contains exactly the previous list vector, pointer bindings, language
catalog, Japanese binding, and attached flag. It introduces no generic Services
container, registry, guest dependency, or relocated game algorithm. Its NOTSA
annotation identifies the native composition structure. Display still owns GPU
resources; Session still guards regular readiness and performs archive/display
operations; TitleScene still owns menu mode, loaded flags, readiness, and tasks.

Deleting copy/move protects pointers into the retained vector. This also makes
Session nonmovable through that member, but maintained construction sites in
`src/apps/sadx_native.mm` and `tests/unit/mac_native_logo_scene.mm` construct it
in place. Maintained consumers borrow Session; no move, copy, return-by-value, or
container relocation requirement was found. Approval covers those consumers,
not hypothetical external clients or ABI compatibility.

Regular storage is declared before the new owner in Session and therefore
outlives it. Successful regular attachment prevents subsequent regular-vector
replacement through the existing state guard. Inside the new owner the member
order matches the five former Session members: catalogs and aliases are
destroyed before owned lists. The change adds no destructor work. App Display
and Session/TitleScene construction order remains unchanged.

## Fidelity and STYLE 12 checks

- Attachment preserves the exact resize, list construction/aliasing, language
  publication, Japanese publication, then attached-flag order. Exceptions leave
  the same public catalog prefix and false flag. Same-shape retries rebuild the
  bindings and complete publication as before. Shape-changing retries were not
  newly tested or strengthened; the inherited raw-view invalidation risk is not
  represented as a new guarantee.
- Regular aliases borrow the original list object, including Japanese aliases.
  Owned aliases across languages share the retained list objects; they are not
  copies. Mutable list pointers retain live mutations. The alias branch checks
  only the declaration count and ignores filename/attribute payload, as before.
- An absent archive remains an absent archive. An absent texture-list index
  binds nullptr; it is not an attachment error. Session's ordinary menu loader
  still rejects absent archive names; named archive/null-list behavior remains
  delegated to the unchanged regular-texture implementation. These optionals
  describe imported resource declarations, not missing oracle facts.
- Lookup rejects unattached state or invalid language before checking index.
  Raw catalog/Japanese access adds no eager attachment or language validation.
  Japanese loading checks attachment/archive, selection checks Display/list,
  and release checks list availability. Original title initialization and
  fade-out language-zero guards remain in `game/title_menu.cpp`; the owner
  does not replace them with a universal guard.
- The extraction uses semantic names, structured control flow and native
  containers. No new numerical operation, source address, constant table,
  enhancement policy, unrecovered callee, or silent fallback is introduced.
  No feature framework or new hook is needed for this storage extraction.
- Existing resource algorithms and probes remain byte-for-byte unchanged from
  `ddac8f7`. Judge additions are separated from implementation. The title names
  table revalidation updates consumer evidence without retiring table debt.

## Evidence checked and limits

Read AGENTS, canonical memory instructions and relevant campaign/title context,
STYLE including section 12, JUDGE, packet description and qualification, actual
Session/TitleScene/app consumers, the baseline fixture, unit and registration.

Read-only comparisons establish that the fixture's attachment and lookup bodies
are verbatim copies of `ddac8f7`. Their adapter satisfies regular readiness and
uses synthetic content. They are a prior-native baseline, not Windows evidence.
The unit compares public state, alias equivalence, filenames/attributes, live
mutations, ignored alias payload, five failure positions and retry, and lookup
order. Attached null-list behavior is supported by unchanged source inspection;
the synthetic attached catalog does not itself exercise an absent list index.

All seven qualification source/test hashes match both `4ad9175` and the target
worktree. All 936 protected-input hashes match the private manifest. The new
unit has its manifest row. The frozen receipt records five successfully compiled
mutants rejected at runtime: copied regular alias, early ready flag, wrong
language, validating ignored payload, and eager catalog validation. Receipt
evidence was inspected; mutants and unit were not rerun.

Cached native-build log hash matches qualification and its terminal file is 0;
the log ends with native force-load closure PASS. Qualification records strict
Session/title-task/app TU compiles as 0 and reuse of only pinned installed
SDL/libusb. Both cached full judge hashes match the qualification receipt:
menu_resource_lifetime has 1,401 cases/0 mismatches; regular_textures has 609
cases/0 mismatches. Their source/header/probe files are unchanged from base.
These source judges do not independently prove the new native binding owner.

No builds, unit executions, oracles, mutations, gates, network/provider/storage
operations, workers, source changes, global inventory changes, HANDOFF or STATUS
edits were performed. Only this report is committed on the review branch.
Native journeys were running at task issuance; no terminal journey pass is
claimed here. Root retains remaining qualification, integrated gate and closure.
No Windows binding equivalence, new GPU lifetime proof, debt retirement, complete
resource/Session ownership, or campaign completion is certified by this review.
