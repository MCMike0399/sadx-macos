# Rollout extractor evidence

Read-only format inspection on 2026-10-26; no extraction of this session was run.
Session: `21/rollout-2026-10-21T16-05-06-01a1607f-216c-7ff3-b29e-fadbbe0b87c6.jsonl` under `~/Developer/sadx-agent-history/codex-rollouts/`.
SHA-256: `e462b5b70d66ebaaebe8c01b2d823a14ba588b2ee5a44a2202a17612e209fc9f`.

Five relevant calls inspected, identified by one-based JSONL record:

- 2177, `call_3fa029c01a8a497d97086651ac851c0c`: custom_tool_call `exec`;
  literal JS exec_command wrapper, mkdir, source cp, unrelated inspection Python heredoc.
- 2184, `call_e805512e0e404feb8805652da26ea848`: same wrapper;
  read-only shell inspections followed by a quoted cat heredoc write.
- 2198, `call_e5bc8dc9cee1477eb0071085e745dc5a`: quoted cat heredoc write.
- 2212, `call_de3fa5a1113645bd831448a61ce9baba`: quoted cat heredoc write.
- 2221, `call_d0fe3a4267224ac6971da1aecc29cc8c`: oracle/build invocation;
  selected output paths do not make this a supported authored write.

No game source excerpts or raw transcripts are committed. The tests use synthetic,
small payloads with the actual response_item/custom_tool_call and JS literal wrapper
shape. Normal function_call JSON wrappers are tested too. Tests assert exact content,
ordered overwrite/append/patch results, session/file hashes, call provenance, unsupported
evidence, literal command-substitution text, shell/Python/JS non-execution, traversal and
outside rejection, stale output and output-symlink rejection, unresolved destinations,
copy prerequisites, missing/ambiguous/overlapping patch context, and read-only exclusion.

Focused verification: `python3 -m unittest discover -s tools/agents/tests -v`:
18 tests passed, exit 0. No full gate was run or recorded. No app or game-code claim.

Integration handoff and the root extraction command are in PACKET.md. The sampled cp
alone requires an incomplete verdict without historical provenance; unsupported later
edits must remain in the manifest, rather than declaring a reconstructed game tree sound.
