# Player contact reaction instruction comparison

Target: supplied PC `sonic.exe`, SHA256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`. Genuine retained REA dossier [speed-response.json](position-dependencies/speed-response.json), Evidence ID `ev_bba2c578aad8fb13b244ec43a9095d729021888191609d45e99a55574431e49f`. Exact additional LLVM listing: [player-reaction.asm](player-reaction.asm).

This independent audit exercises the real source43c580 reaction body, actual7887d0 plane intersection,438410 vector angle, Ninja arithmetic/normalization, CRTacos and integer conversion. Only41bed0 normal output is an explicitly seeded boundary. The source geometry/normal producer, startup/mod state and original hardware are not established by this comparison.

## Source correction and audited ordering

The initial gate is `FCOMP m32(1)` followed by `TEST AH,5; JNP43c7bd`. Less sets C0 alone and returns; equal has C3 with masked test result0/even parity and continues. The original native draft returned on equality. Root corrected the producing comparison to return only on `less` and added an exact-one test. Fixtures check exact1, adjacent-above1, below1 and grounded/collision masks.

The raw41bed0 normal is copied to player+44/+48/+4c before the source normalizes its local scratch. Plane construction intentionally uses the raw committed normal, together with raw floor+50/+54/+58. The local normalization still executes and contributes FP status even though its result is unused later. The old speed words are loaded before the first replacement speed store; replacement stores occur X,Y,Z. Both prior and replacement vectors normalize before angle438410. Yaw subtraction and the optional8000 correction occur only after angle completion, then the target-yaw store. A pending CRT boundary leaves the committed normal and replacement speed intact, with yaw/target untouched. Source instruction store traces and actual angle argument bytes are retained in the report.

The subsequent length-zero parity gates and direction-sign gate match the source. Yaw is32-bit subtraction/addition without prematurely masking the angle to16 bits; the source angle-distance helper uses the low angle representation. No production source changes were made by this worker; root owns the gate correction and live binder.

## Results and retained differences

[original-player-reaction-comparison.json](original-player-reaction-comparison.json) contains 866 fixtures ×12 CW profiles = 10,392 evaluations. The CW profiles cover24/53/64 precision and all4 rounding directions. The blob [original-player-reaction-comparison.cases.bin](original-player-reaction-comparison.cases.bin) preserves21 raw LE uint32 words per case; SHA256 `19431b3ae8e640bf30375054864d172f2549f2fcec5048babb3477e426d63210`.

- The 10,152 explicit CRT-bypass/owner-boundary evaluations match task flags, full32-bit yaw/target, all world-speed/player44 bit patterns, normal call count, declared boundaries and CW. This includes finite inputs, signed zero, subnormals, infinities, quiet/signaling NaN perturbations, ground/collision flags and unknown/null native resource distinctions.
 - Policy0 nonfinite fixtures reach the actual648b6b dispatcher call and match native pending binary64 spills, source prefix stores and CW. No error handler or return value is substituted.
- Unknown-policy cases stop before the actual CRT policy read. Boundaries, state and CW match. Three pending ext80 values per profile differ in low significand bits, for36 differences total: source Unicorn FPATAN produces a host-double result while native uses the retained Bochs P5/P6 ext80 model. These are preserved, not rounded to fit the emulator.
- For known policy0, three ordinary finite cases in11 profiles return in Unicorn but the native model reaches the missing precision-loss dispatcher648b6b. The source helper checks PE in648c4b..648c52; Unicorn does not set that flag for these FPATAN results. The report retains33 resulting state/boundary differences and27 CW differences. CW27f follows its actual source skip-dispatch condition and matches state/boundary/CW.
- FP flags differ in7320 evaluations, reported separately with examples. `state_boundaries_control_match=false` and `full_emulator_match=false` remain explicit; the bypass subset is named separately. No stronger hardware, complete scene or running-game equivalence is claimed.

## Probe contract and validation

`tests/native_player_reaction_probe.cpp` accepts40 exact constant bytes,21 input words, an explicit CW and40 output words. Constant order: f32zero7dcb2c, f32one7dcb10, f32parallel7e6c30, f64distanceScale7e0b40, f32negativeOne7dcb0c, f64factor7dccd0, f64factor7dccc8. The probe uses the shared production decode_player_reaction_constants owner. The output retains shared state, failure sites, normal calls, plane/angle partial facts, pending f64/ext80 bits, FP flags and CW. The deepest CRT boundary is serialized independently of intermediate caller438460/43c78c.

Private probe: `/private/tmp/sadx-port-sources/native-player-reaction-probe.dylib`; reusable runner: `sadx-native/tools/verify_original_player_reaction.py`. No production core, Makefile or shared archive was edited. The strict C++17 private probe build uses O2, ff-contract=off and no fast-math. The reaction owner and existing reaction test pass ASan+UBSan; the linked numeric core archive is not instrumented in that check.

## Source bindings

| Source address | Size | Exact bytes |
|---|---|---|
| 0x7dcb2c | 4 | `00000000` |
| 0x7dcb10 | 4 | `0000803f` |
| 0x7e6c30 | 4 | `9a99793f` |
| 0x7e0b40 | 8 | `000000000000f03f` |
| 0x7dcb0c | 4 | `000080bf` |
| 0x7dccd0 | 8 | `000000000000f040` |
| 0x7dccc8 | 8 | `0ae3e4b4305fc43f` |

| Native source/probe/tool | SHA256 |
|---|---|
| `src/player_reaction.cpp` | `477c673f441a1421d148297aa2fc7aa04763d6543d1e612d85829a0a105c9693` |
| `src/player_reaction.hpp` | `caec2970845dbe32c14dee0a5f6f7232afe3c624ec2f5aad76a8b3afa8ff040e` |
| `src/player_plane.cpp` | `d4e8708dcb409631067fff9ebac96eafaade5eb15ccc9105f66dbc75bff9de9e` |
| `src/player_projection.cpp` | `38ecb7d03d789b0e5ce1b3c53d330bd029ee177a9fb446f59409114b7dbea898` |
| `src/ninja_math.cpp` | `3c9788806b2015a5d29d6a77a6440bc20bd5958fae903b5dc69c972b5f5204d0` |
| `src/x87_math.cpp` | `632db745ec15eab0e8d5144e2fb63c20098eb4de3d64bd2b5828bf2be63e16a0` |
| `src/x87_transcendental.cpp` | `2e55ad21b3493525254fd3efa2ad385e3c2bbbfde26b2d493b0fb941baef9ec9` |
| `tests/native_player_reaction_probe.cpp` | `64bfb382cfd2efe4edc37b960f2233714703d650c4109e80b0c6bd464f4dcf7c` |
| `tools/verify_original_player_reaction.py` | `be86f37f55681f374db04954da2c65fc0f4b91503653aaf3a165ca22d59f9691` |

Actual private dylib SHA256: `87bdac99d4bcc056cabe34db2d0c61e317731696120e66e40ea3fd17aad7fef3`. Report function-byte SHA256 and source digests are retained as the observed source/probe snapshot. Root may rebuild and repin its production probe after integration.

Remaining goal: native PC gameplay with actual required mods, source initialization/error handlers and scene geometry, original hardware/reference validation and live Metal/controller integration. This reaction comparison closes a bounded local source slice, not the complete game.

Production integration update: the combined arm64 gate now passes44 native programs and4 extraction tests. Current production probe/report digests are pinned in continuation.json; older private-library measurements above remain historical. Full game/hardware equivalence is still unverified.
