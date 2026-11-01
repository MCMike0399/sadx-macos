# Authored module closure

File conformance never automatically closes a module. The optional `modules` map
in review.json uses the exact generated module identity as its key. A reviewed
entry supplies disposition, reason, tracked evidence, and reviewed_members: an
exact path-to-SHA256 map of every current member. Added, renamed, deleted or
changed members require fresh whole-module review rather than inheriting approval.

Conforming modules also require semantic_justification, resolved_owners,
resolved_consumers, accepted_judges and recorded classification_limits. Every
member must already have its current semantic file review. Reference-purpose and
obsolete module classifications require matching member dispositions; removal
also needs the existing explicit obsolescence proof. Unknown module IDs and
missing tracked evidence fail. Broader runtime/system/campaign claims remain
separate and cannot be inferred from this module record.

This guard stores authored evidence; it does not prove C++ semantics, infer
closure from token counts or invalidate a stable module whenever an unrelated
caller is added. New callers remain individually inventoried/reviewed, and a
review's accepted consumer scope and limitations stay explicit. No module is
closed by this tooling setup. Seven focused file/module projection tests cover
changed/widened/renamed membership, incomplete semantic/member reviews, missing
judges and purpose exceptions. All existing ratchets and scope paths remain.
