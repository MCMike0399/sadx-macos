#pragma once
#include "engine/native_texture_lifetime.hpp"
#include "game/game_cleanup.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
namespace sadx::menu_caption {
// NOTSA: borrowed immutable asset identity; the imported help catalog owns the bytes.
using TextIdentity = const std::string *;
// NOTSA: native allocation resource; bytes outside the initialized bitmap remain indeterminate.
struct Buffer {
    explicit Buffer(std::size_t size) : bytes(new std::uint8_t[size]), size(size) {}
    std::unique_ptr<std::uint8_t[]> bytes;
    std::size_t size;
};
// NOTSA: source bitmap storage borrowed from a heap buffer or the maintained draw arena.
struct BitmapView {
    std::uint8_t *bytes;
    std::size_t size;
};
// MSGC. The texture list retains canonical texture-memory references when copied for release.
struct Context : cleanup::Message {
    Context() : cleanup::Message{0} {}
    std::int16_t x = 0, y = 0;
    std::uint16_t width = 0, height = 0, buffer_width = 0, buffer_height = 0;
    std::uint16_t bitmap_width = 0, bitmap_height = 0;
    std::int16_t cursor_x = 0, cursor_y = 0, spacing_x = 0, spacing_y = 0;
    std::uint16_t color = 0;
    std::uint32_t foreground = 0, background = 0;
    float scale = 0;
    std::uint16_t *bitmap = nullptr;
    std::size_t bitmap_capacity = 0;
    std::uint32_t global_index = 0;
    texture_lifetime::List textures{};
    bool texture_names_attached = false;
    std::uint32_t flags = 0;
};
// NOTSA: semantic caption resource owner instantiated by each native/reference world.
struct State {
    // GLOBAL: SONIC 0x3abdf84 DialogJimakuBuffer size=4
    std::shared_ptr<Buffer> buffer = {};
    // GLOBAL: SONIC 0x3abdf88 DialogJimakuTimer size=4
    std::uint32_t timer = 0;
    // GLOBAL: SONIC 0x3abdc18 DialogJimakuContext size=72
    Context message = {};
    // GLOBAL: SONIC 0x3abdc8c DialogJimakuLastText size=4
    TextIdentity last_text = nullptr;
};
struct ReleaseEntry {
    texture_lifetime::List textures{};
    bool names_present = false;
};
struct DeferredRelease {
    // NOTSA: borrows the existing Display/reference-world registered texture-list count.
    std::uint32_t &count;
    std::array<ReleaseEntry, 32> lists{};
};
struct Calls {
    virtual ~Calls() = default;
    virtual std::shared_ptr<Buffer> allocate_buffer(std::size_t) = 0;
    virtual void free_buffer(std::shared_ptr<Buffer>) = 0;
    // NOTSA: source TH_Wait; service must use the actual cooperative-thread owner.
    virtual void wait_for_cooperative_thread() = 0;
    // NOTSA: composition service delegates to message_text::put_text.
    virtual void append_message(Context &, const std::string &) = 0;
    // NOTSA: composition service delegates to message_text::request_texture_upload.
    virtual void load_message_texture(Context &) = 0;
    // NOTSA: connects message contexts to cleanup::display_message's canonical queue.
    virtual void draw_message(Context &) = 0;
    virtual void release_texture_list(texture_lifetime::List &) = 0;
    // NOTSA: source assertion failure for the32-entry deferred release array.
    virtual void release_queue_overflow() = 0;
};
// FUNCTION: SONIC 0x40d850 MSG_Cls
void clear_message(Context &, std::uint32_t &pause_state, Calls &);
// FUNCTION: SONIC 0x40e1e0 NH_MSG_Open
void open_message(Context &, std::int16_t x, std::int16_t y, std::int32_t width,
                  std::int32_t height, std::uint32_t global_index, BitmapView,
                  std::uint32_t &pause_state, Calls &);
// NOTSA: compatibility overload retains owning Caption allocations and their existing callers.
void open_message(Context &, std::int16_t x, std::int16_t y, std::int32_t width,
                  std::int32_t height, std::uint32_t global_index, Buffer *,
                  std::uint32_t &pause_state, Calls &);
// FUNCTION: SONIC 0x40bc80 DialogJimakuInit
void initialize(State &, std::uint32_t &pause_state, Calls &);
// FUNCTION: SONIC 0x40bd30 DialogJimakuPut
void put(const std::string *, State &, std::uint32_t &pause_state, Calls &);
// FUNCTION: SONIC 0x40d330 QueueMessageTextureRelease
void queue_release(const Context *, DeferredRelease &, Calls &);
// FUNCTION: SONIC 0x40e2c0 NH_MSG_Close
void close_message(Context &, DeferredRelease &, Calls &);
// NOTSA: original close/free boundaries shared by native resources and reference receipts.
struct FrameCalls {
    virtual ~FrameCalls() = default;
    virtual void close_message() = 0;
    virtual void free_buffer(std::shared_ptr<Buffer>) = 0;
};
// NOTSA: named prefix permits reference validation before the following buffer read.
void advance_timer(State &);
// NOTSA: shared source release tail; the buffer is reloaded after close_message.
void release_expired(State &, FrameCalls &);
// FUNCTION: SONIC 0x40bcf0 CompleteDialogJimakuFrame
void complete_frame(State &, DeferredRelease &, Calls &);
// FUNCTION: SONIC 0x40d3c0 ReleaseMessageTextures
void release_queued(DeferredRelease &, Calls &);
using HelpCatalog = std::array<std::array<std::shared_ptr<const std::string>, 5>, 3>;
// FUNCTION: SONIC 0x5056a0 FileSelectDialogHelp
void draw_help(std::int8_t cursor, const HelpCatalog &, const std::int32_t &language, State &,
               std::uint32_t &pause_state, Calls &);
} // namespace sadx::menu_caption
