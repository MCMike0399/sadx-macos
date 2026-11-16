# Gamma gun-effects and shadow holdout audit

Branch `packet/holdout-audit`, base Main `d8ef40e1`.

## Audit of the prior evidence

Neither `reference/oracle-reports/gamma_gun_effects.json` nor
`reference/oracle-reports/gamma_shadow.json` existed before this packet.
`docs/STATUS.md` recorded both slugs as "no tracked report", and the judge
manifest only carried hashes for the scripts, probes and unit tests. The gate
records in `reference/production-integration-20261109/` carry those same script
hashes, not oracle results.

The old scripts generated one cohort: `fixtures(count, seed)` always prefixed
the twelve gun or eight shadow curated cases and then appended seeded random
cases. `holdout_index` was only `len(cases) // 2` (38 of 76 for gun, 36 of 72
for shadow), and the single loop executed every case under the same seed. The
gun report also stored `holdout_mode`, which was just the mode of the case at
that midpoint. No holdout seed, separate cohort, or holdout-only run existed.
The only establishable coverage is therefore development-only: curated cases
plus the seeded random cases, all in one cohort.

The tracked partial summary often conflated with this slug is
`reference/oracle-reports/gamma_effects.json` (1145 cases, zero value failures,
559 FP partials). It belongs to `verify_original_gamma_effects.py`, which runs
the `0x49fb10` effects body and compares x87 flags. It is not the gun-effects
oracle. The gun and shadow oracles do not compare x87 status flags, so their
reports have no FP flag partial count to preserve.

The packet mutant records `gamma-gun-effects-mutants.json` and
`gamma-shadow-mutants.json` contain six mutation definitions each and no run
receipts. The 2026-11-09 handoff's 76/72 case, 6-of-6 mutant claim is not
supported by a tracked report.

## Change

Both scripts now follow `verify_original_player_underwater.py`:

* `--cases` and `--seed` drive the development cohort, curated cases included.
* `--holdout-cases` (default 64) and `--holdout-seed` drive a holdout cohort
  generated with `curated=False`, so curated cases are excluded.
* The JSON report has a distinct `holdout` block with its case, mismatch and
  fault counts and seed. `passed` and `values_match` require both cohorts clean.
* `samples` keeps the per-case `index`, `case`, `native`, `original`, `fault`,
  `completed` and `ok` fields, adds a `cohort` label, and now holds both cohorts.
* Curated cases, random distributions and comparisons are unchanged. The only
  new seeds are gun `0x4c38d0` and shadow `0x49e920`; development seeds remain
  `0x4c44a0` and `0x49ee30`.

## Verification

Pinned executable: `/Users/burbujamc/Developer/sadx-macos/game/Sonic Adventure DX/sonic.exe`,
SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The isolated worktree has no `game/` directory, so the make runs overrode
`GAME_EXE` with that pinned copy and `REFERENCE_PYTHON` with the main checkout's
reference venv. The probes built in this worktree.

Development runs, each with `--holdout-cases 0`:

* gun: exit 0, 76 development cases, 0 mismatches, 0 faults.
* shadow: exit 0, 72 development cases, 0 mismatches, 0 faults.

Final runs, each once with defaults through the make targets:

* `make oracle-gamma-gun-effects`: exit 0, 76 development cases (0 mismatches,
  0 faults) and 64 holdout cases (0 mismatches, 0 faults), holdout seed
  `0x4c38d0`.
* `make oracle-gamma-shadow`: exit 0, 72 development cases (0 mismatches,
  0 faults) and 64 holdout cases (0 mismatches, 0 faults), holdout seed
  `0x49e920`.
* `verdict.py` reports `pass` for both tracked reports.

## Judge inputs changed

* `tools/oracles/verify_original_gamma_gun_effects.py`
* `tools/oracles/verify_original_gamma_shadow.py`
* `reference/oracle-reports/gamma_gun_effects.json` (new)
* `reference/oracle-reports/gamma_shadow.json` (new)

The judge manifest was not edited. Its 53 pre-existing missing `.cases.bin`
inputs remain; this packet adds no further registration work beyond the four
paths above.

## Not run

No gate, no mutant run, no frontier and no status regeneration. The old handoff
claim remains a claim; only the new development and holdout runs are receipts.
