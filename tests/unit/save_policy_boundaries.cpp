// Additional contracts for source sentinel identity and reentrant live scheduling.
#include "game/save_file_list.hpp"
#include "game/save_frame.hpp"
#include <cassert>
#include <limits>
#include <vector>
namespace {
struct Failure {};
struct Reentry final : sadx::save::FrameCalls {
    std::uint8_t ready = 255, count = 255, exec = 255, pending = 255;
    std::int32_t phase = -2;
    std::uint32_t pause = std::numeric_limits<std::uint32_t>::max();
    sadx::menu_caption::Context message;
    std::vector<int> events;
    bool nested_display = false, fail_close = false, fail_writer = false;
    sadx::save::FrameStateView view() { return {ready, count, phase, exec, pending, pause, message}; }
    void display_saving_message(sadx::menu_caption::Context &context) override {
        assert(&context == &message && count == 0 && phase == 1 && exec == 1);
        events.push_back(1);
        ready = 0;
        phase = 0;
        pending = 7;
        exec = 9;
        nested_display = true;
        sadx::save::before_frame_completion(view(), *this);
        nested_display = false;
        phase = 0;
        exec = 7;
    }
    void close_saving_message(sadx::menu_caption::Context &context) override {
        assert(&context == &message);
        events.push_back(2);
        if (fail_close) throw Failure{};
        pending = 5;
        pause = 2;
        if (!nested_display) phase = 1;
    }
    void write_save_file() override {
        assert(exec == 0);
        events.push_back(3);
        if (fail_writer) throw Failure{};
        sadx::save::after_frame_completion(view(), *this);
        assert(pause == 1);
        pending = 8;
        pause = 2;
    }
};
void list_identity() {
    namespace files = sadx::file_select;
    files::List head;
    const std::string name = "SonicDX01.snc";
    files::append_save_list(head, &name, {name, 123});
    auto sentinel = head;
    assert(sentinel->is_sentinel() && !sentinel->file());
    try { files::append_save_list(head, nullptr, {}); assert(false); }
    catch (const files::Error &error) { assert(error.boundary() == files::Boundary::IndeterminateSentinel); }
    auto partial = sentinel->next;
    assert(partial && !partial->is_sentinel() && !partial->file());
    files::append_save_list(head, &name, {name, std::numeric_limits<std::uint64_t>::max()});
    auto file = partial->next;
    assert(!file->is_sentinel() && file->file()->name == name);
    assert(file->file()->last_write_time == std::numeric_limits<std::uint64_t>::max());
    files::free_save_list(head);
    assert(!head && sentinel->is_sentinel() && !sentinel->next);
    assert(!partial->is_sentinel() && !partial->file() && !partial->next);
    assert(!file->is_sentinel() && !file->file() && !file->next);
}
}
int main() {
    list_identity();
    Reentry state;
    sadx::save::before_frame_completion(state.view(), state);
    sadx::save::after_frame_completion(state.view(), state);
    assert((state.events == std::vector<int>{1, 2, 3, 2}));
    assert(state.ready == 0 && state.count == 0 && state.phase == 1);
    assert(state.exec == 0 && state.pending == 0 && state.pause == 1);
    Reentry close_failure;
    close_failure.fail_close = true;
    try { sadx::save::before_frame_completion(close_failure.view(), close_failure); assert(false); }
    catch (const Failure &) {}
    assert((close_failure.events == std::vector<int>{1, 2}));
    assert(close_failure.exec == 9 && close_failure.pending == 7 && close_failure.phase == 0);
    Reentry writer_failure;
    sadx::save::before_frame_completion(writer_failure.view(), writer_failure);
    writer_failure.fail_writer = true;
    try { sadx::save::after_frame_completion(writer_failure.view(), writer_failure); assert(false); }
    catch (const Failure &) {}
    assert(writer_failure.exec == 0 && writer_failure.pending == 0 && writer_failure.pause == 2);
}
