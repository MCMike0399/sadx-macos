# MPEG timing readability context

Base Main and assigned isolated worktree start at a0aa342. Only the complete
67-line loader/mpeg_timing header and implementation are source scope. PE and
heap integration already exists at this base; no edits to those owners.

The sole runtime caller is HleDshow AddSourceFilter. It reads file bytes,
propagates parser errors with its own prefix, records duration in Graph, clears
completion/position and explicitly assumes PTS span plus one frame. Graph clamps
virtual stream time to that duration; get_Duration/get_CurrentPosition convert
100 ns integer values to double by division by 1e7; deliver_due queues one
EC_COMPLETE. These existing assumptions remain unchanged. Native MoviePlayer's
AVFoundation/sample/audio presentation clock is a different responsibility.

Source is the contract for this readability packet, not a recovered Windows
splitter specification. MPEG start tags, nibble masks, timestamp bit positions,
90 kHz conversion and rate fractions are format encodings, not addresses.
No symbol lookup, provider, network or new format research is needed.

Preserve E0-only video, fixed 12-byte MPEG-1 packs, all other packet skips,
stuffing/STD handling, permissive marker decoding and PTS+DTS payload offset,
first usable rate and size, invalid-rate break within a packet but retry in
later packets, numeric PTS min/max without unwrapping, independent truncated
integer terms, unchanged error strings/ordering and success leaving error intact.
A sequence header need not have any pictures; zero dimensions remain accepted.
Missing PTS takes precedence over missing sequence header. A short trailing tail
is ignored, and end code terminates immediately, including trailing garbage.
No MPEG-2, split-header or other stream-ID support is introduced.

Qualification starts with fresh unchanged unit, synthetic full byte/result corpus
and actual HleDshow consumer calls. New fixtures and registrations are a separate
evidenced judge commit. Source changes are then judged by exact full outputs and
compiled discriminating faults. Existing protected inputs are retained; do not
refresh the manifest without all 53 frozen raw inputs. Root owns independent
review, final protected union, inventory reconciliation, combined gate and
native/reference journeys. No gate, delegates, Main writes or push here.
