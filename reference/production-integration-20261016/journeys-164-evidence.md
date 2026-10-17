# GOAL 6.1: journey and judge wiring

Baseline: 6fec622. `check_judge.py --update --additions-only` exits 2 because
that flag is not implemented. The existing runner starts four windowed app
processes, so obscuring their windows stops progress. R2/R3 are outside the gate.
Four built unit programs have no manifest rows. Corpus file rules depend only
on the oracle script, so a newer native probe leaves replay data stale.

This judge-only change preserves existing frame thresholds and guest digests,
adds synthetic frame/manifest fixtures, and uses one offscreen app with scripted
checkpoint captures. No game source or fidelity policy is changed.

The first `make oracle-game-reset-composed` exited 2 (`Native rejected`). The
native probe reads 0x3a7cf3c and 0x38f6ec0 but neither containing page was in
its twelve-page Memory map. Its `memcpy` therefore has undefined behavior;
`otool -tvV` shows the complete exported body optimized to `mov w0, #0; ret`.
The judge fix adds those two pages to both sides and compares all fourteen.
The original twelve randomized pages and all 300 failure indices are preserved;
additional page bytes use an independent RNG. Game source is unchanged.

Pre-gate verification:

- One offscreen 2,000-frame route: R1/R2/R3 pass; R4 open with 101 skipped
  frames (FVF 0x142), 2 colours and no textured draws. Duration 232.63 s.
  Character-select capture was visually inspected. Local signatures were blessed
  for the three passing checkpoints; the gate rerun will compare them.
- All 64 Python tests pass using the documented PYTHONPATH. Fixtures cover
  additions with dirty/staged source, old hash edits/removals, forged mode metadata,
  registration prefix edits/duplicates/Make overrides, frame properties, golden
  differences, missing/stale captures, and timed gate-log parsing.
- Removing the new probe prerequisite deliberately makes the corpus dependency
  fixture fail. Restoring it passes. All 150 unit sources now have manifest rows.
- All four formerly unregistered unit programs pass: Metal sprite 16,384 channels,
  texture 136 records / 144 levels / 2,094,420 channels, queue-to-GPU composition,
  and player child-state factory. These are Apple GPU checks, not Windows parity.
- Reset composition: 300 cases, zero mismatches, pass. A historical/current fixture
  comparison proves all 14,745,600 old bytes and all 300 failure indices unchanged.
  Two added pages are compared too. No tolerances or comparison fields were removed.
- Layer, readability and hygiene checks pass; game source remains unchanged.

The full gate result and remaining scope are recorded separately in the gate
record and HANDOFF. No original imagery, assets, saves or binaries are committed.
