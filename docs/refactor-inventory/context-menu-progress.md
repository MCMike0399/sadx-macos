# Shared menu progress predicates

This connected naming/shape migration preserves the existing eligibility algorithms,
borrowed persistent story bytes, imported stage catalogs and all original probe APIs.
The reviewed menu/save fragment c28b9a6 identifies the concrete predicate findings.
The full refactor campaign and broader save/UI/Session ownership remain open.

The pinned SDK's SADXEnumsNew.h:578-602 defines playable/completed flags. The menu
order is Sonic, Tails, Knuckles, Amy, Big, Gamma, then the extra selection slot;
source tables deliberately swap Big/Gamma relative to raw character IDs. SDK Miles
and E102 name the same Tails/Gamma roles used by the existing game modules. Shared
story_flags constants expose these numeric indices without creating another owner.
The live Metal flag store remains in metal_sonic_enabled, visible through Progress.

SDK SADXStructsNew.h:4351 declares SAVE_DATA::flgCompletedActionStage[8][43].
StageCompletions names the same 344-byte live array and kStagesPerCharacter names
its stride. Persistent Data retains its exact nested std::array type. Bootstrap,
load/capture views and all maintained menu consumers borrow that same shape.
SDK MissionFlagBits_Complete (SADXEnums.h:494-505) is bit 7. The separate thresholds
130 emblems and 20 emblems/missions remain source literals with policy names.

ADVERTISE_WORK::MainMenuSelectedMode (SADXStructsNew.h:2267) is the borrowed byte
previously described as tutorial_variant in Progress. Progress now calls it
selection_variant. Native flow still supplies its existing byte; no owning field
or lifetime changes. MetalSelectionVariant and RegularCharactersOnlyVariant name
verified predicate roles, without claiming every imported variant's wider meaning.
UnknownCharacter remains an explicit unsupported roster error. The unchanged probe
maps it to the existing marker and the original judge checks the unspecified incoming
register there; game code does not invent or transport that register's value.

The trial-stage final read intentionally uses the current menu character, while
other reads use the requested character. Byte counts still wrap as before, stage
catalog ordering and lazy nonzero tests are unchanged, and unlock override still
precedes unsupported-character rejection. No algorithm was moved into reference.

Fresh pre-edit baseline is frozen in menu-progress-baseline. The two predicate
probes were built from packet source; consumer probes initially borrowed existing
Main libraries. Seven current probes rebuild all eight affected/header-dependent
game objects and borrow only unchanged Root library dependencies. Six existing
unit programs pass. No judge, expected value, fixture, tolerance or oracle input
was edited. Character Select and title retain 733 and 270 FP partial cases; full
native/original output digests and exact partial identities match their baselines.
No completed system, GPU, actor, new journey or gameplay performance claim.
