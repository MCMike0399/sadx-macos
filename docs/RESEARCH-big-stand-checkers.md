# Big stand-mode checker chain (`BigUpdateActionState` 0x48e640)

Status: research note, read-only. No production source is changed here. The
scope is the seven checkers called from the `MD_BIG_STND` case of
`BigUpdateActionState` 0x48e640:

```
BigCheckDamage 0x48d1f0
  -> 0x48d400 -> 0x48c190 -> 0x48db90 -> 0x48d2a0 -> 0x48c1c0 -> 0x48d380
```

Each non-zero return is "transition handled, stop the chain". The caller stores
the result into a source local and returns from `update_action_state`; the mode
store is what actually changes the state (`src/player/big_selection_core.cpp:169`).

Evidence used:

* `reference/inventory-raw/big-checkers-decompile.json` (the seven pseudocodes
  plus `0x48c550` `BigHurtReaction` and `0x4146b0` `IsLevelChaoGarden`).
* `reference/inventory-raw/big-thecat-490a00.pseudocode.txt` and
  `reference/inventory-raw/big-held-48e000.pseudocode.txt` for the common tail
  and the `kHolding`/held-object paths.
* `reference/inventory/functions.json` for the callee lists and byte ranges
  (next-function boundaries: `0x48d1f0..0x48d2a0`, `0x48d400..0x48d7c0`,
  `0x48c190..0x48c1c0`, `0x48db90..0x48dc00`, `0x48d2a0..0x48d380`,
  `0x48c1c0..0x48c2a0`, `0x48d380..0x48d400`).
* `reference/sadx-mod-loader/SADXModLoader/include/SADXStructsNew.h`,
  `SADXEnumsNew.h`, `SADXVariables.h` for the source layouts, mode names and
  button masks.
* Helper bodies not in the checkers JSON (`0x48c130`, `0x43bf40`, `0x43bfe0`,
  `0x48ce60`, `0x48c000`) were read from the pinned `sonic.exe` `.text` with
  `objdump -d` at the same virtual addresses; the JSON has no assembly.
* `reference/symbols/overrides.tsv` for the existing inferred names. No
  SADXModLoader header names any of the seven addresses; the only Big names in
  `SADXFunctionsNew.h` are `BigDestruct`, `BigDisplay`, `BigTheCat`,
  `BigGetFloat` and `BigDirectAhead`. The names below are therefore semantic
  proposals, not imported symbols.

The preview facts assumed by the verdicts are: character-select actor, no
controller buttons, `MD_BIG_STND`, no land/contact, no held task, after the
two warm-up frames. The report also states where the preview itself injects
state that changes the answer.

## 1. Field and ABI map

The checker ABI is the game's register convention: `taskwk*` in `ECX`/`ESI`,
`playerwk*` in `EAX`/`EDI`, and the caller's `motionwk2*` on the stack. The
decompiler's `in_EAX` is the `playerwk*`; `unaff_EDI`/`in_ECX`/`unaff_ESI` are
the `taskwk*`; `param_1` is the `motionwk2*`.

| Source field | Offset | Native owner |
|---|---|---|
| `taskwk.mode` | +0x00 | `PlayerTaskState.action_at_0` / `PrimaryView.mode` |
| `taskwk.smode` | +0x01 | `PlayerTaskState.secondary_mode_at_1` |
| `taskwk.flag` | +0x04 | `PlayerTaskState.orientation.flags` / `PrimaryView.status` |
| `taskwk.wtimer` | +0x06 | `PlayerTaskState.control_timer_at_6` / `bind_wait_timer` |
| `taskwk.value` (held pointer in 0x48d400) | +0x10 | not exposed as a semantic field; arena read only |
| `taskwk.cwp` | +0x38 | `collision_component::Work` reference |
| `taskwk.ewp` | +0x3c | `event_actions` event work (`event_work`) |
| `motionwk2.spd` | +0x04 | `MotionWork.speed` |
| `motionwk2.ang_aim.x` | +0x1c | `PlayerMotionState.angle_x_at_1c` |
| `motionwk2.ang_aim.z` | +0x24 | `PlayerMotionState.angle_z_at_24` |
| `playerwk.equipment` | +0x04 | `PlayerPhysicsState.equipment` |
| `playerwk.flag` | +0x10 | `PlayerPhysicsState.flags_at_10` |
| `playerwk.spd` | +0x38 | `PlayerPhysicsState.motion.speed` |
| `playerwk.attr` | +0x5c | `PlayerPhysicsState.attributes` |
| `playerwk.htp` | +0x68 | `PlayerPhysicsState.held_task_at_68` |
| `playerwk.ttp` | +0x6c | `PlayerPhysicsState.moving_task_at_6c` |
| `playerwk.p.run_speed` | +0xc4 | `PlayerParameters.run_speed` |
| `playerwk.p.jmp_y_spd` | +0xb4 | `PlayerParameters.jmp_y_spd` |
| `playerwk.mj.reqaction` | +0x124 | `InitializedPlayerView.requested_animation` |
| `playerwk.mj.nframe` | +0x130 | `PlayerPhysicsState.animation_tail.frame_at_130` |
| `playerwk.shadow_attributes_top` | +0x1e4 | `PlayerPhysicsState.shadow_attributes_top` |
| `playerwk.shadow_top_y` | +0x1ec | `PlayerPhysicsState.shadow_top_y` |

