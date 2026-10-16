# Judge changes

`make check` verifies `reference/judge-manifest.json`. Its SHA-256 entries cover oracle and
gate tools, their shared `tools/lib` helpers, probes, fixtures, Python checker tests,
oracle summaries, frozen corpora, mutant
specifications, the Makefile and unit-test registration. The checker itself is covered.
Generated Python caches and `.DS_Store` are excluded. Regenerated `build/oracles/*.cases.bin`
files are development outputs, not frozen corpora. New frozen corpora belong under
`reference/frozen-corpora/`; top-level `reference/*corpus*` files are also covered.

Changed, added and removed inputs fail the check. A manifest refresh records its reason,
evidence-file hash and base commit. In Git checkouts, refreshing the manifest while any
`src/` file differs from HEAD is rejected, including staged and untracked source. Source
archives still verify the supplied hashes; commit separation requires Git history.

## A judge step

1. Establish the judge defect or new verification requirement; save the evidence in a file.
   For example, a fixture must fail against the old checker and pass after its correction.
2. Start with game-source changes committed separately. When the optional hook is enabled,
   declare this step before editing protected inputs:

   ```sh
   python3 tools/gate/check_judge.py --declare-change \
     --reason "Describe the judge defect and intended correction" --evidence build/judge-evidence.txt
   ```

   The ignored `.claude/judge-change.json` records the current HEAD and evidence hash. It
   expires when HEAD or the evidence changes. Declaration documents intent; it is not user
   authorization to change the project's fidelity policy or milestone requirements.
3. Make the judge change and verify its fixtures, including a deliberate failing mutation.
   Save a durable evidence summary under `reference/production-integration-<date>/`.
4. Refresh explicitly, pointing to the saved evidence:

   ```sh
   python3 tools/gate/check_judge.py --update \
     --reason "Describe the verified correction" \
     --evidence reference/production-integration-<date>/judge-evidence.json
   make -j10 gate
   ```

   Completion: the full gate exits zero and `check_judge.py` reports no drift. Record the
   gate and regenerate STATUS using GOAL §7. Commit the judge, manifest and evidence in a
   separate `fix(judge): …` commit. A port and its report-summary refresh therefore require
   separate commits: verify and commit the port first, then update tracked oracle summaries
   and the manifest as an evidenced judge step. The raw oracle results stay in `build/`.

## Proposed edit hook (approval pending)

`reference/judge-hook-settings.proposed.json` is the complete proposed project settings file;
it is inactive. With approval, install its content as `.claude/settings.json` (merge the
hook if settings have since appeared). It matches Claude Code `Edit` and `Write` calls,
runs `tools/gate/guard_judge_edit.py`, and refuses protected edits until a declaration for
the current HEAD has unchanged evidence and no mixed game-source changes. Its path check
uses both lexical and resolved paths. Worktree identity follows the hook payload's `cwd`.
Malformed input is rejected. Unprotected edits retain the normal permission flow.

The proposal uses the command-hook input/exit protocol and `onFailure: block` described in
the [official Claude Code hook reference](https://code.claude.com/docs/en/hooks). The installed
CLI was 2.1.301 when prepared; `onFailure` requires 2.1.295 or later. Direct protocol fixtures
are verified separately from live activation in Claude Code, which remains unverified until
the approved settings are loaded.

This provides tamper evidence and an editing guard, not a security boundary against a
process that can rewrite both the judge and its hashes. The hook covers Claude Edit/Write;
shell writes, other coding harnesses and policy changes still depend on the separate-commit
workflow, gate and evidence review. The manifest does not certify the existing oracle
verdicts as correct or passing; STATUS keeps pass/partial/fail distinct.
