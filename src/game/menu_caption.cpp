#include "game/menu_caption.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace sadx::menu_caption {
namespace {
// NOTSA: source power-of-two allocation dimension, bounded at the original overflow boundary.
std::uint16_t bitmap_dimension(std::int32_t value, std::uint32_t maximum) {
    std::uint32_t result = 8;
    while (static_cast<std::int32_t>(result) < value) {
        if (result == 1073741824U)
            throw std::runtime_error(
                "message bitmap dimension overflows the source signed doubling loop");
        result *= 2;
    }
    return static_cast<std::uint16_t>(std::min(result, maximum));
}
} // namespace
void clear_message(Context &message, std::uint32_t &pause_state, Calls &calls) {
    const auto pixels = static_cast<std::uint32_t>(message.bitmap_width) * message.bitmap_height;
    if (message.bitmap && static_cast<std::int32_t>(pixels) > 0) {
        if (pixels > message.bitmap_capacity)
            throw std::runtime_error("message clear exceeds its bitmap allocation");
        std::fill_n(message.bitmap, pixels, 0);
    }
    message.cursor_y = 0;
    message.cursor_x = 0;
    message.flags |= 1;
    pause_state = 1;
    calls.wait_for_cooperative_thread();
    pause_state = 0;
}
void open_message(Context &message, std::int16_t x, std::int16_t y, std::int32_t width,
                  std::int32_t height, std::uint32_t global_index, Buffer *buffer,
                  std::uint32_t &pause_state, Calls &calls) {
    message.bitmap_width = bitmap_dimension(width, 1024);
    message.bitmap_height = bitmap_dimension(height, 512);
    void *aligned = buffer ? buffer->bytes.get() : nullptr;
    auto remaining = buffer ? buffer->size : 0;
    const auto required =
        static_cast<std::size_t>(message.bitmap_width) * message.bitmap_height * 2;
    if (buffer && !std::align(32, required, aligned, remaining))
        throw std::runtime_error(
            "message bitmap allocation cannot hold the aligned source dimensions");
    message.x = x;
    message.y = y;
    message.buffer_height = message.bitmap_height;
    message.kind = 0;
    message.width = static_cast<std::uint16_t>(width);
    message.height = static_cast<std::uint16_t>(height);
    message.buffer_width = message.bitmap_width;
    message.cursor_x = 0;
    message.cursor_y = 0;
    message.spacing_x = 2;
    message.spacing_y = 4;
    message.color = 65535;
    message.foreground = 4294967295U;
    message.background = 0;
    message.bitmap = static_cast<std::uint16_t *>(aligned);
    message.bitmap_capacity = remaining / 2;
    message.global_index = global_index;
    message.texture_names_attached = false;
    message.flags = 0;
    message.scale = 1;
    clear_message(message, pause_state, calls);
}
void initialize(State &state, std::uint32_t &pause_state, Calls &calls) {
    if (!state.buffer) {
        state.buffer = calls.allocate_buffer(262208);
        state.last_text = nullptr;
        open_message(state.message, 64, 400, 512, 40, 3489660976U, state.buffer.get(), pause_state,
                     calls);
        // Original shared context fields are reloaded after its wait service.
        state.message.spacing_x = 0;
        state.message.spacing_y = 0;
        state.message.background = 2147483648U;
        state.timer = 1;
    }
}
void put(const std::string *text, State &state, std::uint32_t &pause_state, Calls &calls) {
    if (!state.buffer || !text || text->empty())
        return;
    state.timer = 2;
    if (state.last_text != text) {
        clear_message(state.message, pause_state, calls);
        calls.append_message(state.message, *text);
        calls.load_message_texture(state.message);
        state.last_text = text;
    }
    calls.draw_message(state.message);
}
void queue_release(const Context *message, DeferredRelease &queue, Calls &calls) {
    if (!message) {
        queue.count = 0;
        return;
    }
    if (static_cast<std::int32_t>(queue.count) > 31)
        calls.release_queue_overflow();
    if (queue.count >= queue.lists.size())
        throw std::runtime_error(
            "message release overflow assertion returned beyond recovered storage");
    queue.lists[queue.count].textures = message->textures;
    queue.lists[queue.count].names_present = true;
    ++queue.count;
}
void close_message(Context &message, DeferredRelease &queue, Calls &calls) {
    queue_release(&message, queue, calls);
    message.bitmap = nullptr;
}
void advance_timer(State &state) {
    if (static_cast<std::int32_t>(state.timer) > 0)
        --state.timer;
}
void release_expired(State &state, FrameCalls &calls) {
    if (state.buffer && state.timer == 0) {
        calls.close_message();
        calls.free_buffer(state.buffer);
        state.buffer.reset();
    }
}
void complete_frame(State &state, DeferredRelease &queue, Calls &calls) {
    // NOTSA: binds the same two source callees to native message resources.
    struct NativeFrameCalls final : FrameCalls {
        State &state;
        DeferredRelease &queue;
        Calls &calls;
        NativeFrameCalls(State &s, DeferredRelease &q, Calls &c) : state(s), queue(q), calls(c) {}
        void close_message() override {
            menu_caption::close_message(state.message, queue, calls);
        }
        void free_buffer(std::shared_ptr<Buffer> buffer) override {
            calls.free_buffer(std::move(buffer));
        }
    } frame_calls(state, queue, calls);
    advance_timer(state);
    release_expired(state, frame_calls);
}
void release_queued(DeferredRelease &queue, Calls &calls) {
    // The source reloads the shared count after every list release.
    for (std::int32_t index = 0; index < static_cast<std::int32_t>(queue.count); ++index) {
        if (static_cast<std::size_t>(index) >= queue.lists.size())
            throw std::runtime_error("message release reads beyond recovered deferred storage");
        auto &list = queue.lists[static_cast<std::size_t>(index)];
        if (list.names_present) {
            calls.release_texture_list(list.textures);
            list.names_present = false;
            list.textures.entries.clear();
        }
    }
    queue.count = 0;
}
void draw_help(std::int8_t cursor, const HelpCatalog &texts, const std::int32_t &language,
               State &state, std::uint32_t &pause_state, Calls &calls) {
    initialize(state, pause_state, calls);
    const auto language_index = static_cast<std::uint32_t>(language);
    if (cursor < 0 || static_cast<std::size_t>(cursor) >= texts.size() || language_index >= 5)
        throw std::runtime_error("dialog caption choice/language has no imported help resource");
    const auto &text = texts[static_cast<std::size_t>(cursor)][language_index];
    put(text.get(), state, pause_state, calls);
}
} // namespace sadx::menu_caption
