# Direct source dependencies for the additional consumer test

Main support archives changed concurrently during private qualification. The
pre-source consumer transcript and current transcript were still identical, but
archive hashes observed later no longer matched. This does not invalidate the
pre-source parser/corpus baseline, which compiles only this owner directly.

Pinned private support copies now link both immutable old owner source and
current owner source plus freshly compiled unchanged HleDshow. Both complete
10,873-byte consumer transcripts are identical to each other and to the actual
pre-source transcript. This is an additional retrospective old-source replay,
not a claim that the pinned replay occurred before the rewrite.

Make's added consumer rule should express the same direct source build: compile
HleDshow and MPEG timing explicitly, and use archives for unchanged support only.
This prevents a private prebuilt support archive from selecting an old parser.
It also lists both .cpp files and the parser header as direct prerequisites.
Only this packet's new target is changed; all inherited Makefile bytes and
registrations retain their exact prefix. No expected values or inputs change.
