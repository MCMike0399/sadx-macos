# Sonic animation current-tree integration

Candidate branch: `packet/sonic-animation-integration`, supplied T3 worktree
`t3-d77b6d10`, base Main `76b44198`. Root alone admits Main.
Accepted source `a77d7110` + `02a6d9a4`, judge `c4616102`, consumed as
`d321d89d`, `ccd5d026`, `94f5cf72`. Reviews `d410b20a` and `9663fd1c`
accepted without actionable findings. Current-tree source is `ac7e2ad3`.

One PlayerSystem unique_ptr owns SonicAnimationAssets. Session rejects null and
replacement installs. The app optionally loads its metadata against the same
combined CharacterPack. The actual TitleScene Sonic callback borrows the table's
shared_ptr member and creates an invocation-local animation Runtime. Existing
stage/event capture/parameters/editor/forced/alternate semantics remain intact.
Gamma setup/effects/material owners and current source APIs are preserved.
Accepted owner, loader, importer, unit and verifier hashes are unchanged.
No duplicate geometry, scene or parameter owner; resources.objects remains absent.

The metadata-only import reconstructs the existing pack exactly: 3,328 records,
zero appends, SHA-256
`c414e173bf4bf9f803f24d61642e233ff9650fe9276d70da2ac3939fee2b5d7f`.
149 declarations retain 4 loaded, 3 known-null, 142 unavailable and all 147
canonical rows. No resource renumbering or inherited asset replacement.

Focused qualification (`VERIFICATION.json` has commands, exits and hashes):

- Fresh local native build at -j2 and force-load closure pass, using dereferenced
  local copies of public SDL3/libusb and the required SDL-header make override.
- Four native programs pass: accepted owner/init/Stand witness, new Session and
  actual-callback integration test, existing Sonic callback/event-capture test,
  existing Gamma effects real-caller test. The first also retains malformed-input,
  immutable table/key lifetime and real two-player initialization checks.
- Original imported table comparison matches every one of 2,352 bytes.
  Accepted unchanged 103-case oracle, 882 publications and compiled fault evidence
  are reused explicitly; no claim that those suites were rerun here.
- Readability, layers, unique owners and provenance pass. The metadata importer
  initially lacked capstone under system Python; the existing reference venv
  completed it successfully. No package/provider/environment changes were made.
- Actual normal input uses Main's same key schedule and stops at frame 1332,
  exit 1, native_gamma_set_motion. The inspected outgoing-title image has the
  same `c7d02f64…` hash as Main. This is no completed actor frame.

Registration is a separate evidenced fix(judge) commit. Five protected additions
and two registration-file hash changes retain every inherited manifest path,
including all 53 absent isolated-worktree binary protections. The judge checker
exits 1 with exactly those 53 missing files and no other drift; it is not a pass.
Two save-warning header evidence rows and their generated mirror are narrowly
rebound: removing the single added Session API declaration reproduces the old
header byte for byte. Owner/borrower proof and unrecovered_open_debt remain.
The inventory checker exits 1 with isolated-tree/content drift; broad inventory
regeneration is intentionally left to Root. See judge-reconciliation.json and
save-warning-rebinding.json. No existing test/fixture/fault was edited.

Dynamic limit: the real Sonic callback stops at sonic_check_edit_mode_editor
before animation consumption, both without and with metadata. The controlled
accepted witness separately reaches real initialization/Stand; it does not supply
production editor defaults. Normal input stops earlier in Gamma motion.
Editor/model/common tail/PSetMotion, other actors and N3b remain open. No new
full gate, unrelated suites, reserved Task/reset inputs, particle lifecycle file
edits, Root writes, push, schedules, storage or provider changes occurred.
