> Historical candidate description for 41e3fc3/c306a21, blocked by independent review 9c9fb0f.
> Corrected behavior and tests are recorded in review-fixes.json. In particular, unresolved
> callable tokens are now report-only and source archive validation is supported.

# Signature dispatch v1 judge candidate

Base: c55762db5a42ad77b56fbaa8617ccfead4dbf0e1. This packet changes no
source behavior, old readability measurement, old allowance, inventory generator,
generated inventory, HANDOFF, STATUS, gate registration or judge manifest.

The supplied `checker-gaps-before.json` demonstrates Sonic's missed fourth target
argument. The separate `check_address_dispatch.py` now uses 11 file-scoped helper
contracts in `address_dispatch_signatures_v1.json`. Each entry includes the exact
existing declaration/body, permitted receivers, argument count bounds, one-based
target position and source forwarding evidence. Sonic's `a` becomes
`SonicPlayerCall.address` (`sonic_player.hpp:78`); `b` becomes `call_site`.
Other entries follow named call records into `invoke`, rather than inferring an
address from its magnitude or from the spelling `call`. There is no currently
recovered first-target helper in this registry; the first-target contract is
covered by a synthetic forwarding fixture. Unregistered first-target `call` is a
candidate, not proven dispatch. The old heuristic remains independently enforced.

The new measurement finds 113 sites, including 41 in Sonic versus zero in the old
Sonic `address_calls`. These are measurement changes on unchanged source, not
source regressions. A target variable, decimal, constexpr alias or ternary remains
a dispatch site. The diagnostic site and color/mask argument are not target proof.
The old measurements remain exactly unchanged: 495 files and 61 `address_calls`.
The companion JSON records per-file counts, source findings, prior hashes and
focused check results. No full gate, build, oracle, provider or formal review ran.

## Registry maintenance and conservative candidate guard

A registered definition is checked even if its file disappears. Changes, moves,
renames, duplicate definitions, unsupported arity, additional same-name ordinary
bodies and lambdas fail for explicit review. Receiver spellings are restricted to
the existing unqualified calls and the evidenced `op.call` uses. Comment/raw-string
copies cannot stand in for a definition. This is a lexical file-scoped contract,
not overload or type resolution; new unsupported same-name declarations, macros,
conditional branches and unusual receivers require manual review.

Every other callable-name token is explicitly an **unresolved inventory review
candidate**, including declarations and ordinary semantic calls. There are 23,463
such tokens. Their per-file/name counts also ratchet. This is deliberately broad:
adding or renaming even a variable-target helper introduces a candidate name,
rather than silently disappearing because it lacks hex or a registered name.
A candidate increase reports `candidate review required`, not proven dispatch or
a source fidelity regression. Ordinary semantic helper additions also require
review. V1 offers no automatic approval/allowance expansion command; a future
reviewed, named migration is needed to expand this review surface or the registry.
This conservative guard is a review obligation, not a claim that all unregistered
helpers are semantically resolved. Existing candidate names/counts can still hide
semantic substitutions; source inventory and independent review remain essential.

## Migration safety

The separate baseline is explicitly versioned `signature-dispatch-v1`. The only
initialization is `--migrate-signature-v1`, which reads **all immutable HEAD
sources**, including tracked files absent from any old allowance file. It does
not seed from dirty or new files. It preserves those HEAD counts exactly even if
the dirty source has already reduced debt. It compares any working baseline to
that immutable seed before writing. After the baseline is committed, HEAD's
allowances and registry digest govern; `--write-baseline` can only lower counts.
Reusing the migration flag cannot reinitialize an existing committed baseline.
Registry changes require a separately implemented named migration, not a refresh.
A Git checkout is required; source-archive support is intentionally not added here.
Existing schema-1-to-2 readability migration tests remain unmodified and pass.

The candidate baseline is 324 KiB of per-file/name accounting, mostly unresolved
candidates. Its size reflects the conservative review guard, not 23,463 alleged
address calls. None of its allowances replace the schema-2 readability allowances.

## Lexical limits

The analyzer masks comments/ordinary/raw strings with the existing masker, then
balances parentheses, brackets and braces for registered calls. Each invocation
is counted once; nested invocations get their own site. Bounds are 65,536
characters per registered invocation and 128 delimiter levels; crossing a bound
fails. It does not expand macros, preprocess C++, resolve aliases/types/overloads,
or parse template angle brackets. Top-level template commas can make argument
positions ambiguous; parenthesize such expressions and review them before adding
a contract. The current registered call sites do not use such target forms.
Callable tokens use identifier-followed-by-parenthesis syntax; indirect pointer
invocations and template-id callees can escape this candidate surface. The guard
therefore does not establish semantic completeness. No table classification or
scalar constexpr exemption changes are included.

## Safe Root integration

1. Review/cherry-pick this judge-only commit on top of the Root baseline. Verify
   the schema-2 checker and `reference/readability-baseline.json` are unchanged.
   The new baseline was measured from c55762d HEAD, not the edited worktree.
2. Run both focused fixture suites and both checker CLIs. After cherry-pick the
   new baseline is committed, so run `python3 tools/gate/check_address_dispatch.py`
   without a migration flag. Investigate any drift; do not seed from Root's edited
   source or manually raise allowances. Existing-source recognition increases
   need another named immutable-HEAD migration with explicit evidence.
3. Independently review the signature evidence and candidate-guard tradeoff.
   Root owns any inventory regeneration, fixture/gate registration, manifest
   protection of the new checker/registry/baseline/tests, final manifest refresh,
   full gate and integration receipts. This packet is not gate-integrated or
   manifest-current and does not claim either. The old checker remains required.
