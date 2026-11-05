# Gamma first animated body — packet/draw-path-2

Source commit `f814c96`, based on `6fc88b1`. The real Gamma constructor/display now
completes its first body action: **77 motion nodes, 98 submitted draws and 2,862
vertices**, then stops at
`native_gamma_display_missing_arm_action_export_50_after_body`.
This is submission progress. There is still no completed CharacterSelect actor
frame or N3b acceptance.

The ordinary source call now runs through ds_ActionClipNoScale, DrawAction,
motion/frame setup, root-wide transform selection and animated MDATA2 traversal.
Translation, rotation and scale consume the existing imported/generated motion
owners; model drawing uses the maintained graph, matrix stack, materials,
geometry, texture lists and vertex buffers. No static pose, identity replacement,
emulator call or second renderer was introduced.

Actual integration exposed additional existing renderer boundaries. The recovered
constant-material dispatcher reuses Direct3D_ParseMaterial and the existing normal/UV
strip leaf. New triangle/quad/list leaves borrow canonical buffers 6 and 8. Quads
emit source corners 0,1,2 then 1,2,3. Polygon table entries call the source's proven
bare RET; they are not an omitted unknown. `njRotateZYX` applies Y, X, then Z;
source execution and the stored assembly distinguish it from an X/Y/Z composition.

The two object-callback slots remain distinct. E102Display publishes the Ninja
callback; the animated traversal slot remains zero on this route. No bridge was
invented. The Gamma callback itself is separately recovered and compared for rows
12 and 28, full-word aim guards, low-word rotation and doubled recoil. Those two
rows are unavailable in the supplied private append if that callback is enabled.
After the body, the source callback clear, matrix push, recoil translation,
canonical node-54 multiplication and arm rotation execute before the missing arm
export is consumed.

All rows below have **zero value failures**. Counts and flags are separate cohorts;
FP partial counts are cases with equal compared values and differing FP flags.

| Cohort | Development | Holdout | FP partials, dev / holdout |
|---|---:|---:|---:|
| Preserved display topology | 1,126 | not rerun separately | not compared |
| Preserved display placement prefix | 1,024 | not rerun separately | not compared |
| DrawAction / MDATA2 / traversal | 1,253 | 4,325 | not compared |
| Actual numeric XYZ / ZYX / translation | 1,222 | 2,246 | 1,072 / 2,096 |
| Gamma object callback | 1,024 | 2,048 | 13 / 13 |
| Ordinary selection and postbody prefix | 1,024 | 2,048 | 90 / 90 |
| Constant-material dispatcher | 1,024 | 2,048 | 373 / 783 |
| Normal/UV triangles, quads and list wrappers | 1,024 | 2,048 | 812 / 1,620 |
| Normal-only strip, triangles and quads | 1,536 | 3,072 | 0 / 0 |
| Polygon RET source proof | 1,024 | none | 0 |

The draw cohort preserves 1,024 positive development cases and adds 208 negative
frame cases plus 21 numeric-leaf failure prefixes. Its holdout has 4,096 positive
cases plus the same boundary cohorts, seed `0x9e4053d1`. The five requested compiled
faults all compile with exit 0 and fail original comparison with exit 1:
wrong model callback identity 1,249 failures; transform order 1,176; missing child
traversal 1,228; missing final clip clear 154; wrong Gamma row selection 402.
The row fault uses its separate 1,024-case callback cohort. Nine additional
renderer faults are recorded in `CONSTANT-MATERIAL-VERIFICATION.json`.

Nine affected unit programs pass. The actual constructor test asserts nonempty
submitted geometry and all 77 consumed motion records, not only an exception
message. An isolated compiled suppression of the real model handoff reaches the
same arm stop but submits zero commands and aborts on the vertex assertion
(process exit -6, shell status 134). Generated-action tests also distinguish
known-null keys from unbound references, retain channel state on failure and
exercise the loop guard without consuming unrelated player fields.

The final unmodified offscreen Metal app uses normal input and a new directory
containing copies of all three verified save files. It exits **1 at frame 1332**,
completed frame count **1330**, pending logic ticks **0**, at the missing arm export.
The original save files remain byte-identical. The canonical private pack hash is
`382e3e53b613c42890c9e48afa3fae3e4d9398e6520019a1a2150451422688dc`.

The inspected 640×480 image is still the outgoing title:
`build/draw-path-2/normal-input-qualified/final.ppm`, SHA-256
`c7d02f642ae8e5493f19051e7cf13a06a2d1d103351e72b0576a108b52aba427`.
The exception occurs inside `Session::advance`, before `end_game_frame`, frame
completion and `present_target`; the offscreen catch saves the last rendered
texture (`src/apps/sadx_native.mm`, advance and offscreen catch). A private,
read-only observer additionally measures the body submission in the actual normal
run. It observes the exact body-call delta separately from the frame's other draw;
its commands, source hashes and counts are in `VERIFICATION.json`. It changes no
presentation or game state and produces the same screenshot hash.

Reproduce the product run after building with `-j2`, copying the baseline save to
a new directory first:

```sh
./build/sadx-native --assets build/gamma-display-native/assets \
  --controller-library build/native-controller-deps/lib/libSDL3.0.dylib \
  --offscreen --paced --exit-after 1550 \
  --save-directory NEW_SAVE_COPY \
  --key-events '500:13:down,501:13:up,750:13:down,751:13:up,950:13:down,951:13:up,1050:13:down,1051:13:up,1300:13:down,1301:13:up' \
  --screenshot build/gamma-body.ppm
```

Exact verification argv, exits, hashes, source fingerprints and fault results are
in `VERIFICATION.json` and `CONSTANT-MATERIAL-VERIFICATION.json`. Source evidence
is retained in the adjacent evidence manifests and text extracts. Independent
STYLE section 12 review found and corrected channel-store order, generated-null
reference handling, premature payload access and an unrelated-field read.
The final review found no remaining concrete source issue.

`make -j2 check` exits **2**, with **53 judge registration violations** inherited or
proposed by these packets. Layers, ownership, provenance, readability and constexpr
source checks pass. No judge manifest or `tools/gate` file was edited. The separate
range-aware profile check exits **1**: development and holdout each intersect the
same active loader `LinearMotionA_r` hook at `0x404c10`, whose registry handler is
still null. The tested sampler is the original branch while interpolation is
disabled; the enabled hook branch and its Windows caller state remain unqualified.
The orchestrator owns registry admission. No full gate was run.

Remaining boundaries are arm export 50 and the subsequent arm/matrix/attachment,
equipment, effects and cleanup tail; event blending, walking sway and hidden-display
branches; non-MDATA2 motion layouts, other DrawAction callbacks, nonzero clip-scale
view tests and other vertex layouts; enabled loader interpolation; missing Gamma
callback rows if that separate slot is enabled; active welding and later gameplay
qualification. Repeated completed display, cancellation/reentry, all other actors,
a cold source-only build, the full unit/oracle suites, Windows pixels and N3b are
not certified by this packet.
