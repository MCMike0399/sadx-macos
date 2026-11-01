# Additional MPEG timing qualification

The existing unit covers a single frame rate and three rejection categories.
The fresh unchanged-source baseline in baseline.json freezes 30,657 synthetic
byte/result cases (7,774,713 full observation bytes), covering all replacement
bytes/offsets of the seed stream, every truncation, all tags/rate nibbles,
PTS boundaries/order, short payloads, invalid headers and stopping rules.
The new unit's frozen size/FNV comes exclusively from that pre-edit run; its
--capture mode emits every byte and field for full before/current comparison.
No existing unit, fixture, expectation, tolerance or case is modified.

The additional actual HleDshow unit calls the registered COM traps in a guest
process with synthetic files only. It checks all eight rates, exact binary64
outputs, absent duration before load, completion one tick before and at duration,
once-only completion, clamping, pause/stop/reload reset, the explicit splitter
assumption and parser failure with the actual AddSourceFilter error prefix.
It is additive reference integration coverage, not decoder/Windows parity.
Fresh pre-source baseline commands all exit zero, including the existing unit
with the read-only local movie. The consumer links unchanged Main support
archives; HleDshow and MPEG source themselves were compiled fresh directly.

Both new tests have manifest registrations. The contract target lists both
source/header plus fixture; the HLE target lists fixture/header and archives
whose existing dependency files track their source. No probe or stale source
copy is used. All 53 inherited raw files were copied byte-identically from the
local Main checkout into this private checkout, with expected hashes recorded in
restored-raw.json; no transport, provider or external storage service was used.
Full gate remains Root's task. Later compiled faults must discriminate the
frozen identities; compile errors cannot count as caught faults.
