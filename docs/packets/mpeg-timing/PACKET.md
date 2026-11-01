# MPEG timing readability packet

Branch: `packet/mpeg-timing-readability`.
Worktree: `/Users/burbujamc/.t3/worktrees/sadx-macos/t3-2b862ddd`.
Base: Main `a0aa342`. Source: `537691b`.

Both complete owner files are rewritten: 67 old lines become 172 maintained lines.
Public API, aggregate field order/defaults and behavior are preserved. Named format
constants and packet/timestamp/sequence steps replace compact multi-statement
flow. MPEG tags/masks/rate fractions remain format values. No game behavior moved
layers; no native player/AVFoundation clock, PE image, heap or other source changed.

HleDshow's metadata assumption stays explicit. The parser still handles only the
existing MPEG-1 subset and E0 video. It keeps permissive marker decoding, STD and
PTS+DTS skipping, first usable sequence size/rate, invalid-header search stopping,
min/max PTS without unwrapping, separately truncated integer duration terms,
no-picture/zero-dimension acceptance, short-tail/end-code handling, error order,
exact diagnostic strings, null error pointer and untouched success error text.
There is no frame count: video_packets_with_pts counts qualifying packets only.

## Durable sequence

- Context `36bd0ef`: complete ownership/boundary reasoning and original hashes.
- Fresh pre-source baseline `6831a56`: unchanged existing unit with read-only
  RE-US.MPG, 30,657 synthetic cases and actual HleDshow consumer calls all exit 0.
- Additive judge `fd24501`: synthetic fixture, frozen full-output contract,
  actual COM consumer test, registrations and complete parent-preserving manifest.
- Source `537691b`: only the two MPEG owner files.
- Judge dependency correction `46719e5`: added consumer target compiles both
  MPEG timing and actual HleDshow directly; support archives cannot select old code.
- Qualification `4f003a9`: full byte comparisons and ten discriminating compiled
  faults; source hashes, complete fault substitutions and first divergences saved.
- Registration normalization `46d2286`: remove empty trailing tabs in only two
  new rows; same tests/arguments, unchanged inherited registration prefix.

## Results and evidence

`baseline.json`, `current.json` and `qualification-faults.json` carry exact hashes,
exits, bytes and limits. All 7,774,713 bytes of synthetic inputs/results match
before/current, including optional presence, all fields and both error-pointer
modes. The default new unit freezes size and FNV; `--capture` emits the entire
stream. No existing tests, expected cases, tolerances or fixtures were changed.

Actual HleDshow COM calls cover eight rates and exact binary64 duration/position,
missing duration before load, one tick before/at completion, one-shot EC_COMPLETE
with exact parameters, position clamp, pause/stop/reload reset, existing assumption
and actual AddSourceFilter failure prefix. Its full 10,873-byte transcript matches.
The existing unit still reads 3,206 PTS packets at 640x480 and 107540 ms from the
local RE-US.MPG, with unchanged assertions. No asset bytes are committed.

Ten faults each compile and finish all 30,657 observations, and each fails the
unchanged frozen contract: timestamp bits, rate fraction, packet count, range max,
invalid header stopping, E0 filtering, pack marker rejection, frame period,
sequence width and failure precedence. Full row differences range from 16 to
25,409; no compile failure counts as a caught fault. ASan/UBSan passes the full
contract. clang-format and git diff --check pass.

All 946 protected inputs check with zero violations; all 945 base paths survive.
Only inherited Makefile and unit manifest hashes change through appended targets
and rows, with exact old prefixes. All 53 inherited raw inputs were restored from
local Main with exact expected hashes (`restored-raw.json`). No incomplete refresh.
The inventory generator discovers 287 unit programs and 231 oracle targets.

## Reproduction and integration

Normal full-build test targets:

```sh
make build/mpeg-timing-contract-test build/mpeg-timing-hle-test
build/mpeg-timing-contract-test
build/mpeg-timing-hle-test
build/mpeg-timing-contract-test --capture > build/mpeg-timing/replayed.txt
python3 tools/gate/check_judge.py
```

Private qualification reused local Main archives for unchanged guest/core support;
this is not a clean whole-guest build. Main rebuilt its archives concurrently.
The original pre-source transcript remains frozen; an additional retrospective
replay links immutable old source and candidate with the SAME pinned support
copies, both byte-identical to the initial transcript. See judge-dependencies.md
and current.json. The pinned old-source replay is not called pre-edit evidence.

For a narrow private reproduction with these pinned local copies already present:

```sh
make -o build/libsadx-core.a -o build/libsadx-guest.a \
  build/mpeg-timing-hle-test \
  UNICORN_DIR=/Users/burbujamc/Developer/sadx-macos/reference/python/lib/python3.14/site-packages/unicorn
```

Direct contract build is asset-free. For old-source replay, extract both owner
files from `a0aa342` under an ignored private directory; put its `src` include
first, and compile the same new unit against its `.cpp`. Fault replay replaces
exactly one `replace.before` string from qualification-faults.json with its
`replace.after`, compiles that source against the unchanged contract unit, runs
`--capture`, compares every row with the frozen old capture, and requires default
exit 1. Keep all generated sources/executables/captures ignored.

Root must independently review source and evidence against STYLE §12, run its
reserved holdout if required, reconcile the two-file inventory disposition,
integrate with a parent-preserving protected union, and run the combined gate
and native/reference journeys. No full gate, journeys, provider/network/download,
external storage operation, delegate, Main write or push occurred here. This
packet proves bounded reference metadata preservation; it does not establish
Windows splitter/codec parity, native movie clock qualification, whole Loader
conformance or complete refactor campaign closure.
