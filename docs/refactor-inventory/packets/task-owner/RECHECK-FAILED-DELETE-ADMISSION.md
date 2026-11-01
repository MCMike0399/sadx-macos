# Failed deletion replay admission: independent narrow recheck

**PASS for the admission correction: prior P2 closed; no new findings.**
Candidate `1c15c5d2f77264a409b1d827686a23012ebd6cce`, before
`d48f80e6062cc78b23a5c77400e1676ebb811d2e`. Prior review
`3a7bee18bdc1e8e01cfb028f64166b26f990def7` was imported as
`9140e856c4378d37813b01e5de71cc9194308e0d`.
Read applicable AGENTS/shared memory, STYLE §12 and the prior report. Reviewed
the complete current replay helper and test module, their full diff from the
before commit, and the exact proposal. Performed this recheck without delegates.

## Standards

No findings in this bounded correction. The guard is explicit and local to the
existing admission boundary; comparison, failure accounting and game source are
unchanged. The normal comparator still requires exit 0; its separate pinned
failed-deletion reader still requires exit 1. Its complete file is byte-identical
to the version accepted by the prior structural review. No broader source
conformity or whole-module campaign closure follows from this recheck.

## Spec and fake checks

No findings. `admitted()` first verifies exact commit identity and committed
bytes, then requires `admission_kind == 'root_native_delete_replay_v1'` and
`root_admitted is True` before returning to `main()`. Missing fields, false,
null, integer 1 and string `true` cannot pass. This precedes contracts, output
creation and library loading. The existing positive fake committed-admission
test also passes and still rejects changed working bytes.

The new complete `main()` regression uses real `admitted()`, fake committed JSON,
pins and contracts, and intercepted `Path.mkdir`/`ctypes.CDLL`. All seven
non-admission variants reject with the affirmative-admission error and assert
zero contract, mkdir and CDLL calls. The retained expected-red log SHA-256
`4bda9f99988c5d3c1e8db7cfdc9dd8e73d3eaae4665bd28ce2bda810728a3acb`
matches the proposal; it contains seven `Output creation reached` assertion
failures against the old helper. That log was parsed, not rerun.

Ran from this review worktree using Main's exact interpreter:
`/Users/burbujamc/Developer/sadx-macos/reference/python/bin/python -B -m unittest discover -s tests/python -p 'test_task_*.py' -q`.
**95 tests passed, exit 0**, including 21 replay fake tests. The seven variants
are subtests of one test. `git diff --check d48f80e...1c15c5d` passed.

Read-only integrity checks verified all eleven proposal pins, including the
unloaded candidate library and interpreter. Reviewed files match both candidate
git blobs and Main bytes. The prior focused-log hash also matches. The proposal
still says PROPOSED, deliberately lacks both affirmative fields, and therefore
does not admit execution. The actual proposed output directory remains absent.

| Complete reviewed file | SHA-256 |
| --- | --- |
| `reference/packets/task-delete-native-replay/replay.py` | `27d73c9af213f073ab2de9c5e8193aa57c14e14830991c8461f7ab3c57cd5089` |
| `tests/python/test_task_delete_native_replay.py` | `6378464aee9f1d2f592c1c0df67d32f2f648332bf15ac420c00d0f3f59478179` |
| `docs/refactor-inventory/packets/task-owner/DELETE-NATIVE-REPLAY-PROPOSAL.json` | `26e18c67d26111b6755e34878898fdc5398f87c2387081d31545d0466b69e808` |
| Unchanged `tools/oracles/task_judges/compare_streams.py` | `38d882f2fad2711584f37ea9b129e52f4b6ddaee9d14c758aecb6652be4485f1` |

## Preserved limits

Read-only JSON parsing confirms 570 original attempts (314 returns, 256 stops),
563 captured calls with oracle exit 1, and the same seven exclusions:
85, 222, 238, 242, 361, 502, 562. Case 207 retains all eleven source-differing
words and remains source FAIL. The continuation remains terminal EXIT 1.
Candidate deletion's full cohort remains unattempted except the separately
admitted case 207 diagnostic; no new exclusion or qualification is granted.

No native/original/game execution or game-library load; no reserved generation
or input changes, build, mutation, gate, provider/network action, Source edit,
or actual replay artifact creation. Source72 was not re-reviewed. Only this
report is committed on the review packet branch; no Main edits or push.
Root retains distinct committed admission, native-only 563-call replay, final
preservation/source-FAIL accounting, manifest union, Task merge, source-only
closure, journeys and full gate. **This review grants no execution approval.**
