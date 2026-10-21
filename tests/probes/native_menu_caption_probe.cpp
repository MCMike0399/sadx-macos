#include "game/menu_caption.hpp"
#include "engine/cooperative_thread_native.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>
namespace c = sadx::menu_caption;
namespace {
struct Calls final : c::Calls {
    std::uint32_t *out;
    std::uint32_t &pause;
    std::int32_t &language;
    bool fail_allocation;
    std::uint32_t language_after_wait;
    std::shared_ptr<void> current_thread;
    unsigned count = 0;
    Calls(std::uint32_t *o, std::uint32_t &p, std::int32_t &l, bool fail, std::uint32_t next)
        : out(o), pause(p), language(l), fail_allocation(fail), language_after_wait(next) {}
    void receipt(unsigned kind, unsigned argument = 0) {
        out[65 + count * 2] = kind;
        out[66 + count * 2] = argument;
        ++count;
        out[64] = count;
    }
    std::shared_ptr<c::Buffer> allocate_buffer(std::size_t size) override {
        receipt(1, static_cast<unsigned>(size));
        if (fail_allocation) return {};
        auto buffer = std::make_shared<c::Buffer>(size);
        std::fill_n(buffer->bytes.get(), buffer->size, 0xa5);
        return buffer;
    }
    void free_buffer(std::shared_ptr<c::Buffer>) override { receipt(2); }
    void wait_for_cooperative_thread() override {
        receipt(3, pause);
        if (language_after_wait != 0xffffffffU)
            language = static_cast<std::int32_t>(language_after_wait);
        if (sadx::cooperative_threads::wait(current_thread) !=
            sadx::cooperative_threads::WaitResult::Completed)
            throw std::runtime_error("active cooperative thread continuation is not recovered");
    }
    void append_message(c::Context &, const std::string &) override { receipt(4); }
    void load_message_texture(c::Context &) override { receipt(5); }
    void draw_message(c::Context &) override { receipt(6); }
    void release_texture_list(sadx::texture_lifetime::List &) override { receipt(7); }
    void release_queue_overflow() override { throw std::runtime_error("release overflow"); }
};
}
extern "C" void sadx_native_menu_caption_probe(const std::uint32_t *input, std::uint32_t *out,
                                               const char *const *raw_texts) {
    std::fill_n(out, 256, 0);
    std::uint32_t pause = input[1];
    std::int32_t language = input[0] == 2 ? static_cast<std::int32_t>(input[9]) : 1;
    Calls calls(out, pause, language, input[11] != 0, input[12]);
    c::State state;
    std::uint32_t registered_count = 0;
    c::DeferredRelease queue{registered_count, {}};
    std::shared_ptr<c::Buffer> allocation;
    if (input[0] != 1) {
        c::HelpCatalog texts{};
        std::unordered_map<const char *, std::shared_ptr<const std::string>> identities;
        const auto fixture = std::make_shared<const std::string>(input[2] ? "test" : "");
        for (unsigned choice = 0; choice < 3; ++choice) {
            for (unsigned language_index = 0; language_index < 5; ++language_index) {
                if (input[0] == 2) {
                    const auto *raw = raw_texts[choice * 5 + language_index];
                    auto &resource = identities[raw];
                    if (!resource) resource = std::make_shared<const std::string>(raw);
                    texts[choice][language_index] = resource;
                } else texts[choice][language_index] = fixture;
            }
        }
        for (unsigned cycle = 0; cycle < input[10]; ++cycle) {
            c::draw_help(input[0] == 2 ? static_cast<std::int8_t>(input[5]) : 0, texts,
                         language, state, pause, calls);
            allocation = state.buffer;
            if (input[3]) c::complete_frame(state, queue, calls);
            if (input[4]) c::release_queued(queue, calls);
        }
    } else {
        allocation = std::make_shared<c::Buffer>(262208);
        std::fill_n(allocation->bytes.get(), allocation->size, 0xa5);
        c::open_message(state.message, static_cast<std::int16_t>(input[5]),
                        static_cast<std::int16_t>(input[6]), static_cast<std::int32_t>(input[7]),
                        static_cast<std::int32_t>(input[8]), input[9], allocation.get(), pause, calls);
    }
    const auto &message = state.message;
    out[0] = state.buffer ? 1 : 0;
    out[1] = state.timer;
    out[2] = queue.count;
    out[3] = pause;
    const std::uint32_t values[]{
        message.kind, static_cast<std::uint16_t>(message.x), static_cast<std::uint16_t>(message.y),
        message.width, message.height, message.buffer_width, message.buffer_height,
        message.bitmap_width, message.bitmap_height, static_cast<std::uint16_t>(message.cursor_x),
        static_cast<std::uint16_t>(message.cursor_y), static_cast<std::uint16_t>(message.spacing_x),
        static_cast<std::uint16_t>(message.spacing_y), message.color, message.foreground,
        message.background};
    std::copy_n(values, 16, out + 4);
    std::memcpy(out + 20, &message.scale, 4);
    out[21] = message.bitmap ? 1 : 0;
    out[22] = message.global_index;
    out[23] = message.flags;
    out[24] = message.texture_names_attached;
    out[25] = static_cast<std::uint32_t>(message.textures.entries.size());
    if (allocation) {
        void *aligned = allocation->bytes.get();
        auto remaining = allocation->size;
        std::align(32, 1, aligned, remaining);
        const auto pixels = static_cast<std::size_t>(message.bitmap_width) * message.bitmap_height;
        const auto *bytes = static_cast<const std::uint8_t *>(aligned);
        unsigned nonzero = 0;
        for (std::size_t index = 0; index < pixels * 2; ++index) nonzero += bytes[index] != 0;
        out[26] = nonzero;
        out[27] = bytes[pixels * 2];
    }
    out[28] = queue.lists[0].names_present;
}
