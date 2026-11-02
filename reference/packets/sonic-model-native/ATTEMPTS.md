# Retained setup observations

* First Objective-C++ app compile rejected two `auto` declarations deduced as
  Objective-C `id`; both were changed to explicit `id`. Source/native builds pass.
* The first new source-site fixture used the stack after the source's pending
  PInitialize argument cleanup. At 49ab58 ESP still includes eight argument bytes;
  the fixture now sets that exact entry state. Its invalid task read at 41caf5
  was a fixture setup failure, not a game mismatch or an old-judge edit.
* The first callback expectation assumed slot6's sibling differed from slot5.
  The actual imported source object already links slot5. Source-backed cases
  now cover both the true first callback and an explicit changed-sibling state.
* The first consumer assertion compared a known-null optional object reference
  to numeric zero. The existing view correctly represents null as no id; the
  assertion now compares its value_or(0). No consumer implementation changed.
* The old narrow numeric build helper omitted CharacterPack::ObjectState's
  already-admitted provider. The first reference link failed accordingly. The
  successful second build appends character_pack.cpp to that helper's source
  list in memory, without editing any old build/verifier input. Complete old
  defaults, partial identities and extras were checked against that build.
* New source comparisons retain 34 flag-only partials per cohort from the
  reached collision arithmetic. No tolerance, source flag or expected value
  was normalized. All eight compiled faults fail their intended observations.
