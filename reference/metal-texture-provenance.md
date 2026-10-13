# Actual Metal source textures

The separate Objective-C++ consumer accepts the caller's real Metal device and
queue and retains a shared immutable archive owner, record ordinal, archive SHA,
source packed16 samples and all mip/range metadata. It uploads R16Uint textures
and uses a safe/precise format shader to write direct5/6/4-bit channel ratios into
RGBA32Float, with no RGBA8 intermediate. The resource is ready only after the
real command buffer completes; partial decoded/allocated facts survive tagged
failures. No texaddr, original cache/refcount, gameplay binding or sampler/blend
reply is manufactured. Apple Silicon shared memory is an explicit supported
profile; other device memory profiles remain tagged unsupported.

Actual Apple M5 Pro checks cover532 source records,540 levels and16,883,028 FP32
channels, all with zero differences against direct float32 channel ratios. This
establishes the declared upload/normalization model on this GPU, not original
NVIDIA/D3D hardware equivalence. EFF_REGULAR136 records/144 levels also pass
UBSan; missing device/queue/archive, invalid ordinal and mip remain explicit.

The bubble's actual GPU FP32 readback and inspection PNG are in
metal-texture-readback/bubble.*. The PNG quantizes real GPU channels to the
nearest byte solely for preview. The original source packed16 resource remains
authoritative. Current hardware reports FP32 filtering support, but this owner
chooses no filter or blend state. Native sprite consumers can retain this same
resource with a draw after an actual texture-list resolver closes that boundary.

SDK header identities, official Apple documentation links, private report hashes
and limitations are in metal-texture-ledger.json. The parent separately retains
its canonical production archive/executable bubble gate; private all-archive
reports and production-gate identity are deliberately distinct. Earlier private
all-archive runs precede a CLI-only PNG error-cleanup repair; uploader/kernel
source was unchanged, and final bubble/UBSan checks use the final source.
