# Scene dispatcher include preparation

Before moving the existing fixture types out of player_scene_position.hpp, the
two existing probes and three unit sources include the guest compatibility header.
That header currently forwards to the unchanged old player header. No assertions,
fixtures, comparison code, compiler flags, source arithmetic or output words change.
This completes the include portion of aa55337's earlier link/source-identity prep.
The source include seam was committed separately; full dispatcher migration follows.
