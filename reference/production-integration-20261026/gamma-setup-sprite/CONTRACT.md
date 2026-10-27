# Sprite constructor packet contract

Base cf1c51a; packet/gamma-setup-sprite is isolated from Root integration.

Engine sprite_registry::State is the single new canonical owner of source head
3c4858c and SpriteCount 3c48590. Searches of src and SDK found no existing native
owner; SDK names only SpriteCount (SADXVariables.h:2328). RegisterSprite and
SpriteRegistryHead are proposed neutral role names, not original SDK symbols.
Root should register those names with the evidence below before integration.

register_sprite borrows State, the real TaskWorkArena and a nullable Definition
identity. The opaque Definition forward declaration asserts no size or contents.
The source stores that identity without reading it. Nodes retain the arena
allocation, next identity and nullable interior extra-data view. All other source
record bytes are left as established by CAlloc; no meaning is assigned to them.
The allocation seam is plain link-time allocate_zeroed(arena, count, size), to be
bound to the existing CAlloc implementation. This packet does not supply a fake
pool or add runtime services. The probe and unit bind it with the actual arena.
Host typed node allocation is additional representation storage; host bad_alloc
is outside source allocation-failure qualification. No automatic source release.
Wrapped undersized allocation/interior pointers are representable identities,
not permission to access outside the arena. No data consumers are recovered.

Gamma binding is deliberately absent. Source 4c5420 captures the pointer at
982148, calls the constructor with extra size zero and assigns result to 982154.
982148 points to 9820f8; it is not a filename. No native payload or canonical
982154 owner is added until the resource schema/import is proved. Existing
setup_dependencies metadata describes a boundary, not an imported definition.
Root needs xrefs/consumer bodies for 9820f8 and 982154, plus definition-consuming
sprite registry functions (likely 456cd0 and registry updates/removal, to verify
by xrefs) to establish exact layout. No new provider was started here.

Authoritative Root dossiers, provider closed rc 0:
/Users/burbujamc/Developer/sadx-wt-normal-uv-strip/reference/inventory-raw/gamma-setup-recovery-20261026/gamma-setup-{456c50,4c5420}.json
456c50: ev_d230a26a849ab50f52074bcb36135343ad49bb007dc52671f2bb869ac52d3b42.
Complete body 456c50–456ca0 inclusive (81 bytes), no pcode truncation; full
assembly inspected. Only callee is CAlloc 40b2d0. References include Gamma,
OSmoke 544cd0 and an unowned call site 49c137; not all callers are recovered.
Source order: null guard; CAlloc(1, extra+28 wrapped32); failure guard; read
count then head; increment local count; node.next; global head; global count;
nonzero extra interior pointer; definition; return. JBE follows TEST ESI,ESI.

Verification uses both the unchanged normal compiled core and a separate test-only
instrumented copy. Five native snapshots follow next/head/count/extra/definition
stores and compare with original instruction prefixes. Before-store fields whose
source storage is uninitialized in wrapped-small allocations are excluded from
prefix snapshots, while final untouched storage bytes are always compared.
The instrumentation is not linked into the product and adds no native service.
Seven mutants include head/count publication reversal, which preserves final
values but is caught by prefix comparison. CAlloc remains intercepted: strict
constructor equality is not a proof of lower allocation implementation.

Root integration actions remain pending: approve the new names using
symbol-proposals.tsv, bind allocate_zeroed to existing native allocation with
proper boundary errors, register the unit/probe compiler recipes and manifest
row, register new judge files, independently review the packet, then integrate.
No old judge, original core, app, manifest, compiler recipe or baseline is edited.
Private provenance checks use a generated copy of Root's symbol DB with only the
two proposed names appended; this is conditional validation, not registration.
The private symbol generator cannot run fully here because reference/sa-tools
is absent; no dependency install or shared checkout change was attempted.
