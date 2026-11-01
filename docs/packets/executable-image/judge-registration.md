# Registration evidence and pending seal

The frozen baseline test was added in 85def98. Before the source rewrite, its
65,630 valid checks passed and LB-1 failed (old source incorrectly returned an
image). The complete module Source commit is fc4be98. No existing test,
expectation, tolerance, fixture, corpus or judge implementation is changed.

This separate judge step appends two registrations and two explicit direct
compiler recipes. Both `make -j1 build/executable-image-test
build/executable-image-edges-test` targets build and execute successfully. The
actual generator emits both run-unit-tests commands. Unsupported CLI options
return 1. Synthetic tests need no assets, archives, guest runtime or downloads.
Qualification records exact commands, current source/test hashes, sanitizers
and six compiled faults caught by the unchanged expected results. The edge
suite additionally matches 11 valid checks on the exact base header/body.

Manifest refresh is deliberately pending Root. Its 943 inherited entries are
unchanged; this worktree lacks all 53 ignored raw corpora. The only changed old
protected files are Makefile and tests/unit/manifest.tsv, both prefix-preserving
append edits. Root must preserve every inherited entry, restore all 53 raw
corpora byte-identically, include both new unit sources, rehash the two reviewed
registration files and seal an explicit parent-preserving union. Never replace
that union with discovery from this incomplete worktree. Full gate, independent
review, inventory/module classification and Main integration remain Root work.
