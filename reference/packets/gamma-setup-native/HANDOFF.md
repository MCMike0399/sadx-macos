Gamma setup now has a readable typed body, a real imported sprite definition and an arena allocation binding.
The original comparison passes 704 development and 2,048 final holdout cases; six compiled faults are caught.
The inherited constructor passes 1,136 development and 2,048 holdout cases; all seven existing faults are caught.
Three native programs and five Python tests pass, including imported-byte inverses and real allocation exhaustion.
The real Gamma caller runs setup twice and stops at material consumption; it does not skip materials or effects.
The unchanged Gamma judge retains 1,422 cases, zero value failures and all 498 individually recorded FP partials.
No app frame, GPU output, N3b, full gate, push, reserved input or shared-owner integration is claimed.
Root needs the explicit owner, texture identity, registration and binding steps in PACKET.md, then independent review.

### 2026-11-01 — Gamma definition identity observation correction (no gate)

* Source review accepted the bounded setup body and found one P2 observer gap: all nonnull definitions were normalized to one identity. Root now maps two actual pointers explicitly and rejects unexpected definition/node identities. Game source is unchanged.
* All 704 development and 2,048 held inputs are preserved, plus 144 cases each with a second definition and reentrant replacement. Expanded 848 development and 2,192 retained-holdout cases pass; six prior and one new wrong-definition mutation are caught.
* The wrong-definition mutation produces 32 mismatches in original inputs, all hidden by the previous nonnull projection. The old receipts/probes remain intact; review-correction.json binds the supplement.
* No new app run, gate, Main merge or N3b claim. Fresh correction review precedes canonical owner/resource binding and actual native frontier verification.
