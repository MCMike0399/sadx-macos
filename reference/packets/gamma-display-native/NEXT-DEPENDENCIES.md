# Actual display continuation contracts

This is source analysis, not proof of which branch the first normal-input display
will take. The heavy hold prevents the required new actual-caller measurement.

The complete pinned body is 47fd50–480722 inclusive. The current draft implements
its topology and placement prefix, stopping before the first animated model draw.
The full dossier, assembly and profile context are retained in this worktree.

The object writes are sibling links at NJS_OBJECT +48. After each store the source
reloads the table global. The first upgraded branch reads destination row 22
before row 16, whereas the ordinary branch reads row 14 before row 22. The second
store selects row 18 or 20, then destination row 23. The final reload follows row
22 → sibling → child and writes GammaWeldInfo[0].source_object at 3c5455c. This
must remain the same row consumed by canonical PJoinVertexes.

After palette 4, the source initially reads the requested animation, replacing the
local with current animation only in transition mode 2. Its status bit 2 guard
skips the body but still calls ResetMaterial, LoadControl3D and palette 0, then
conditionally Shadow when ChkPause is nonzero. That path remains a named stop.

The ordinary body uses the existing texture list E102, SaveControl3D,
njControl3D(16), SetMaterial(1,1,1,1), SaveConstantAttr and OffConstantAttr(0,524288).
Matrix placement borrows the existing Display stack and exact Ninja primitives;
its temporary frame uses the unit matrix and captures a local vertical offset.
The source publishes two collision centers before subsequent body rotations. The
four scalar additions retain ExactFloat until a stored binary32, conservatively;
no FP equivalence or simpler-arithmetic claim has been qualified yet.

Requests 46 and 52 with ChkPause == 0 use the separate walk-sway arithmetic path.
The ordinary reached request resets E102WK.wrun_mov_yang to zero. The sway path is
explicitly unimplemented rather than reset or bypassed for those animations.

The event/action dispatch then reads primary.eventwk.action.list at +64. A null
event allocation is a fault, not an absent list. An empty list uses the ordinary
animation renderer: install callback 47f990, choose player animation table vs
its generated transition action, and call ds_ActionClipNoScale at 4800de. That
wrapper sets model callback 403470 and invokes 4053d0; the full animated traversal
and Gamma object callback must be recovered/composed before claiming a draw.
Clearing the callback after a failed draw would not match the source prefix.

A nonempty event list first calls helper 47ef40. That helper requires nonnull
current and old action lists plus a nonzero current link_frames byte before it
initializes its global temporary mtnjvwk and calls PSetMotion three times around
CalcMMMatrix. The temporary block 3c545d0–3c545fb is adjacent to, not part of,
GammaWeldInfo. Its two recipe rows use the old/current event nodes, and its return
is the actual CalcMMMatrix result. The denominator is current link_frames + 1;
the numerator uses the captured event timer + 1, with source arithmetic retained.
This is a pending dedicated animation composition, not a second player/pose.

Regardless of helper return, the source calls 417fb0(primary,4), resets palette 4,
and gets matrices 54 and 30 on the appropriate side of that call. The exact
417fb0 body and its event/render ownership remain a required next dossier. Do
not replace it with the ordinary action renderer or skip the call when blending
is inactive. Active PJoin deformation and attachments remain independently open.

After the first body draw, the source always draws the arm action (export index
50) and computes two matrices. That action payload is not supplied by this
object-only append. Later fan/effect/gun/aim/headlight paths consume more assets
and mutable owners; they remain explicit dependencies. Imported animation
unavailability cannot be replaced by a static pose, identity matrix or NULL.

After Root releases the slot: compile this draft and minimal shared fixture;
run the topology-prefix judge and render-state leaf judge with faults/holdout;
qualify placement against original instructions; exercise the actual constructor
and scheduler display callback; run normal input against a private appended pack;
inspect its coherent capture and record the actual next call. Only then choose
the event vs ordinary-renderer continuation from observed state.
