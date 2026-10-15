# SADX macOS

The project reconstructs Sonic Adventure DX for native macOS play with the pinned
speedrun behavior. These terms distinguish the delivered game from its reference.

## Language

**Native game**:
The delivered macOS game whose game logic and speedrun behavior execute as compiled
arm64 code, using imported assets.

**Reference runtime**:
The development version that executes the pinned Windows game and mods to supply
behavioral evidence and comparisons.

**Imported assets**:
Game content obtained from the user's own media or installation and made available
independently of the original executable containers.

**Pinned speedrun profile**:
The specific game, mod versions and settings that define the behavior this port
must reproduce, including their speedrun techniques and quirks.

**Source provenance**:
The evidence connecting a reconstructed game behavior or datum to the pinned original.

**Game tier**:
The shipping source: game systems, the Ninja engine, the pinned profile's behaviour and
the native session, written for readers (semantic names, structured flow, literal
constants). It names original addresses only in provenance annotations.

**Reference tier**:
The development-only code that connects the game tier to the original for proof: the
guest, HLE, reference adapters and the override table. Everything address-shaped lives
here.

**Reference adapter**:
Per-system reference-tier code that marshals guest memory to a system's state types, fills
its seams from the guest, and exposes it to oracles and override registration.

**Seam (STUB)**:
A named operation through which game-tier code reaches behaviour that is not recovered
yet. The adapter supplies it from the guest; a native build without it fails loudly.

**Enhancement**:
An opt-in addition beyond the pinned profile, off by default. It is either replay-neutral
(proved not to change game state) or gameplay-affecting (marks the session non-faithful).

**Readability ratchet**:
The per-file count of address-shaped constructs in the game tier, tracked in the gate, which
may only go down.
