# Ownership and consumer context

The complete modules are `game/save_file_list.{hpp,cpp}` and
`game/save_frame.{hpp,cpp}`. Source is pinned separately at a77dd95; later commits
contain only judge bindings/contracts/evidence.

List ownership stays at the existing Session lifetime: the field is now
`save_file_list_`, with no second head, vector or cache. TitleScene discovery/load,
writer, confirmation/name publication, display, selected-save public observation
and cleanup all borrow that same head. Profiles owns current/menu_backup as before;
load/enumeration still decode into current before closing each file. Neither
backend nor save codec, creation, Profile or SaveCompletion algorithm changes.

ListNode's variant stores exactly one of Sentinel, an unpopulated/released file
node, or FileInfo. It never constructs a FileInfo for the sentinel. First append
ignores the supplied filename/time; later append allocates and links the empty
file node before testing/copying the borrowed name. Reverse free clears payloads
before detaching links and releases the head last, preserving externally retained
node identities. Existing partial-prefix/read/close/search/traversal boundaries
and uint8 slot/count arithmetic are unchanged.

The frame algorithm is shared in game/save_frame.hpp. Native exported wrappers
still accept FrameStateView and FrameCalls; TitleScene::MessageServices still
passes the actual Session saving_message_ Context and invokes saving_message::display,
menu_caption::close_message and Session::write_live_save. No real service becomes
a STUB or a successful fixture callback. The separate warning/delete/cursor seams
remain as they were.

The reference loop creates only borrowed lazy field proxies. Conversion invokes
its existing need() at the original read PC; assignment writes straight into the
same optional owner without reading it. Message identity, pre/post close sites,
call cancellation and tagged pending calls stay at that boundary. The shared
policy has no addresses, PE facts, availability optionals or state copy-back.
Explicit int32_t phase capture preserves the original immediate read, rather than
accidentally copying a lazy proxy. The intervening render/tail calls and actual
frame_411d00 call remain in place. NativeGame's actual loop caller reaches this
same shared policy through run_game_loop_iteration.

Unrelated debt stays open: GameLoopState's reference-shaped transport is still
in game/game_loop.hpp; non-save loop/startup/shutdown/frame-completion interpreter
bodies are unchanged. ResetLeafState still transports a guest list pointer in its
separate reference world; no duplicate native list or second GLOBAL claim was
introduced. NativeGame's legacy survey dispatcher and underlying reference save
service routing are unchanged; this packet claims shared scheduling, not a whole
NativeGame/Session rewrite or newly recovered service behavior.

Evidence: accepted save-data-io-frame inventory SL1/SL2/SF1, foundation source freeze,
qualified-build/pinned-inputs, 39 ordered writer-capture source proof and the ten
unchanged default original judges. Local cached SDK *New.h names listData,
AddLineList, SaveCount, SaveReady and SaveExecFlag agree with those source facts.
No live provider or network was used. Integer operations retain their original
widths; no FP algorithm is rewritten. Controls' inherited partials remain partial.
