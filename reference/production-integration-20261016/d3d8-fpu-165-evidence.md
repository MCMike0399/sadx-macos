# GOAL 6.2 / ADR-0008: precision census judge refresh

Source correction: d0c039a. Microsoft documentation and Windows capture are
cited in d3d8-fpu-source-evidence.md. Source tests and a deliberate wrong-flag
mutation pass/fail as expected. Reset still reports its resource-recreation STUB.

The 120-frame smoke census matches plain/original/native traces but has mixed
0x023f/0x007f observations for six shared startup functions. The collector must
retain those samples and distinguish calls before/after device creation rather
than claim every entry is single precision. The full census adds the already
verified menu-to-Station-Square input route, without claiming R4 is drawn or R5
is complete. No existing oracle tolerance, fixture or digest expectation changes.

Full results before the gate:

- Nine runs: intro 7,228 frames, controller 900, Station Square 2,150, each
  plain/original/native. All six trace comparisons pass; all three variants also
  have identical stage transitions. Stage 26 begins at frame 1,895.
- Every post-device observation is CW 0x007f: intro original 345,983 calls;
  controller original 580,549; stage original 9,791,330 across 159 reached sites.
  Native stage has 8,023,016 observations across 148 sites, all CW 0x007f.
  The original intro retains 69 pre-device CW 0x023f observations. Zero coverage
  stays explicit; no claim about unobserved call sites or Windows stage parity.
- Paired before/after captures restore only the old HLE D3D8 object while sharing
  every other guest/core object. Intro/controller/stage and native first 600 all
  first change their stream digest at frame 390. Clock arrays are byte-identical.
  Full old/new hashes and binary identities: d3d8-fpu-digest-delta.json.
- Guest frame/input/controller tests compare runs dynamically; they have no frozen
  digest constants to change. Their comparisons and timing expectations stay intact.
  The census's recorded hashes are refreshed under ADR-0008. Local journey golden
  signatures are retained for the gate; none has been blessed for this fix.
- Fog (4,700 cases, partial), ground acceleration (29,952 cases, partial) and frame
  control (2,672 cases, pass) reproduce their entire pre-fix report files byte for
  byte, including all flags and case hashes. All 65 Python tests pass. The FPU
  preserve-bit mutation is caught; Reset still stops at its lifecycle boundary.
