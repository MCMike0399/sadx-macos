# Explicit active-processing boundary

This packet recovers a PJoinVertexes prefix, not complete welding. First-call
initialization and the separate backup restoration branch are implemented.
Join mode 1 stops at `native_gamma_join_vertices_CalcMMMatrix`, before consuming
the join table. An empty/null join table must not hide that missing producer.
The mode/action/frame selection immediately before the source call and every
active side effect remain unqualified; the current stop does not claim their
argument comparison. No active matrix or attachment values are supplied.

The owned recovery examined more than the first callee. SOURCE-EVIDENCE.json
pins complete GetNodeIndex/helper dossiers, CalcMMMatrix, GetMMMatrix and the
recursive motion-matrix producer. Their required composition is:

* CalcMMMatrix sets the unsigned limit to requested object count plus 1,
  clamps it to 128, clears the output count, pushes the existing matrix and
  optionally copies the supplied base matrix. PJoin supplies the existing unit
  matrix. It takes the requested row's count in both animation branches; motion
  mode 2 takes the actual generated action, and other modes the requested action.
* The producer consumes the real motion's node/key storage and source frame
  wrapping through the existing MSVC conversion. Source state is count at
  0x3c58270, the 128 by 64 byte buffer at 0x3c58278 and limit at 0x3c5a278.
  These need one canonical persistent owner, not temporary matrices in a callback.
* Recursive traversal at 0x4b7d00 samples MDATA2/MDATA3 position/angle/scale,
  updates the shared matrix stack and writes the real output slots. It returns a
  sibling; its parent loops over children. The top-level CalcMMMatrix calls it
  once and ignores its returned root sibling. GetNodeIndex, in contrast, counts
  root siblings as well. A generic shared tree traversal would change behavior.
* Missing MDATA2 position keys translate by the live object pose unconditionally;
  MDATA3's corresponding fallback checks the object's position flag. Non-null
  channels and fallback flags have different rules. The animation transition
  builder is not a drop-in sampler for this producer.
* GetMMMatrix returns false without changing the destination for an invalid
  unsigned index. PJoin ignores that return. The four persistent weld scratch
  matrices at 0x3b42f38, 0x3b42ef8, 0x3b42eb8 and 0x3b42e78 must therefore retain
  source bytes across calls; identity substitution or local zero defaults fail.
* Active geometry uses existing point/vector transforms, normalization and
  `invert_mesh_matrix`. The unrecovered njMultiMatrix and numeric njRotateXYZ
  wrapper remain additional necessary matrix primitives; copying the SSE D3DX
  multiplication or the animation algorithm would not implement their source.
* Six player attachment positions and six direction vectors are part of the
  original 512-byte player allocation. A source-zero allocation is evidence for
  their initial bytes, not permission to create a second pose/vector owner.

Session, PlayerSystem, Display and global inventories were not edited. Recovery
of this producer should add its canonical state through Root's exact owner
binding, borrow Display's existing matrix machinery, and borrow both imported
GammaMotionAssets resources and the actual generated action/motion/arena keys.
It must compare nonzero node matrices and every attachment component before
removing this packet's stop. This is a separate tight cluster beyond the current
first-call/backup attempt; none of its runtime arithmetic has been implemented
or claimed here. Laser targets are the actual next normal-input stop.
