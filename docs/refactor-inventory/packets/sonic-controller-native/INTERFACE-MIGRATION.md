# Reference include migration

The controller task requires executable-independent Sonic headers. Existing
reference consumers now include `guest/sonic_player_reference.hpp`, initially
forwarding to the unchanged old header. The subsequent source commit places the
legacy declarations there and makes the player header semantic.

Only include directives change in these existing judge files; no assertions,
inputs, comparison fields, expected values, tolerances or exclusions change.
Root must reconcile protected hashes in its separate integration judge step.

Paths:

- `tests/unit/player_scene_position_binding.cpp`
- `tests/unit/sonic_player.cpp`
- `tests/unit/animation_sonic_composition.cpp`
- `tests/unit/actor_current.cpp`
- `tests/unit/input_chain.cpp`
- `tests/unit/sonic_actions.cpp`
- `tests/probes/native_sonic_player_probe.cpp`
- `tests/probes/native_sonic_action_probe.cpp`
- `tests/reference/actor_current_fixture.hpp`
- `tools/oracles/animation_final_cohorts/consumer.cpp`
