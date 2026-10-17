# GOAL 6.2: Direct3D calling-thread precision

Microsoft's D3DCREATE and Top Issues for Windows Titles documentation specify
single precision / round to nearest unless FPU_PRESERVE is requested:
https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcreate
https://learn.microsoft.com/en-us/windows/win32/dxtecharts/top-issues-for-windows-titles
The Windows thread capture in HANDOFF, "Floating-point observations", observed
main-thread CW 0x007f. ADR-0008 authorizes this reference-platform correction.

The helper changes guest PC/RC and normalizes reserved bit 6, retaining masks,
status, x87 stack and MXCSR. Each device retains its own creation flags in HLE
metadata. Reset applies its FPU policy then retains the existing explicit
unmodelled resource-recreation boundary; this is not successful Reset support.

14 guest COM cases pass: five initial words with and without PRESERVE plus four
alternating-device Reset calls. Changing PRESERVE's bit to 0x4 makes the test
fail. Six 120-frame smoke variants preserve trace parity and show 0x007f after
creation, with shared startup entries also sampled at 0x023f. Full census and
phase separation follow in the separate judge step. No broad float-default or
full gate claim is made by this source commit.
