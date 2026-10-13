# Windows system DLLs the guest runs as real code (local only; never committed)

`tools/gate/check_repo_hygiene.py` and `.gitignore` keep every `*.dll` out of the repository. These are the
32-bit Microsoft Visual C++ runtime DLLs the mod loader and the mods import. The guest (src/hle) loads them
as real code instead of re-implementing the MSVC STL ABI; everything below them (the UCRT API sets,
KERNEL32, USER32…) is HLE.

| File | Version | SHA-256 | Source on this Mac |
|---|---|---|---|
| msvcp140.dll | 14.51.36247.0 | `f0cda2a0cf1fe6fbbf579b9098462329d3aaa7513a207af2e0f33b01456e388a` | CrossOver bottle `Persona 3 Reload`, `drive_c/windows/syswow64/` (installed by a VC++ 2015-2022 x86 redistributable; not a Wine builtin) |
| vcruntime140.dll | 14.51.36247.0 | `2fa6efc053203460a23d3a25158f227d895d2dadc63acc1a372da97c3a4281c3` | same |

The reference Windows machine's runtime version was not captured; the guest-boot report lists this as an
assumption. Recreate with:

    B="$HOME/Library/Application Support/CrossOver/Bottles/Persona 3 Reload/drive_c/windows/syswow64"
    cp "$B/msvcp140.dll" "$B/vcruntime140.dll" reference/windows-system/