Source-backed flag names already recovered for this project
(`src/player/sonic_player_internal.hpp:13-23`):

```
kGrounded = 3 (Status_Ground | Status_OnColli)
kHurt = 4
kAttack = 1024
kHolding = 2048
kNextAction = 4096
kDisableControl = 16384
```

The checker `0x48d400` uses `kNextAction`; `0x48db90` uses `kGrounded` and
`kHolding`; `0x48d2a0` uses `kHolding` and `kNextAction` on the held task;
`0x48d380` sets `kAttack`.

The source `playerwk.flag` at +0x10 is the same status word as
`taskwk.flag`: `PSetPosition` copies `taskwk.orientation.flags` into it
(`src/player/player_scene_position.cpp:702`). It is zeroed at player
calloc (`src/player/player_startup.cpp:70`).

## 2. Verdict summary

| Address | Proposed name | Preview verdict | Decisive source guard |
|---|---|---|---|
| `0x48d1f0` | `BigCheckDamage` | **returns 0** | `attr & 0x10000 == 0` (no collision contact) |
| `0x48d400` | `BigCheckNextAction` | **needs real recovery** | `flag & 0x1000` is set by the preview's own `set_input(12)`, and `smode=12` takes the PLON case |
| `0x48c190` | `BigCheckWaterEntry` | **returns 0** | `shadow_attributes_top & 2 == 0` (no water/land) |
| `0x48db90` | `BigCheckFall` | **needs real recovery** | `(playerwk.flag | taskwk.flag) & 3 == 0` and no held task -> mode 8 |
| `0x48d2a0` | `BigCheckJump` | **returns 0** | `JumpAllowed` sees no held jump button |
| `0x48c1c0` | `BigCheckGrab` | **returns 0** | no held grab button |
| `0x48d380` | `BigCheckAttack` | **returns 0** | no held attack button |

The two non-zero cases are the important result: `0x48db90` transitions on the
first stand frame, and `0x48d400` is not provably inactive once the preview's
display pass has run. The other five can be recovered with their exact guards,
and their preview branch is the zero branch. `0x48d400` still needs an exact
body: whether the current task ordering reaches its `kNextAction` branch in the
preview is an integration question, not an inactive-branch proof.

## 3. Functions

### 3.1 `0x48d1f0` — `BigCheckDamage` (existing name)

**Range:** `0x48d1f0..0x48d2a0`. Calls `dsPlay_oneshot` 0x423d70 and
`CameraReleaseEventCamera` 0x436140.

**Tests.** `taskwk.mode`, `taskwk.smode`, `taskwk.flag`,
`playerwk.attr (+0x5c)`, `playerwk.p.run_speed (+0xc4)` and
`playerwk.p.jmp_y_spd (+0xb4)`.

The early-outs are, in order:

* `mode == 0x37` (`MD_BIG_CART`) -> 0.
* `mode == 0x27` (`MD_BIG_MISS`) -> 0 at the common tail.
* `(flag & kNextAction) && smode == 0x13` -> 0.
* `mode` in signed-byte range `0x23..0x25` (`jl 0x23`, `jle 0x25`) -> 0.
* `mode` in `0x2b..0x2d` -> 0.
* If `flag & kHurt` is clear, require a non-null player and
  `(playerwk.attr & 0x10000) != 0`; otherwise 0.

If it falls through, it plays sound `0x52a`, stores `MD_BIG_DAMG` (0x0b),
`playerwk.mj.reqaction = 0x13`, clears `kAttack` (0x400) and `kHurt` (0x04),
sets `playerwk.spd.x = -playerwk.p.run_speed`,
`playerwk.spd.y = playerwk.p.jmp_y_spd`, `taskwk.wtimer = 0x78`, calls
`CameraReleaseEventCamera`, and returns 1. The source order matters:
`speed.x` is the negated `run_speed` through x87 `FCHS`, `speed.y` is the
`jmp_y_spd` word copy, and `speed.z` is untouched.

**Return convention.** `EAX` = 0 (not handled) or 1 (damage transition
handled); no other value is produced.

**Preview verdict: returns 0.** On the stand frame `taskwk.flag` has no
`kHurt` and `playerwk.attr` is zero. `attributes` is initialized to zero by the
value-initialized `PlayerPhysicsState` (`src/player/player_physics.hpp:20`,
allocated at `src/player/player_startup.cpp:321`) and is only OR'd from a
collision contact in `apply_player_contact_attributes`
(`src/player/player_contact_attributes.cpp:15`). The preview has no land or
contact, so the collision response that would set the damage bit never runs.
The exact guard is `0x48d238: testl $0x10000,0x5c(%edi)` followed by
`0x48d23f: je 0x48d1f7` (return 0). The `flag & kHurt` test at `0x48d230` is
clear for the same reason (`taskwk.flag` begins at zero).

**Native owners.** `PlayerPhysicsState.attributes` and
`PlayerParameters.run_speed`/`jmp_y_spd`
(`src/player/player_physics.hpp:20`, `src/player/player_state.hpp:13`);
`sound_effects::play_one_shot` (`src/game/sound_effects.hpp:120`); the camera
release call is not recovered and needs a named seam or a camera owner.

### 3.2 `0x48d400` — proposed `BigCheckNextAction`

**Range:** `0x48d400..0x48d7c0`. Calls `dsPlay_oneshot` 0x423d70,
`PClearSpeed` 0x43c550, `CancelLookingAtP` 0x441080, `FUN_00473780` 0x473780,
`BigStandTail` 0x48ba80 and `FUN_0048cc10` 0x48cc10.

**Tests.** `taskwk.ewp->move.mode` (`ewp+0x24`), `taskwk.ewp->path.list`
(`ewp+0x4c`), `taskwk.flag & kNextAction`, and `taskwk.smode`. The sub-mode is a
signed byte jump table over `smode-1 <= 0x31` (`0x48d44e..0x48d45f`). It handles
Big's action transitions, not just a predicate: the cases cover
`MD_BIG_1DWK`, `SPRG`, `LNCH`, `PARA`, `PLON`, `FALL`, `CART`, `STND`,
`LDWT`, `POSE`, `HOLD`, `DOZA` and `KILL`. It also uses
`taskwk.value.ptr` (`taskwk+0x10`) as the held-task pointer in the `smode 0x25`
case.

The source shape is the Big counterpart of Sonic's 0x495fa0 `event_action`
(`src/player/sonic_player_internal.hpp:494`): the same
`move.mode` / `path.list` prefix, the same `kNextAction` gate and a
character-specific sub-mode switch. The existing placeholder
`BigStandCheckA` should become `BigCheckNextAction`; `BigEventAction` is an
acceptable alternative if the team wants the Sonic analogy in the name.

**Return convention.** `EAX` = 0 or 1. A non-zero return has already stored the
new `taskwk.mode`, the new `playerwk.mj.reqaction`, flag changes, and in some
cases motion speed/acceleration or `BigStandTail`.

**Preview verdict: needs real recovery.** The naive "no controller input"
argument does not apply to this function, because the preview injects an
event-input action into the actor's primary. Specifically:

* `selection_preview::update_actions` calls
  `calls.set_input(slot, kEventInput)` with `kEventInput = 12` for every actor
  except slot 6 (`src/game/selection_preview_actions.cpp:16,26`). Big is
  `ActorKind::Big`, slot 4 (`src/game/selection_preview.hpp:32`).
* `event_actions::set_input` sets `primary.orientation.flags |= kInputPending`
  (`src/player/event_actions.cpp:9,158`). `kInputPending` is `1U << 12`, which
  is the same 0x1000 bit named `kNextAction` in the Sonic controller
  (`src/player/sonic_player_internal.hpp:20`).
* The same call stores `secondary_mode = 12`
  (`src/runtime/native_event_actions_context.cpp:66-68`), which is
  `taskwk.smode`.

`selection_preview::draw` runs `update_actor_actions` only when
`warmup_frames >= 2` (`src/game/selection_preview.cpp:132`). Once that display
warm-up has run, the stand chain can therefore see
`flag & 0x1000 != 0` and `smode = 12`. In the switch, `0x0c` is in the
`case 0x09 / 0x0a / 0x0c` group: it stores `MD_BIG_PLON` (0x0e),
`reqaction = 0`, copies the motion aim angles into `taskwk.ang`, clears the
motion speeds and `playerwk.spd`, clears the `0x24` flag bits, and returns 1.
The exact guard is `0x48d43e..0x48d44c` (`flag & 0x1000`; `je 0x48d421` return
0) and the jump table at `0x48d44e`; the `0x0c` case is part of the
pseudocode's `case '\t' / case '\n' / case '\f'` group.

The ordering caveat: in the current task graph the first stand frame runs
before the first draw-time `update_actions` (warmup reaches 2 in that frame),
and `0x48db90` preempts this function with the fall transition on that frame.
So a run that only ever inspects frame 2 can observe 0. That is an ordering
fact, not an inactive-branch proof, and it must be tested rather than assumed.
The function itself is not provably zero and its `smode 0x0c` case is a real
transition that leads to `MD_BIG_PLON`, which the core currently sends to
`unrecovered_mode`.

**Native owners.** `event_actions::move_mode` and `event_actions::has_path`
already expose the two event-work guards (`src/player/event_actions.hpp:66-69`);
`src/player/event_actions.cpp:8` pins `kMoveMode = 36` (0x24) and
`kPathHead = 76` (0x4c), matching the checker offsets.
`event_actions::set_input` explains the 0x1000 bit. `clear_speed`
(`src/player/player_motion.hpp:53`) covers `PClearSpeed`. `BigStandTail`
(`src/player/big_selection_core.hpp:144`) is a Big seam. `CancelLookingAtP`
0x441080, `FUN_00473780` 0x473780 and `FUN_0048cc10` 0x48cc10 have no native
owner yet. The held-task pointer at `taskwk+0x10` is not exposed as a semantic
field; `PlayerPhysicsState.held_task_at_68` is the player side only.

### 3.3 `0x48c190` — proposed `BigCheckWaterEntry`

**Range:** `0x48c190..0x48c1c0`. Its whole test is the helper
`FUN_0048c130` (`0x48c130..0x48c190`), which is executed in place; this is not
a leaf predicate that should be replaced in an oracle.

**Tests.** `playerwk.equipment & 0x1000` (Big's rod equipment),
`Controllers[taskwk.player_index].held & JumpButtons`
(`0x3b0e800 + index*0x34`, mask `0x909f98`), `playerwk.shadow_attributes_top &
2`, `taskwk.cwp->info[0].center.y + 5.0` against
`playerwk.shadow_top_y`, and `taskwk.flag & kGrounded` on the non-submerged
branch.

The `0x2` bit is the water signal: the native shadow owner defines
`kWaterAttributes = 0x400002` (`src/player/character_shadow.cpp:6`), and the
same bit is used by the cast check at `0x48c000` (`testl $0x400002`). The
helper returns 1 only when the rod is equipped, the jump button is not held,
the shadow is over water, and either the water is more than five units above
the collision center or Big is not grounded. It then stores
`MD_BIG_W_WAIT` (0x2f) and `reqaction = 0x5c` (92). There is no sound store in
either function.

**Return convention.** `0x48c130` returns 0/1 in `EAX`; `0x48c190` returns 0
when the helper is zero and 1 after storing the water-wait mode/action.

**Preview verdict: returns 0.** The shadow attributes are initialized to zero
(`src/player/player_startup.cpp:97`; `initialize_physics` zeroes
`shadow_attributes_top`) and are only written by `character_shadow::update`
from land candidates (`src/player/character_shadow.cpp:29,49,89`). The Big
binding's shadow product has no land candidates on the selection path and
returns an empty list (`src/runtime/native_big_binding.cpp:465-470`), so
`shadow_attributes_top` stays zero. The exact guard is
`0x48c158: testb $0x2,0x1e4(%eax)` followed by
`0x48c15f: je 0x48c189` (`xor eax,eax; ret`). The earlier rod guard
`0x48c13c..0x48c13f` returns 0 as well when Big has no rod, so the result is
zero with or without the save's rod event flags. The jump-button guard is also
clear, but it is not needed for the verdict.

**Native owners.** `PlayerPhysicsState.equipment`,
`shadow_attributes_top` and `shadow_top_y`
(`src/player/player_physics.hpp:27,21,43`);
`collision_component::Info.center` (`src/engine/collision_component.hpp:18`);
`GameLogicController::held_offset` and `GameControllerButtonMasks`
(`src/engine/game_controller.hpp:19,33`); `character_shadow::update`
(`src/player/character_shadow.cpp:19`) for the water attribute source.

### 3.4 `0x48db90` — proposed `BigCheckFall`

**Range:** `0x48db90..0x48dc00`. Calls `PCheckHoldObject` 0x43b9c0 and
`BigHurtReaction` 0x48c550.

**Tests.** `playerwk.flag` (+0x10) OR'd with the low byte of `taskwk.flag`,
masked with `kGrounded` (3); then `taskwk.flag & kHolding` (0x800); then
`PCheckHoldObject(taskwk)` and its `0x200` (`kCarryable`) bit.

The exact source:

```
if (((playerwk.flag | taskwk.flag) & 3) != 0) return 0;
if ((taskwk.flag & 0x800) != 0) {
    held = PCheckHoldObject(taskwk);
    if (held & 0x200) { mode = 0x16 (HFLL); reqaction = 0x27; return 1; }
    BigHurtReaction(motionwk);
}
mode = 0x08 (FALL);
reqaction = 0x0e;
return 1;
```

If Big is not grounded but is carrying a carryable object, it enters the held
fall; otherwise it enters the ordinary fall. It is a fall selector, not a
damage check.

**Preview verdict: needs real recovery.** The preview has no land/contact, so
both status words keep their zero grounded bits: the task primary is zeroed and
`enter` only ever toggles `kOnCollision` (0x02), never `kGrounded`
(`src/player/big_selection_core.cpp:32-60`), and `playerwk.flag` is zeroed at
calloc and not synced before the checker (`src/player/player_startup.cpp:70`).
The preview also has no held task, so `taskwk.flag & kHolding` is zero. The
exact guards are `0x48dba1: testb $0x3,%cl` / `0x48dba4: jne 0x48dbf3` (return 0), then
`0x48dba6: andl $0x800,%eax` / `0x48dbae: je 0x48dbdf`, and the stores at
`0x48dbdf: movb $0x8,(%esi)` and `0x48dbe2: movw $0xe,0x124(%edi)` with
`0x48dbec: movl $1,%eax`. There is no held-object call and no hurt-reaction
call on this branch. This is the first checker that actually changes the
preview state.

Consequence: an exact implementation returns 1 and puts Big in `MD_BIG_FALL`
(0x08). The current core's mode switch has no `MD_BIG_FALL` case and reaches
`unrecovered_mode` (`src/player/big_selection_core.cpp:250-274`), so the
preview cannot be made visible by this checker alone. The next recovery after
this checker is either the fall mode or the collision/landing owner that sets
`kGrounded` before the stand chain; it must not be special-cased in the
checker.

**Native owners.** `PlayerPhysicsState.flags_at_10`
(`src/player/player_physics.hpp:32`), `PlayerTaskState.orientation.flags`,
`query_player_hold`/`bind_player_hold` (`src/player/player_hold.hpp:35,39`) for
the real `PCheckHoldObject`, and `BigHurtReaction` 0x48c550 (still a Big stub).

### 3.5 `0x48d2a0` — proposed `BigCheckJump`

**Range:** `0x48d2a0..0x48d380`. Calls `dsPlay_oneshot` 0x423d70,
`JumpAllowed` 0x43bf40, `SetAccelerationP` 0x441750 and `BigHurtReaction`
0x48c550.

**Tests.** `JumpAllowed(taskwk, playerwk)`, then `taskwk.flag & kHolding`,
`playerwk.htp` (+0x68), the held task's `taskwk.flag & kNextAction` (0x1000),
the moving task `playerwk.ttp` (+0x6c) and its `+0x28` vector at
`+0x10/+0x14/+0x18`, and `playerwk.p.jmp_y_spd`.

`JumpAllowed` 0x43bf40 itself tests `taskwk.flag & kDisableControl` (0x4000)
and `Controllers[taskwk.player_index].held & JumpButtons` (`0x909f98`), then
classifies the jump target with `FUN_004b83f0`. Its source counterpart is
`jump_pressed` (`src/player/sonic_player_internal.hpp:484-490`).

If allowed, it plays sound `0x51e`. A holding Big whose held task has
`kNextAction` enters `MD_BIG_HJMP` (0x17), sets
`playerwk.spd.y = jmp_y_spd`, `reqaction = 0x25`, clears the task's
`kOnCollision` bit, plays sound `0x11`, and returns 1. Otherwise it enters
`MD_BIG_JUMP` (0x04), sets `playerwk.spd.y = jmp_y_spd`, optionally calls
`SetAccelerationP(index, moving->+0x10, +0x14, +0x18)`,
`reqaction = 0x0f`, clears `kOnCollision`, zeroes `playerwk.jumptimer`
(+0x08), plays sound `0x11`, and returns 1. If holding but the held task
is null or its flag lacks `kNextAction`, it calls `BigHurtReaction` and then
takes this ordinary jump.

**Return convention.** `EAX` = 0 when `JumpAllowed` is zero; otherwise 1 after
the mode/reqaction stores.

**Preview verdict: returns 0.** The pad has no held jump button, and
`taskwk.flag` has no `kDisableControl` (0x4000); the preview's `set_input` only
sets 0x1000. `JumpAllowed` returns 0 at `0x43bf68` (`je 0x43bf7e`), and the
Big caller returns 0 at `0x48d2b2` (`je 0x48d379`). The exact source guard is
`Controllers[taskwk.player_index].held & JumpButtons == 0` in 0x43bf40; the
`JumpButtons` mask is the same `0x909f98` pointer used by the mod loader
(`reference/sadx-mod-loader/SADXModLoader/include/SADXVariables.h:104`).

**Native owners.** `jump_pressed` (`src/player/sonic_player_internal.hpp:484`)
or a shared Big variant; `PlayerPhysicsState.held_task_at_68` and
`moving_task_at_6c` (`src/player/player_physics.hpp:26,34`);
`PlayerParameters.jmp_y_spd`; `play_one_shot`; `BigHurtReaction`;
`SetAccelerationP` has no native owner yet (the current Big binding has only
`get_acceleration`, a stub at `src/runtime/native_big_binding.cpp:355`).

### 3.6 `0x48c1c0` — proposed `BigCheckGrab`

**Range:** `0x48c1c0..0x48c2a0`. Calls `PCheckHoldObject` 0x43b9c0 and
`FUN_0043bfe0` 0x43bfe0 (the recovered `wall_form`).

**Tests.** `Controllers[taskwk.player_index].held & GrabButtons`
(`0x909fa4`), `PCheckHoldObject(taskwk)`, `FUN_0043bfe0(motionwk, taskwk)` (the actual
call-site pair), and the held-object result bits. `FUN_0043bfe0` returns 1..4;
the source guard `0 < form < 5` is therefore a form-validity gate, not a
gameplay predicate (the recovered body is
`src/player/sonic_player_internal.hpp:426-437`). The recovered `wall_form`
context assumes a player/task view, while this call site passes the motion work
as the first argument; the Big binding and oracle must model the actual pair.

The result bits select the carry action:

* `held & 0x400` (`kTouchable`): `MD_BIG_TOCH` (0x12), `reqaction = 0x23`,
  stores a task pointer from the collision hit chain into `playerwk.htp`, and
  sets `kHolding` (0x800) on the task flag.
* `held & 0x200` (`kCarryable`): `MD_BIG_PICK` (0x0f),
  `reqaction = 0x1a` or `0x4e` depending on the held task's taskwk id
  (`id == 4` selects 0x4e; the source computes
  `(((id != 4) - 1) & 0x34) + 0x1a`).
* `held & 0x800` (`kSpecialHold`): `MD_BIG_TOCH` (0x12),
  `reqaction = 0x23`.

If none of the three bits is set, it returns 0 without a mode store.

**Preview verdict: returns 0.** The pad has no held grab button, so
`0x48c1cd: testl %ecx,0x3b0e800(%eax)` / `0x48c1d4: je 0x48c295` returns 0
before any hold query. This is independent of `PCheckHoldObject` and
`wall_form`.

**Native owners.** `GameLogicController`/`GameControllerButtonMasks`
(`src/engine/game_controller.hpp:19,33`); `query_player_hold`
(`src/player/player_hold.hpp:35`); `wall_form`
(`src/player/sonic_player_internal.hpp:426`) if the form check is kept, after
generalizing its first argument for this call site;
`PlayerPhysicsState.held_task_at_68`; the collision hit task chain
(`collision_component::Work.hits`/`other` at
`src/engine/collision_component.hpp:22-38`) for the `0x400` branch.

### 3.7 `0x48d380` — proposed `BigCheckAttack`

**Range:** `0x48d380..0x48d400`. Calls `FUN_0048ce60` 0x48ce60,
`FUN_0046ee40` 0x46ee40 and `FUN_0048c000` 0x48c000.

**Tests.** `FUN_0048ce60()` (stage/act gate: reads `ssStageNumber` at
`0x3b22dcc`, act at `0x3b22dec`, and a global at `0x3b2caac`),
`Controllers[taskwk.player_index].held & AttackButtons` (`0x909f94`), then
`FUN_0046ee40(taskwk)` and `FUN_0048c000(motionwk)`.

`FUN_0048c000` is the cast allowance: it tests
`playerwk.shadow_attributes_top & 0x400002` (the water predicate again) and,
over water, compares `shadow_top_y` against a converted eye/point value. If
`FUN_0046ee40()` is zero and `FUN_0048c000()` is non-zero, it stores
`reqaction = 0x34` and `MD_BIG_CAST` (0x20). Otherwise it stores
`reqaction = 0x31`, sets `kAttack` (0x400), and stores `MD_BIG_ATCK` (0x1f).
Both branches return 1.

**Preview verdict: returns 0.** The attack button is not held:
`0x48d393: movl $0x909f94,%ecx` / `0x48d39c: testl %ecx,0x3b0e800(%eax)` /
`0x48d3a2: je 0x48d3ee` returns 0. `FUN_0048ce60` may return 1 for a non-stage
0x1a selection, but the `&&` means the pad is the decisive fact.

**Native owners.** `GameLogicController`/`GameControllerButtonMasks`;
`context.stage`/`context.act` and the stage/act gate (not yet a named native
owner); `FUN_0046ee40`, `FUN_0048c000` and `FUN_0048ce60` have no native
owners; the cast check reuses `shadow_attributes_top` and `shadow_top_y`.

## 4. Ordered implementation plan

The order below is chosen to remove the reached preview stops in the order the
stand frame hits them, while keeping each new owner shared by the largest
number of checkers. It deliberately does **not** start with the largest
function (`0x48d400`) just because it is second in the source chain.

### Phase 0 — shared owners (no checker body yet)

1. Extend `big_selection::Context` with the existing controller sample and
   button masks, mirroring the Gamma context's `controller`/`play_effect`
   fields (`src/runtime/native_player_system.hpp:97-103`). The sample must
   provide the held word at record offset 0x08 and the three masks
   (`0x909f94`, `0x909f98`, `0x909fa4`), because the checkers read the held
   word (`GameLogicController::held_offset`,
   `src/engine/game_controller.hpp:19`).
2. Add the sound callback (`sound_effects::play_one_shot`,
   `src/game/sound_effects.hpp:120`) and the camera-release seam
   (`CameraReleaseEventCamera` 0x436140) to the Big context.
3. Expose `playerwk.htp`/`ttp` task views and the collision hit task chain so
   `0x48d2a0` and `0x48c1c0` can read the held/moving task without guest
   pointers.
4. Recover `BigHurtReaction` 0x48c550 first; it is shared by `0x48db90` and
   `0x48d2a0`. It is still a stub at
   `src/runtime/native_big_binding.cpp:264`.

### Phase 1 — small, self-contained checks

5. `0x48d1f0` `BigCheckDamage`. Smallest function, chain head, only needs
   `attributes`, the two parameters, sound and camera release. Its preview
   branch is the zero branch, but the full body is still required for the
   damage modes.
6. `0x48c190` + `0x48c130` `BigCheckWaterEntry`. Small and fully self-contained
   once the collision info center and shadow water attribute are wired. The
   oracle must execute `0x48c130` in place rather than replace it.
7. `0x48d2a0` `BigCheckJump`. Medium; the pad/mask owner and the held/moving
   task views are now in place. Reuse `jump_pressed` (`0x43bf40`) and add the
   missing `SetAccelerationP` owner or a named seam.

### Phase 2 — the active transition and the pad checks

8. `0x48db90` `BigCheckFall`. It is the first checker that actually changes the
   preview state. Recover it **together with** `BigHurtReaction` and the
   `MD_BIG_FALL` transition (or a source-backed grounded/collision owner), or
   the preview will stop at the new mode immediately after the checker starts
   returning 1.
9. `0x48c1c0` `BigCheckGrab`. Needs `PCheckHoldObject`, `wall_form`, the
   collision hit task chain and the held-task store.
10. `0x48d380` `BigCheckAttack`. Needs the stage/act gate, the cast check and
    the attack/cast mode stores.

### Phase 3 — the large state machine

11. `0x48d400` `BigCheckNextAction`. Recover after the event-work owner,
    `PClearSpeed`, `CancelLookingAtP`, `BigStandTail` and `FUN_0048cc10` are
    available. The full `smode` table is the largest single piece; the oracle
    should cover every case, not just the preview's `0x0c`. Pair it with
    `MD_BIG_PLON` (0x0e) and the modes it reaches.

The exact source-order recovery is `0x48d1f0`, `0x48d400`, `0x48c190`,
`0x48db90`, `0x48d2a0`, `0x48c1c0`, `0x48d380`. The plan above is the same
content grouped by owner and risk: the two functions that can return 1 in the
preview (`0x48db90`, and `0x48d400` once the display pass has set the input
bit) are paired with their target modes, and the five zero-branch functions are
recovered first because they are cheap and remove the stub throws in order.

## 5. Oracle shape

The pattern is the one used by
`tools/oracles/verify_original_player_underwater.py`: a Unicorn `Source` maps
the PE sections, executes the original bytes from `ENTRY` to `END`, replaces
unrecovered callees at their entry addresses, records their arguments, and
compares a fixed word array against a native probe. The probe is a small C ABI
function in `tests/probes/`, the verifier writes
`build/oracles/big_stand_checkers.json`, and `tools/gate/mutate.py` mutates one
constant per function.

Suggested slug: `big_stand_checkers`. One probe
`tests/probes/native_big_stand_checkers_probe.cpp` exports one function per
checker:

```
sadx_native_big_check_damage
sadx_native_big_check_next_action
sadx_native_big_check_water_entry
sadx_native_big_check_fall
sadx_native_big_check_jump
sadx_native_big_check_grab
sadx_native_big_check_attack
```

One verifier `tools/oracles/verify_original_big_stand_checkers.py` runs seven
cohorts and reports a `functions` map; the top-level `passed` is the AND of all
seven. `make oracle-big-stand-checkers` and `make oracles-verdict` then work
without a manifest entry unless the probe needs extra inputs.

| Function | Entry/end | Replaced calls (arguments checked) | Input block | Observed outputs | Curated cases |
|---|---|---|---|---|---|
| `damage` | `0x48d1f0..0x48d2a0` | `0x423d70` `(0x52a,0,0,0)`, `0x436140` `()` | task mode/smode/flag/wtimer, player attr/run_speed/jmp_y_spd | task mode/flag/wtimer, player reqaction/spd.x/spd.y, return, call flags | modes 0x37/0x27/0x23-0x25/0x2b-0x2d, `kNextAction`+smode 0x13, `kHurt`, attr 0/0x10000, run_speed sign, player null |
| `next_action` | `0x48d400..0x48d7c0` | `0x423d70`, `0x43c550`, `0x441080`, `0x473780`, `0x48ba80`, `0x48cc10` | task mode/smode/flag/value, ewp move.mode/path.list, player hpos/reqaction, motion spd/angles | task mode/smode/flag, player reqaction/htp, motion spd, return, call flags | move.mode 0/1, path null/non-null, `kNextAction` 0/1, every `smode` case, held pointer in 0x25, FP compare 0x1a |
| `water_entry` | `0x48c190..0x48c1c0` | none (0x48c130 executed in place) | task flag/index, player equipment/flag/shadow attrs/top_y, held jump mask, cwp/info center | task mode, player reqaction, return | rod 0/0x1000, jump 0/1, shadow attr 0/2/0x400002, center vs top_y less/equal/greater/NaN, grounded 0/3 |
| `fall` | `0x48db90..0x48dc00` | `0x43b9c0` `(taskwk)`, `0x48c550` `(motionwk)` | task flag, player flag, hold result | task mode, player reqaction, return, call flags | grounded 0/1/2/3, holding 0/0x800, hold result 0/0x200/0x400/0x800 |
| `jump` | `0x48d2a0..0x48d380` | `0x43bf40` `(taskwk,playerwk)`, `0x423d70` `(0x51e/0x11)`, `0x441750` `(index,vector)`, `0x48c550` `(motionwk)` | task flag/index, player htp/ttp/jmp_y_spd/reqaction, held task flag, moving vector, pad held mask | task mode/flag/jumptimer, player reqaction/spd.y, return, call flags | jump 0/1/2, holding 0/0x800, held `kNextAction` 0/1, htp/ttp null/non-null |
| `grab` | `0x48c1c0..0x48c2a0` | `0x43b9c0` `(taskwk)`, `0x43bfe0` `(motionwk,taskwk)` | task flag/index, player htp, hold result, wall form, hit chain, pad held mask | task mode/flag, player reqaction/htp, return | pad 0, hold result 0/0x200/0x400/0x800/0x600, form 0/1/4/5, id 4/other |
| `attack` | `0x48d380..0x48d400` | `0x48ce60` `()`, `0x46ee40` `(taskwk)`, `0x48c000` `(motionwk)` | task flag/index, player index, pad held mask, fixture results | task mode/flag, player reqaction, return | pad 0, stage gate 0/1, 46ee40 0/1, cast 0/1 |

Shared limits to record in the report:

* The replaced calls are fixtures, not the original bodies; their arguments are
  checked, and a missing required call fails the case.
* x87 status flags are not compared unless a case depends on the unordered
  branch; the stored binary32 values and the compare branch are.
* The event path is a fixture (`has_path`/`move.mode`), not a full event
  runtime; the source-order task/draw interaction that sets `kNextAction` is a
  separate integration test, not an oracle case.
* The `0x48c1c0` hit-chain pointer and the `0x48d400` `taskwk.value` held
  pointer are fixture words; the native probe must consume the task/held views
  without casting guest addresses.

Mutants: one wrong constant per checker, at minimum:

* `damage`: damage attribute 0x10000 -> 0x20000; mode 0x0b -> 0x0c.
* `next_action`: 0x1000 -> 0x2000; smode 0x0c mapping -> 0x0e mode.
* `water_entry`: water bit 2 -> 4; W_WAIT 0x2f -> 0x30.
* `fall`: grounded mask 3 -> 1; fall mode 8 -> 9.
* `jump`: jump mask -> grab mask; jump mode 4 -> 5.
* `grab`: grab mask -> attack mask; carryable 0x200 -> 0x400.
* `attack`: attack mask -> jump mask; attack mode 0x1f -> 0x20.

## 6. Open risks and facts to re-check

* The preview injects `kNextAction`/`smode=12` through its own display path.
  Any claim that `0x48d400` is inactive on "no controller input" is incomplete
  until the task/draw ordering is tested.
* `0x48db90` is the first active transition. It exposes `MD_BIG_FALL` before
  the core can display a stand frame. The next step after this report is either
  the fall mode or the source collision/landing owner that sets `kGrounded`.
* The checker verdicts are about the stand chain only. The surrounding preview
  frame has other owners (the display submit, for example); the underwater
  binding is now composed at `src/runtime/native_underwater_binding.cpp:41`,
  but its breath callback is still an unrecovered boundary. None of that
  changes the guard verdicts.
* `0x48c190`'s rod guard depends on the save's Big event flags (464/465 map to
  0x1000/0x800, `src/player/player_startup.cpp:115`); the water attribute
  guard is independent and is the decisive preview fact.
* `0x48c130`'s collision-center read uses `taskwk.cwp->info[0].center.y`; the
  native collision `Info.center` is available, but the source also uses the
  first record even when `info_count` is larger. Keep the exact record 0.
* No SADXModLoader header names any of the seven addresses. The proposed names
  should be recorded in `reference/symbols/overrides.tsv` only after the
  bodies and oracles agree; the current entries are still inferred.
